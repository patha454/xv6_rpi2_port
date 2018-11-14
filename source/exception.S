/**
 * @file exception.S
 *
 * exception.S provides low-level trap handeling code,
 * such as the interrupt vector, interrupt service routines,
 * and assembly level interrup masking.
 *
 * @todo exception.S also contains some code which is not related
 * to trap handling. Examples include context switching and
 * reading the CPSR. Such code was apparenlty mereged into a
 * single ASM file when xv6 as ported to ARM, and should
 * probably be refactored back to seperated files.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation, styling, and refactoring.)
 *
 * @date 14/10/2018
 */


.align 4
.section .text


/**
 * vectors is the interrupt vector table for the OS.
 *
 * The trap vector is hard coded into the text section. The trap
 * vector is copied into the HVECTORS, where the CPU expects to
 * find it, during initialisation.
 *
 * @see tvinit() in trap.c
 *
 * When a trap is fired, the CPU will branch to one of the eight
 * entries in the interrupt vector. Each of these entries
 * will begin executing the relevant interrupt service routine.
 */
.global vectors
vectors:
    ldr pc, reset_handler       @ Processor reset trap.
    ldr pc, undefintr_handler   @ Undefined instruction trap.
    ldr pc, swi_handler         @ Software interrupt.
    ldr pc, prefetch_handler    @ Prefetch abort trap (Instruction address invalid.)
    ldr pc, data_handler        @ Data abort trap (Data address invalid.)
    ldr pc, unused_handler      @ Reserved by ARM. (No meaning in current architecture.)
    ldr pc, irq_handler         @ Interrupt request.
    ldr pc, fiq_handler         @ Fast interrupt request.


/*
 * Processor reset handler function pointer.
 * This is not implimented and will crash to an infinite loop.
 *
 * More research is requried to determine why.
 */
reset_handler:
    .word hang    /* reset, in svc mode already */

/* Undefined instruction handler function pointer. */
undefintr_handler:
    .word do_und

/* Software interrupt (SWI) and service call (SCI) handler funciton pointer. */
swi_handler:
    .word do_svc

/* Instruction prefetch abort handler funcion pointer. */
prefetch_handler:
    .word do_pabt

/* Data handler abort handler function pointer. */
data_handler:
    .word do_dabt

/* Handler reserved by ARM. This should never be called. */
unused_handler:
    .word hang

/* Interrupt request handler function pointer. */
irq_handler:
    .word do_irq

/* Fast interrupt request handler function pointer. */
fiq_handler:
    .word hang


/**
 * Hang provides a facility to enter an infinite loop in error cases.
 */
hang:
    bl NotOkLoop;
    b hang

/**
 * do_svc (Do Supervisor Call) prepares initiates a
 * system call.
 *
 * do_svc is used for software interrupts (SWI) only. Other traps
 * and interrupts are handled diffrently by switchtosvc, because
 * the CPU must switch from the trap mode to SVC mode.
 *
 * do_svc creates the trap frame by pushing values onto the the stack,
 * and manually adjusting the stack pointer when necessary. Finally,
 * do_svc calls trap in trap.c to execute the system call.
 *
 * @see trap() in trap.c
 */
