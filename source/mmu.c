/*****************************************************************
 *       mmu.c
 *       by Zhiyi Huang, hzy@cs.otago.ac.nz
 *       University of Otago
 *
 ********************************************************************/


#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"

unsigned int pm_size;

void mmuinit0(void)
{
	pde_t *l1;
	pte_t *l2;
	u_int32 pa, va;

	// diable mmu
	// use inline assembly here as there is a limit on 
	// branch distance after mmu is disabled
	//	asm volatile("mrc p15, 0, r1, c1, c0, 0\n\t"
	//		"bic r1,r1,#0x00000004\n\t"	// 2:  Disable data cache
	//		"bic r1,r1,#0x00001000\n\t" // 12: Disable instruction cache
	//		"bic r1,r1,#0x00000800\n\t" // 11: Disable branch prediction
	//		"bic r1,r1,#0x00000001\n\t" // 0:  Disable MMU
	//		"mcr p15, 0, r1, c1, c0, 0\n\t"
	//		"mov r0, #0\n\t"
	//		"mcr p15, 0, r0, c7, c7, 0\n\t" // Invalidate Both Caches (only for ARM11)
	//		"mcr p15, 0, r0, c8, c7, 0\n\t" // Invalidate Unified TLB
	//		::: "r0", "r1", "cc", "memory");


	//for(p=(u_int32 *)0x2000; p<(u_int32 *)0x8000; p++) *p = 0;

	l1 = (pde_t *) K_PDX_BASE;
	l2 = (pte_t *) K_PTX_BASE;

	// map all of ram at KERNBASE
	va = KERNBASE + MBYTE;
	for(pa = PHYSTART + MBYTE; pa < PHYSTART+PHYSIZE; pa += MBYTE){
		l1[PDX(va)] = pa|PDX_ATRB_DOMAIN0|PDX_ATRB_AP(PTX_ATRB_KRW)|PDX_ATRB_SECTION_ENTRY|PTX_ATRB_CACHED|PTX_ATRB_BUFFERED;
		va += MBYTE;
	}

	// identity map first MB of ram so mmu can be enabled
	//l1[PDX(PHYSTART)] = PHYSTART|PDX_ATRB_DOMAIN0|PDX_ATRB_AP(PTX_ATRB_KRW)|PDX_ATRB_SECTION_ENTRY|PTX_ATRB_CACHED|PTX_ATRB_BUFFERED;

	// map IO region
	va = MMIO_VA;
	for(pa = MMIO_PA; pa < MMIO_PA+MMIO_SIZE; pa += MBYTE){
		l1[PDX(va)] = pa|PDX_ATRB_DOMAIN0|PDX_ATRB_AP(PTX_ATRB_KRW)|PDX_ATRB_SECTION_ENTRY;
		va += MBYTE;
	}

	// map GPU memory
	va = GPUMEMBASE;
	for(pa = 0; pa < (u_int32)GPUMEMSIZE; pa += MBYTE){
		l1[PDX(va)] = pa|PDX_ATRB_DOMAIN0|PDX_ATRB_AP(PTX_ATRB_KRW)|PDX_ATRB_SECTION_ENTRY;
		va += MBYTE;
	}

	// double map exception vectors at top of virtual memory
	va = HVECTORS;
	l1[PDX(va)] = (u_int32)l2|PDX_ATRB_DOMAIN0|PDX_ATRB_PTX_ENTRY;
	l2[PTX(va)] = PHYSTART|PTX_ATRB_AP(PTX_ATRB_KRW)|PTX_ATRB_SMALL;

	//	asm volatile("mov r1, #1\n\t"
	//                "mcr p15, 0, r1, c3, c0\n\t"
	//                "mov r1, #0x4000\n\t"
	//                "mcr p15, 0, r1, c2, c0\n\t"
	//                "mrc p15, 0, r0, c1, c0, 0\n\t"
	//                "mov r1, #0x00002000\n\t" // 13: Enable High exception vectors
	//                "orr r1, #0x00000004\n\t" // 2:  Enable data cache
	//                "orr r1, #0x00001000\n\t" // 12: Enable instruction cache
	//                "orr r1, #0x00000001\n\t" // 0:  Enable MMU
	//                "orr r0, r1\n\t"
	//                "mcr p15, 0, r0, c1, c0, 0\n\t"
	//                "mov r1, #1\n\t"
	//                "mcr p15, 0, r1, c15, c12, 0\n\t" // Read Performance Monitor Control Register (ARM11)?
	//                ::: "r0", "r1", "cc", "memory");

}

void
mmuinit1(void)
{
	pde_t *l1;
	u_int32 va1, va2;
	u_int32 pa, va;

	l1 = (pde_t*)(K_PDX_BASE);


	// map the rest of RAM after PHYSTART+PHYSIZE
        va = KERNBASE + PHYSIZE;
        for(pa = PHYSTART + PHYSIZE; pa < PHYSTART+pm_size; pa += MBYTE){
                l1[PDX(va)] = pa|PDX_ATRB_DOMAIN0|PDX_ATRB_AP(PTX_ATRB_KRW)|PDX_ATRB_SECTION_ENTRY|PTX_ATRB_CACHED|PTX_ATRB_BUFFERED;
                va += MBYTE;
        }


	// undo identity map of first MB of ram
	l1[PDX(PHYSTART)] = 0;

	// drain write buffer; writeback data cache range [va, va+n]
	va1 = (u_int32)&l1[PDX(PHYSTART)];
	va2 = va1 + sizeof(pde_t);
	va1 = va1 & ~((u_int32)CACHE_LINE_SIZE-1);
	va2 = va2 & ~((u_int32)CACHE_LINE_SIZE-1);
	flush_dcache(va1, va2);

	// invalidate TLB; DSB barrier used
	flush_tlb();

}

