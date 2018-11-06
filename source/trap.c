/**
 * @file trap.c
 *
 * trap.c provides a trap handling system.
 *
 * trap.c provides the trap handling system in ARM xv6, including
 * setting up the trap vector, handling interrupt requests, and
 * enabling or disabling interrupts.
 *
 *  @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 *  @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation, styling, and refactoring.)
 */


#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"
#include "traps.h"
#include "spinlock.h"


/**
 * 'vectors' is a pointer to the trap vector in the assembly data segment.
 * @see exception.s
 */
extern u_char8 *vectors;


/**
 * set_mode_sp ('Set CPU mode and stack pointer") sets the stack pointer
 * for a given CPU mode.
 *
 * ARM requires the OS to initialize the stack pointer (SP) for each
 * CPU mode which will be used. set_mode_sp provides a mechanisms
 * to initialize (or change) the stack pointer for each mode.
 *
 * set_mode_sp is implemented in assembly.
 * @see exception.s
 *
 * @warning set_mode_sp will disable IRQ and FIQ for each mode the
 * function is called on.
 *
 * @param sp - The address to use as the new stack pointer.
 * @param cpsr_c - The mode to set the SP for, encoded as the CPSR mode field. *
 */
void set_mode_sp(char * sp, u_int32 cpsr_c);


/**
 * The number of times the system clock has 'ticked.'
 *
 * @todo - Move this to timer.c. 'ticks' has nothing to do with the
 * trap implementation.
 */
u_int32 ticks;


/**
 * A lock of the system clock variable 'ticks'.
 *
 * @todo _ Move this to timer.c 'ticks_lock' has nothing to do with the
 * trap implementation.
 */
struct spinlock ticks_lock;


/**
 * Enables interrupts from selected sources.
 *
 * enable_inters enables interrupts from the following sources:
 * - Mini UART
 * - System clock.
 */
void enable_intrs(void)
{
    int_ctrl_regs* ip = (int_ctrl_regs*) INT_REGS_BASE;
    /* Enable the mini UART throuh the (SPI?) Aux. */
    ip->irq_enable[0] |= 1 << 29;
    /* Enable UART IRQ. */
    //ip->irq_enable[1] |= 1 << 25;
    /* Enable the system clock IRQ. */
    ip->irq_basic_enable |= 1 << 0;
}


/**
 * Disables interrupts from all sources.
 *
 * disable_intrs disables both IRQ and FIQ interrupts from all sources
 * and devices.
 */
void disable_intrs(void)
{
    int_ctrl_regs *ip;
    u_int32 disable;
    ip = (int_ctrl_regs *)INT_REGS_BASE;
    disable = (u_int32) 0;
    ip->irq_disable[0] = disable;
    ip->irq_disable[1] = disable;
    ip->irq_basic_disable = disable;
    ip->fiq_control = 0;
}


void tvinit(void)
{
	u_int32 *d, *s;
	char *ptr;

	/* initialize the exception vectors */
	d = (u_int32 *)HVECTORS;
	s = (u_int32 *)&vectors;
	memmove(d, s, sizeof(Vpage0));

	/* cacheuwbinv(); drain write buffer and prefetch buffer
	 * writeback and invalidate data cache
	 * invalidate instruction cache
	 */
	dsb_barrier();
	flush_idcache();
	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xD1);/* fiq mode, fiq and irq are disabled */

	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xD2);/* irq mode, fiq and irq are disabled */
	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xDB);/* undefined mode, fiq and irq are disabled */
	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xD7);/*  abort mode, fiq and irq are disabled */
	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xD6);/* secure monitor mode, fiq and irq are disabled */
	ptr = kalloc();
	memset(ptr, 0, PGSIZE);
	set_mode_sp(ptr+4096, 0xDF);/* system mode, fiq and irq are disabled */

	dsb_barrier();
}

