 /**
  * @file mmu.c
  *
  * mmu.c provides functions to set up the page tables
  * before and after the MMU is activated.
  *
  * mmu.c provides two functions, mmu_init_stage1 and
  * mmu_init_stage2, which set up the page tables
  * respectively before and after the MMU is activated.
  *
  * mmu_init_stage1 sets up a minimal page table mapping
  * so the OS can function and communicate with the
  * hardware when the MMU is enabled.
  *
  * mmu_init_stage2 maps in additional physical memory
  * after the full size of the physical memory has been
  * queried from the hardware.
  *
  * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
  * (Adaption from MIT XV6.)
  *
  * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
  * (Documentation, refactoring, and styling.)
  *
  * @date 06/01/2019
  */


#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"


 /**
  * Memory to store the size of the physical memory, after
  * the size has been read from the hardware.
  *
  * This is used to pass the value into mmu_init_stage2 from
  * getpmsize().
  *
  * @see main.c
  */
unsigned int pm_size;


/**
 * Maps the minimum virtual address space required to run
 * the MMU.
 *
 * mmu_init_stage1 initialises the small subset of virtual
 * addresses required to safely start the memory mapping
 * unit and run the operating system normally.
 *
 * We map only a limited amount of the physical memory
 * we don't know the size of the physical memory at
 * compile time or start time. To avoid exceeding the
 * available phsyical memory, we only allocate PHYSIZE
 * (256MB) starting at the location the kernel is stored.
 * By doing so, we assume PHYSIZE is smaller that the
 * physical memory of any hardware we intend to support,
 * and PHYSIZE is larger than the size of the kernel -
 * so all the kernel is mapped in.
 *
 * We also map in memory-mapped I/O devices including the
 * GPU control registers, and the trap vectors' high memory
 * address (Trap vectors are mapped into two location.)
 *
 * Once mmu_init_stage1 is complete, the initialisation
 * process can use the memory mapped I/O to query to
 * hardware and determine the full physical memory size
 * to map in mmu_init_stage2
 */
void mmu_init_stage1(void)
{
    pde_t* l1;
    pte_t* l2;
    u_int32 pa;
    u_int32 va;
    l1 = (pde_t*) K_PDX_BASE;
    l2 = (pte_t*) K_PTX_BASE;
    /* Map, for the OS, 256MB from physical address 0x0 to
     * virtual address 0x80000000. */
    va = KERNBASE + MBYTE;
    for(pa = PHYSTART + MBYTE; pa < PHYSTART + PHYSIZE; pa += MBYTE){
        l1[PDX(va)] = pa
                    | PDX_ATRB_DOMAIN0
                    | PDX_ATRB_AP(PTX_ATRB_KRW)
                    | PDX_ATRB_SECTION_ENTRY
                    | PTX_ATRB_CACHED
                    | PTX_ATRB_BUFFERED;
        va += MBYTE;
    }
	/* Map the memory mapped I/O devices from PA 0x3f000000
	 * to VA 0xD0000000 */
    va = MMIO_VA;
    for(pa = MMIO_PA; pa < MMIO_PA + MMIO_SIZE; pa += MBYTE){
        l1[PDX(va)] = pa
                    | PDX_ATRB_DOMAIN0
                    | PDX_ATRB_AP(PTX_ATRB_KRW)
                    | PDX_ATRB_SECTION_ENTRY;
        va += MBYTE;
    }
	/* Map 1Gb of GPU memory, from PA 0x0 to VA 0x40000000.
	 * The GPU buffer is nonfunctional in the XV6 for RPI2
	 * RPI 3.
	 * @note Perhaps this is to do with the PA clash with the
	 * kernel.
	 */
    va = GPUMEMBASE;
    for(pa = 0; pa < (u_int32) GPUMEMSIZE; pa += MBYTE){
        l1[PDX(va)] = pa
                    | PDX_ATRB_DOMAIN0
                    | PDX_ATRB_AP(PTX_ATRB_KRW)
                    | PDX_ATRB_SECTION_ENTRY;
        va += MBYTE;
    }
	/* Double map exception vectors at top of virtual memory. */
    va = HVECTORS;
    l1[PDX(va)] = (u_int32) l2
            | PDX_ATRB_DOMAIN0
            | PDX_ATRB_PTX_ENTRY;
    l2[PTX(va)] = PHYSTART
            | PTX_ATRB_AP(PTX_ATRB_KRW)
            | PTX_ATRB_SMALL;
}


/**
 * Allocates the rest of the physical memory after the
 * actual size is known.
 *
 * mmu_init_stage2 allocates any maps any remaining
 * physical memory into the kernel space.
 *
 * After the total physical memory size has been
 * determined, the size is stored in the global variable
 * pm_size.
 *
 * 256MB has already been mapped to the kernel in
 * mmu_init_stage1.
 *
 * The key loop in mmu_init_stage2 maps any remaining
 * memory into kernel space for management or use.
 *
 * @note - This is a system in general: We have 4GB
 * of address space (ARM xv6 still supports the 32 bit
 * RPI1), of which only the second 2GB is used as kernel
 * address space. If the hardware has more that 2GB of
 * physical memory for us to map, this algorithim would
 * cause bug related to overflow in the page tables.
 * Practical OS'es must make some assumptions about their
 * hardware (as this does - it works fine on a Pi with
 * < 2GB of physical memory) but good OS'es should assume
 * as little as possible - and would have a more robust
 * algorithim for this mapping.
 */
void mmu_init_stage2(void)
{
    pde_t *l1;
    u_int32 va1;
    u_int32 va2;
    u_int32 pa;
    u_int32 va;
    l1 = (pde_t*) (K_PDX_BASE);
	/* map the rest of RAM after the PHYSIZE bytes memory allocated in
	 * mmu_init_stage2. */
    va = KERNBASE + PHYSIZE;
    for(pa = PHYSTART + PHYSIZE; pa < PHYSTART + pm_size; pa += MBYTE) {
        l1[PDX(va)] = pa
                | PDX_ATRB_DOMAIN0
                | PDX_ATRB_AP(PTX_ATRB_KRW)
                | PDX_ATRB_SECTION_ENTRY
                | PTX_ATRB_CACHED
                | PTX_ATRB_BUFFERED;
        va += MBYTE;
    }
	/* Undo identity map of first MB of ram
	 * The identity mapping is done in the assembly routine
	 * pagetable_invalidate in entry.S
	 *
	 * This frees the low memory addresses for use by user
	 * space mappings.
	 */
    l1[PDX(PHYSTART)] = 0;
	/* drain write buffer; writeback data cache range [va, va+n] */
    va1 = (u_int32)&l1[PDX(PHYSTART)];
    va2 = va1 + sizeof(pde_t);
    va1 = va1 & ~((u_int32) CACHE_LINE_SIZE - 1);
    va2 = va2 & ~((u_int32) CACHE_LINE_SIZE - 1);
    flush_dcache(va1, va2);
	// Invalidate TLB, because the identity mapping has been
    // deleted, so cached mappings may not be valid.
    // DSB barrier used. */
    flush_tlb();
}

