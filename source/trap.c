/**
 * @file trap.c
 *
 * trap.c provides a trap handling system.
 *
 * trap.c provides the trap handling system in ARM xv6, including
 * setting up the trap vector, handling interrupt requests, and
 * enabling or disabling interrupts.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation, styling, and refactoring.)
 *
 * @date 07/11/2018
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
extern u_char8* vectors;


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
void set_mode_sp(char* sp, u_int32 cpsr_c);


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
 * @todo - Move this to timer.c 'ticks_lock' has nothing to do with the
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
    u_int32 disable_all;
    ip = (int_ctrl_regs*) INT_REGS_BASE;
    disable_all = ~((u_int32) 0);
    ip->irq_disable[0] = disable_all;
    ip->irq_disable[1] = disable_all;
    ip->irq_basic_disable = disable_all;
    ip->fiq_control = 0;
}


/**
 * Initializes the trap vectors and associated memory.
 *
 * tv_init ("Trap Vector Initialize") copies the trap vector from the
 * binary text section to where ARM expects to find the trap vector.
 *
 * tv_init will also allocate stack pages, stack pointers, and IRQ/FIQ
 * masks for each CPU mode used by traps in ARM xv6.
 *
 * tv_init also flushes the caches to the memory, because ARM will
 * fetch the trap vector from RAM, not cache, and sets the stack p
 * pointer for each CPU mode used by traps.
 */
void tv_init(void)
{
    u_int32* d;
    u_int32* s;
    /* Initialize the exception vectors */
    d = (u_int32*) HVECTORS;
    s = (u_int32*) &vectors;
    memmove(d, s, sizeof(Vpage0));
    dsb_barrier();
    flush_idcache();
    /* Allocate stacks and stack pointers. */
    /* FIQ mode. FIQ and IRQ will be disabled. */
    init_mode_stack(0xD1);
    /* IRQ Mode. FIQ and IRQ will be disabled. */
    init_mode_stack(0xD2);
    /* Undefined (instruction) mode. FIQ and IRQ are disabled. */
    init_mode_stack(0xDB);
    /* Abort mode. FIR and IRQ will be disabled. */
    init_mode_stack(0xD7);
    /* Secure monitor mode. FIQ and IRQ will be disabled. */
    init_mode_stack(0xD6);
    /* System mode. FIQ and IRQ are disabled. */
    init_mode_stack(0xDF);
}


/**
 * Initialises the stack stack for a CPU mode.
 *
 * A single kernel page of 'PGSIZE' is allocated for the stack, and the
 * mode specific stack pointer is set.
 *
 * The data and instruction caches are flushed to write page changes to the
 * RAM.
 *
 * @param mode - The mode to initialise a stack for, as a CPSR bit pattern.
 */
void init_mode_stack(u_int32 mode)
{
    char* stack = kalloc();
    memset(stack, 0, PGSIZE);
    set_mode_sp(stack + PGSIZE, mode);
    dsb_barrier();
}


/**
 * Prints debugging information about a trap.
 *
 * print_trap prints the trap frame and register values.
 * print_trap is intended for debugging the trap system only, and
 * should be considered a crashing call.
 *
 * @param tf - The trap frame generated when the trap even was fired.
 */
void print_trap(struct trapframe *tf)
{
    cprintf("Trap debugging info:\n");
    cprintf("cpu: %x, addr: %x, pid: %x, trapno: %x, spsr: %x, sp: %x, pc: %x cpsr: %x ifar: %x\n", curr_cpu->id, tf->pc, tf->trapno, tf->spsr, tf->sp, tf->pc, tf->cpsr, tf->ifar);
    cprintf("Saved registers: r0: %x, r1: %x, r2: %x, r3: %x, r4: %x, r5: %x\n", tf->r0, tf->r1, tf->r2, tf->r3, tf->r4, tf->r5);
    cprintf("More registers: r6: %x, r7: %x, r8: %x, r9: %x, r10: %x, r11: %x, r12: %x\n", tf->r6, tf->r7, tf->r8, tf->r9, tf->r10, tf->r11, tf->r12);
    //not_ok_loop();
}


