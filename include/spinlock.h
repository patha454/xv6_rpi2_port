/**
 * @file spinlock.h
 *
 * spinlock.h provides a synchronization primitive used for concurrency
 * control.
 *
 * A spinlock is a basic locking mechanism which enters a busy-wait loop
 * until the locked resource is available.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and refactoring.)
 *
 * @date 27/10/2018
 */


/**
 * @struct spinlock - A mutual exclusion spinlock.
 *
 * 'spinlock' is a basic locking mechanism. The spinlock thread enters a
 * busy wait loop until a resource is available.
 *
 * Spinlocking avoids the overhead of context-switch or rescheduling
 * processes while waiting, so is efficient when the wait time is likely
 * to be short in task scheduling terms.
 */
struct spinlock {
    /**
     * @var locked - Indicates if the lock is held by another thread.
     * Value is Non-zero if locked, zero otherwise.
     */
    u_int32 locked;

    /** @var name - assigned to the lock for debugging purposes. */
    char *name;

    /** @var cpu - identifies the CPU holding the lock for debugging. */
    struct cpu *cpu;

    /**
     * @var pcs - The call stack (of program counters) which is
     * responsible for acquiring the lock.
     *
     * pcs does not appear to be used in ARM xv6.
     */
    u_int32 pcs[10];
};
