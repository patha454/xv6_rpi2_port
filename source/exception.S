/*****************************************************************
*       exception.s
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/


.align 4
.section .text

.global vectors
vectors:
	ldr pc, reset_handler 
	ldr pc, undefintr_handler
	ldr pc, swi_handler
	ldr pc, prefetch_handler
	ldr pc, data_handler
	ldr pc, unused_handler
	ldr pc, irq_handler
	ldr pc, fiq_handler
reset_handler:
	.word hang    /* reset, in svc mode already */
undefintr_handler:
	.word do_und      /* undefined instruction */
swi_handler:
	.word do_svc    /* SWI & SVC */
prefetch_handler:
	.word do_pabt      /* prefetch abort */
data_handler:
	.word do_dabt     /* data abort */
unused_handler:
	.word hang      /* reserved */
irq_handler:
	.word do_irq    /* IRQ */
fiq_handler:
	.word hang    /* FIQ */

hang:
        bl NotOkLoop;
	b hang
do_svc:
	push {lr}
        mrs lr, spsr
        push {lr}
        mrs lr, cpsr
        push {lr}
        mrc p15, 0, lr, c6, c0, 2  /* read Instruction Fault Address Register (IFAR) */
        push {lr}
        mov lr, #0x40
        push {lr}
	STMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14}
	sub sp, sp, #60
	mov r0, sp   /* save sp */
	STMFD r0, {r13}^   /* save user mode sp */
	mov r1, r1 /* three nops after STM with user mode banked registers */
	mov r1, r1
	mov r1, r1
	mov sp, r0  /* restore sp */
	sub sp, sp, #4
	mov r0, sp
	bl trap

.global trapret
trapret:
	mov r0, sp /* save sp in case it is changed to sp_usr after the following LDMFD instruction */
	LDMFD r0, {r13}^ /* restore user mode sp */
	mov r1, r1  /* three nops after LDMFD */
	mov r1, r1
	mov r1, r1
	mov sp, r0  /* restore sp */
	add sp, sp, #4
	LDMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
	add sp, sp, #72
	pop {lr}
        msr spsr, lr
	pop {lr}
	movs pc, lr  /* subs pc,lr,#0 */

do_und:
	STMFD sp, {r0-r4}
	mov r0, #0x01
	b _switchtosvc

do_pabt:
	STMFD sp, {r0-r4}
	mov r0, #0x02
	b _switchtosvc

do_dabt:
	STMFD sp, {r0-r4}
	mov r0, #0x04
	b _switchtosvc

do_irq:
	STMFD sp, {r0-r4}
	mov r0, #0x80
	b _switchtosvc
_switchtosvc:
	mrs r1, spsr
	sub r2, lr, #4
	mov r3, sp
	mrs lr, cpsr
	bic lr, #0x0000001F /* PSR_MASK */
	orr lr, #0x00000080 /* PSR_DISABLE_IRQ */
	orr lr, #0x00000013 /* PSR_MODE_SVC */
	msr cpsr, lr /* switch to svc */
	push {r2}
	push {r1}
	mrs r1, cpsr
	push {r1}
        mrc p15, 0, r1, c6, c0, 2  /* read Instruction Fault Address Register (I
FAR) */
        push {r1}
        push {r0}
	sub r1, r3, #20
	LDMFD r1, {r0-r4}
	STMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14}
	sub sp, sp, #60
        mov r0, sp   /* save sp */
        STMFD r0, {r13}^    /* save user mode sp */
        mov r1, r1 /* three nops after STM with user mode banked registers */
        mov r1, r1
        mov r1, r1
        mov sp, r0  /* restore sp */
        sub sp, sp, #4
        mov r0, sp

	bl trap

	mov r0, sp
	add r0, #76
	LDMIA r0, {r1}
	mov r2, r1
	and r2, #0xf
	cmp r2, #0
	beq _backtouser
	msr cpsr, r1
        add sp, #4
	LDMFD sp, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
	add sp, sp, #56
	pop {r14}
	add sp, sp, #16
	pop {pc}

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
 * THis will not disable FIQ interrupts.
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



.global swtch
swtch:
	push {lr}  /* save the return address */
	push {lr}
	/* save old callee-save registers */
	push {r12}
	push {r11}
	push {r10}
	push {r9}
	push {r8}
	push {r7}
	push {r6}
	push {r5}
	push {r4}

	/* switch stacks */
	str sp, [r0]
	mov sp, r1

	/* load new callee-save registers */
	pop {r4}
	pop {r5}
	pop {r6}
	pop {r7}
	pop {r8}
	pop {r9}
	pop {r10}
	pop {r11}
	pop {r12}

	/* return to previously saved pc */
	pop {lr}
	pop {pc}