/**
 * Handles IRQ interrupt requests by calling the appropriate handlers.
 *
 * handle_irq enters a handle loop until all out standing IRQ requests have
 * been satisfied.
 *
 * handle_irq recognises the following IRQ sources:
 * - mini-UART.
 * - System timer.
 *
 * @warning If an IRQ if fired from an unrecognised source, handle_irq
 * will enter an infinite loop. Take care only to enable IRQs which are
 * recognised.
 *
 * @note The irq_pending registers contain a number of undocumented IRQ signals
 * related to GPU function - could these force handle_irq into an infinite loop?
 *
 * @todo - add a default case to handel unrecognised interrupts.
 * @param tf - the trap frame generated when the IRQ was fired.
 * @param is_timer_irq - Used to communicate to the caller if the timer
 *                       fired the IRQ.
 */
void handle_irq(struct trapframe* tf, u_int32* is_timer_irq)
{
    int_ctrl_regs* ip;
    ip = (int_ctrl_regs*) INT_REGS_BASE;
    while(ip->irq_pending[0] || ip->irq_pending[1] || ip->irq_basic_pending){
        if(ip->irq_pending[0] & (1 << IRQ_TIMER_BIT)) {
            timer3intr();
            *is_timer_irq = 1;
	    }
        if(ip->irq_pending[0] & (1 << IRQ_MINIUART_BIT)) {
		    miniuartintr();
        }
	}
}


/**
 * Handles system calls by forwarding control to the system call routines.
 *
 * @param tf - The trap frame generated when the system call was fired.
 */
inline void handle_syscall(struct trapframe* tf)
{
    if(curr_proc->killed) {
        exit();
    }
    curr_proc->tf = tf;
    syscall();
    if(curr_proc->killed) {
        exit();
    }
    return;
}


/**
 * Handels unexpected traps by printing error information.
 *
 * handle_bad_trap will terminate a user process throwing in unrecognised trap,
 * or panic is the bad trap was fired by the kernel.
 *
 * @param tf - The trap frame generated when the bad trap was fired.
 */
void handle_bad_trap(struct trapframe *tf)
{
    if(curr_proc == 0 || (tf->spsr & 0xF) != PSR_USER_MODE){
        // In kernel, it must be our mistake.
        cprintf("Unexpected trap from kernel space.\n");
        print_trap(tf);
        panic("trap");
    } else {
        // In user space, assume process misbehaved.
        cprintf("Unexpected trap from user space.\n");
        print_trap(tf);
        curr_proc->killed = 1;
    }
}


/**
 * Recieves traps from the hardware, and calls appropriate handlers.
 *
 * trap is called by the trap vectors when the hardware fires a trap.
 *
 * @see exception.S
 *
 * @param tf  - The trap frame generated when the trap was fired.
 */
void trap(struct trapframe *tf)
{
    u_int32 is_timer_irq;
    if(tf->trapno == T_SYSCALL){
        handle_syscall(tf);
        return;
    }
    is_timer_irq = 0;
    switch(tf->trapno){
        case T_IRQ:
	        handle_irq(tf, &is_timer_irq);
	        break;
        default:
            handle_bad_trap(tf);
    }
    /* Force process exit if it has been killed and is in user space.
     * (If it is still executing in the kernel, let it keep running
     * until it gets to the regular system call return.) */
    if(curr_proc){
        if(curr_proc->killed && (tf->spsr&0xF) == PSR_USER_MODE) {
            exit();
        }
    /* Force process to give up CPU on clock tick.
     * If interrupts were on while locks held, would need to check nlock. */
        if(curr_proc->state == RUNNING && is_timer_irq) {
            yield();
        }
    /* Check if the process has been killed since we yielded. */
        if(curr_proc->killed && (tf->spsr&0xF) == PSR_USER_MODE) {
            exit();
        }
    }
}

