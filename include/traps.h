/**
 * @file traps.h
 *
 * traps.h provides declarations used in the trap handling system.
 *
 * The trap handling system initializes the trap vector, handles
 * interrupt requests, and provides for enabling and disabling
 * interrupts.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation, styling, and refactoring.)
 */


/*
 * The trap codes are arbitrarily chosen, but with care not to
 * overlap with processor defined exceptions or interrupt vectors.
 */

/** The system call trap code. */
#define T_SYSCALL   0x40

/** The interrupt trap code. */
#define T_IRQ       0x80

/** The undefined instruction trap code. */
#define T_UND       0x01

/** The prefetch abort trap code. */
#define T_PABT      0x02

/** The data abort trap code. */
#define T_DABT      0x04


/** The system timer bit in irq_pending register 0. */
#define IRQ_TIMER_BIT       3

/** The miniUART bit in irq_pending register 0. */
#define IRQ_MINIUART_BIT    29


/** The virtual address of the interrupt control registers. */
#define INT_REGS_BASE 	(MMIO_VA+0xB200)