do_svc:
    push {lr}                   @ Push the link register onto the trap frame.
    mrs lr, spsr                @ Read the SPSR into the CPU.
    push {lr}                   @ Push the SPSR onto the trap frame.
    mrs lr, cpsr                @ Read the CPSR into the CPU.
    push {lr}                   @ Push the CPSR onto the onto the trap frame.
    mrc p15, 0, lr, c6, c0, 2   @ Read Instruction Fault Address Register.
    push {lr}                   @ Push the IFAR onto the trap frame.
    mov lr, #0x40               @ Load the system call trap code.
    push {lr}                   @ Push the trap code onto the trap frame.
    /* Push the general purpose registers onto the trap frame. */
    STMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14}
    sub sp, sp, #60             @ Decrement SP through 15 general purporse registers.
    mov r0, sp                  @ Save a pointer to the trap frame.
    STMFD r0, {r13}^            @ Load the user mode stack pointer.
    mov r1, r1                  @ Three no-ops after saving user mode
    mov r1, r1                  @ ...banked registers. More research
    mov r1, r1                  @ ...is needed to find out  why.
    mov sp, r0                  @ Restore the SP (which didn't it change?)
    sub sp, sp, #4              @ Decrement SP through user SP on the trap frame.
    mov r0, sp                  @ Set the trap frame as an argument.
    bl trap                     @ Call trap in exception.c


/**
 * trapret return from a trap by restoring registers from the trap frame.
 */
.global trapret
trapret:
    mov r0, sp                  @ Save the SVC mode stack pointer.
    LDMFD r0, {r13}^            @ Read user SP (from trap frame) into user mode SP.
    mov r1, r1                  @ Three NOPs after LDMFD.
    mov r1, r1                  @ More research is required into why.
    mov r1, r1                  @ Perhaps to avoid pipelining problems...
    mov sp, r0                  @ Restore the SVC mode stack pointer.
    add sp, sp, #4              @ Decrement the stack past the (popped) user SP.
    /* Pop working registers from the trap frame. */
    LDMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
    add sp, sp, #72             @ Move the SP to the trap frame SPSR
    pop {lr}                    @ Load the pre-trap SPSR from the trap frame,
    msr spsr, lr                @ Restore the SPSR.
    pop {lr}                    @ Load the trap return address from the trap frame,
    movs pc, lr                 @ Resume executing from pre-trap PC.


/**
 * do_und is the undefined instruction trap handler.
 *
 * do_und calls _switchtosvc with the undefined instruction trap
 * code as a parameter.
 */
do_und:
    STMFD sp, {r0-r4}           @ Push callee-save registers onto the stack.
    mov r0, #0x01               @ Set Undef. Inst. trap code as an argument.
    b _switchtosvc              @ Perform a system call.


/**
 * do_pabts is the prefetch abort trap handler.
 *
 * do_pabt calls _switchtosvc with the prefetch abort trap
 * code as a parameter.
 */
do_pabt:
    STMFD sp, {r0-r4}           @ Push callee-save registers onto the stack.
    mov r0, #0x02               @ Set Prefetch Abort trap code as an argument.
    b _switchtosvc              @ Perform a system call.


/**
 * do_dabt is the data abort handler.
 *
 * do_dabt calls _switchtosvc with the data abort trap
 * code as a parameter.
 */
do_dabt:
    STMFD sp, {r0-r4}           @ Push callee-save registers onto the stack.
    mov r0, #0x04               @ Set Data Abort trap code as an argument.
    b _switchtosvc              @ Perform a system call.


/**
 * do_irq is the interrupt request trap handler.
 *
 * do_irq calls _switchtosvc with the interrupt request trap
 * code as a parameter.
 */
do_irq:
    STMFD sp, {r0-r4}           @ Push callee-save registers onto the stack.
    mov r0, #0x80               @ Set IRQ trap code as an argument.
    b _switchtosvc              @ Perform a system call.


/**
 * _switchtosvc handles hardware traps by refering them to the OS
 * system call facility.
 *
 * _switchtosvc switches to SVC mode, sets up the trap frame,
 * and calls calls the OS trap handler with a trap code describing
 * the the trap event.
 *
 * _switchtosvc expects the callee-saved registers (r0-r4) to be
 * saved by the caller on the top of the stack. This is important
 * because _switchtosvc must ultimatly be able to find these
 * registers to save them in the trap frame.
 *
 * @see trap() in trap.c
 *
 * @param r0 - The trap code for the trap to handle.
 */
_switchtosvc:
    mrs r1, spsr                @ Save the SPSR from the trap cause instruction.
    sub r2, lr, #4              @ Save the user mode addres to return to.
    mov r3, sp                  @ Save the stack location of callee-saved registers.
    mrs lr, cpsr                @ Move the CPSR to the working registers.
    bic lr, #0x0000001F         @ Mask Thumb and CPU mode bits in the CPSR.
    orr lr, #0x00000080         @ Disable IRQ interrupts.
    orr lr, #0x00000013         @ Set mode bits to SVC mode.
    msr cpsr, lr                @ Switch to SVC, disable Thumb and IRQ.
    push {r2}                   @ Save the return address to trap frame.
    push {r1}                   @ Save the SPSR to the trap frame.
    mrs r1, cpsr                @ Load the CPSR to the working registers.
    push {r1}                   @ Save the SPSR to the trap frame.
    mrc p15, 0, r1, c6, c0, 2   @ Read Instruction Fault Address Register (IFAR).
    push {r1}                   @ Save the IFAR to the trap frame.
    push {r0}                   @ Save the trap number to the trap frame.
    sub r1, r3, #20             @ Allocate stack space for callee-saved registers.
    LDMFD r1, {r0-r4}           @ Load the callee-saved registers.
    /* Save the general purpose registers to the trap frame. */
    STMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14}
    sub sp, sp, #60             @ Decrement SP past saved GP registers.
    mov r0, sp                  @ Point r0 to the top of the trap frame.
    STMFD r0, {r13}^            @ Save user mode SP to the trap frame.
    mov r1, r1                  @ Three NOPs after STM with user mode banked
    mov r1, r1                  @ ...registers. More research is required to
    mov r1, r1                  @ ...determine why. Perhaps pipeline issues?
    mov sp, r0                  @ Restore the stack pointer.
    sub sp, sp, #4              @ Move SP past the user SP on the trap frame.
    mov r0, sp                  @ Set the trap frame pointer as an argument.
    bl trap                     @ Call trap handler.
    mov r0, sp                  @ Copy SP to the working registers.
    add r0, #76                 @ Point r0 to the SPSR on the trap frame
    LDMIA r0, {r1}              @ Load SPSR into r1
    mov r2, r1                  @ Copy SPSR into r2
    and r2, #0xf                @ Get CPU mode bits
    cmp r2, #0                  @ Test if CPU was in user mode.
    beq _backtouser             @ Return to user if trap was fired from userspace.
    /* Otherwise, this trap was generated while responding to another trap.
     * We will return the CPU to the state it was in before the trap,
     * and continue executing that trap handler. */
    msr cpsr, r1                @ Set CPU state to previous mode-state.
    add sp, #4                  @ Move SP to GP registers in the trap frame.
    /* Restore general purpose registers from the trap frame. */
    LDMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
    add sp, sp, #56             @ Move the SP past the GP regs on the trap frame.
    pop {r14}                   @ Restore the link register from the trap frame.
    add sp, sp, #16             @ Move the SP to the PC on the trap frame.
    pop {pc}                    @ Continue executing from the trap frame PC.