void trap_oops(struct trapframe *tf)
{

cprintf("trapno: %x, spsr: %x, sp: %x, pc: %x cpsr: %x ifar: %x\n", tf->trapno, tf->spsr, tf->sp, tf->pc, tf->cpsr, tf->ifar);
cprintf("Saved registers: r0: %x, r1: %x, r2: %x, r3: %x, r4: %x, r5: %x\n", tf->r0, tf->r1, tf->r2, tf->r3, tf->r4, tf->r5);
cprintf("More registers: r6: %x, r7: %x, r8: %x, r9: %x, r10: %x, r11: %x, r12: %x\n", tf->r6, tf->r7, tf->r8, tf->r9, tf->r10, tf->r11, tf->r12);

//NotOkLoop();
}

void handle_irq(struct trapframe *tf)
{
	int_ctrl_regs *ip;

/*cprintf("trapno: %x, spsr: %x, sp: %x, lr: %x cpsr: %x ifar: %x\n", tf->trapno, tf->spsr, tf->sp, tf->pc, tf->cpsr, tf->ifar);
cprintf("Saved registers: r0: %x, r1: %x, r2: %x, r3: %x, r4: %x, r5: %x, r6: %x\n", tf->r0, tf->r1, tf->r2, tf->r3, tf->r4, tf->r5, tf->r6);
cprintf("More registers: r6: %x, r7: %x, r8: %x, r9: %x, r10: %x, r11: %x, r12: %x, r13: %x, r14: %x\n", tf->r7, tf->r8, tf->r9, tf->r10, tf->r11, tf->r12, tf->r13, tf->r14);
*/
	ip = (int_ctrl_regs *)INT_REGS_BASE;
	while(ip->irq_pending[0] || ip->irq_pending[1] || ip->irq_basic_pending){
	    if(ip->irq_pending[0] & (1 << 3)) {
		timer3intr();
	    }
	    if(ip->irq_pending[0] & (1 << 29)) {
		miniuartintr();
	    }
	}

}


//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
	int_ctrl_regs *ip;
	u_int32 istimer;

//cprintf("Trap %d from cpu %d eip %x (cr2=0x%x)\n",
//              tf->trapno, curr_cpu->id, tf->eip, 0);
  //trap_oops(tf);
  if(tf->trapno == T_SYSCALL){
    if(curr_proc->killed)
      exit();
    curr_proc->tf = tf;
    syscall();
    if(curr_proc->killed)
      exit();
    return;
  }

  istimer = 0;
  switch(tf->trapno){
  case T_IRQ:
	ip = (int_ctrl_regs *)INT_REGS_BASE;
	while(ip->irq_pending[0] || ip->irq_pending[1] || ip->irq_basic_pending){
	    if(ip->irq_pending[0] & (1 << IRQ_TIMER3)) {
		istimer = 1;
		timer3intr();
	    }
	    if(ip->irq_pending[0] & (1 << IRQ_MINIUART)) {
		miniuartintr();
	    }
	}

	break;
  default:
    if(curr_proc == 0 || (tf->spsr & 0xF) != USER_MODE){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d addr %x spsr %x cpsr %x ifar %x\n",
              tf->trapno, curr_cpu->id, tf->pc, tf->spsr, tf->cpsr, tf->ifar);
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d on cpu %d "
            "addr 0x%x spsr 0x%x cpsr 0x%x ifar 0x%x--kill proc\n",
            curr_proc->pid, curr_proc->name, tf->trapno, curr_cpu->id, tf->pc,
            tf->spsr, tf->cpsr, tf->ifar);
    curr_proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)

//cprintf("Proc pointer: %d\n", curr_proc);
  if(curr_proc){
        if(curr_proc->killed && (tf->spsr&0xF) == USER_MODE)
                exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
        if(curr_proc->state == RUNNING && istimer)
                yield();

  // Check if the process has been killed since we yielded
        if(curr_proc->killed && (tf->spsr&0xF) == USER_MODE)
                exit();
  }

//cprintf("Proc pointer: %d after\n", curr_proc);

}