_backtouser:
    mov r0, sp /* save sp in case it is changed to sp_usr after the following LDMFD instruction */
    LDMFD r0, {r13}^ /* restore user mode sp */
    mov r1, r1  /* three nops after LDMFD */
    mov r1, r1
    mov r1, r1
    mov sp, r0  /* restore sp */
    add sp, sp, #4
    LDMIA sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
    add sp, sp, #72
    pop {lr}
    msr spsr, lr
    pop {lr}
    movs pc, lr  /* subs pc,lr,#0 */


/**
 * set_mode_sp is used set the stack pointer for a given CPU mode.
 *
 * @param r0 - The address to use as the new stack pointer.
 * @param r1 - The CPU mode the stack pointer is for, given as a
 *             CPSR mode bit pattern.
 */
.global set_mode_sp
set_mode_sp:
    mrs r2, cpsr            @ Save the old mode and state to the working registers.
    msr cpsr_c,r1           @ Set the new CPU mode, etc, passed in as an argument.
    mov sp, r0              @ Set the new stack pointer, passed in as an argument.
    mrs r0, cpsr            @ Copy the new CPSR to the working registers.
    orr r0, #0x00000080     @ Disable IRQ interrupts.
    orr r0, #0x00000040     @ Disable FIQ interrupts.
    msr cpsr, r0            @ Execute the IRQ and FIQ disable for the given mode.
    msr cpsr_c, r2          @ Return to the mode and state to original.
    bx lr                   @ Return.


/**
 * readcpsr returns the Current Program Status Register.
 */
.global readcpsr
readcpsr:
    mrs r0, cpsr            @ Load the CPSR into the return register.
    bx lr                   @ Return.


/**
 * cli ("Clear Interrupts") disables IRQ interrupts.
 *
 * This will not disable FIQ interrupts.
 */
.global cli
cli:
    mrs r0, cpsr            @ Load the CPSR into the working registers.
    orr r0, #0x00000080     @ Set the mask IRQ bit.
    msr cpsr, r0            @ Set the (IRQ masked) CPSR into the coprocessor.
    bx lr                   @ Return.


/**
 * sti ("Set Interrupts") re-enables IRQ interrupts.
 * sti should be used after a call to 'cli' and not
 * to enable IRQs disabled through the interrpupt
 * control registers.
 *
 * This will not enable FIQ interrupts if they are disabled.
 */
.global sti
sti:
    mrs r0, cpsr            @ Load the CPSR into the working registers.
    bic r0, r0, #0x00000080 @ Clear the IRQ mask bit.
    msr cpsr, r0            @ Load the (IRQ enabled) CPSR into the coprocessor.
    bx lr                   @ Return.


/**
 * swtch ("context Switch") switches to a new process context.
 *
 * swtch saves the current context to the top of the current process'
 * stack, and saves the location of the old stack to a pointer passed
 * in as a parameter.
 *
 * The hardware stack pointer is then set to the new context's stack
 * (also passed as a pointer,) and its context is poped into the
 * current registers from the top of its stack.
 *
 * @see struct context in proc.h for the layout of the context on the stack.
 *
 * @param r0 - Where to store the old context's stack pointer.
 * @param r1 - The new context's stack pointer.
 */
.global swtch
swtch:
    /* These two 'push {lr}'s can be confusing, because the first
     * 'push {lr]' saves into what the struct context calls the 'pc.'
     *
     * This is not a typo. The first variable we push should be the
     * program counter (see struct context and the matched
     * pop instructions later in 'swtch'.) However, when this push
     * is executed, the PC points to that push instruction - so instead,
     * we save the link register (where swtch should return to) into the
     * 'pc' variable on the context.
     *
     * When we eventually switch back to the original context (with
     * another call to 'swtch', the final 'pop {pc}' loads the address
     * the original switch was expected to return to (original LR) and not
     * the first 'push {lr}' in 'swtch' - The latter would be a bug, where
     * rubbish values from the stack would be loaded into the registers
     * (including SP and LR) after a context switch, causing undefined
     * behaviour.
     */
    push {lr}               @ Save where the old context should resume from.
    push {lr}               @ ...Which is also the old context LR. Save this.
    /* Save the old context's callee-save registers. In ARMv6, r0-r3 are
     * used to saved by the caller then used to pass arguments. r13-15 are
     * the SP, LR and PC which are handeled diffrently.
     */
    push {r12}
    push {r11}
    push {r10}
    push {r9}
    push {r8}
    push {r7}
    push {r6}
    push {r5}
    push {r4}
    /* Switch Stacks. */
    str sp, [r0]            @ Save the old context's stack pointer.
    mov sp, r1              @ Set the stack to the new context's SP.
	/* Load the new context's callee-save registers. */
    pop {r4}
    pop {r5}
    pop {r6}
    pop {r7}
    pop {r8}
    pop {r9}
    pop {r10}
    pop {r11}
    pop {r12}
	/* Return to previously saved PC. */
    pop {lr}                @ Pop the link register.
    pop {pc}                @ Begin executing the new context
