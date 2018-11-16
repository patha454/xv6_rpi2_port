/**
 * @file spinlock.c
 *
 * spinlock.c proves a synchronization primitive used for concurrency
 * control.
 *
 * A spinlock is a basic locking mechanism which enters a busy-wait loop
 * until the locked resource is available.
 *
 * @warning This implementation works by disabling interrupts while the
 * critical section is execution. This is a bad technique on so many levels:
 * Firstly, this is a naive implementation because interrupts are disabled
 * while the critical section is executing: The kernel cannot be certain
 * control of the CPU will be relinquished.
 * Secondly, this approach is not suitable for multi-processing because
 * interrupts are disabled on a per-core/per-cpu basis. Another CPU/core
 * could race the lock, leading to two threads holding the lock.
 *
 * @todo Reimplement locking using ARM atomic test & set instructions.
 * This will be required for a multiprocessing implementation.
 * https://web.cs.du.edu/~cag/courses/CS/StallingsOS3e/lec-7.pdf is a
 * useful resource.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and refactoring.)
 *
 * @date 27/10/2018
 */


#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"


/**
 * Initialises a lock.
 *
 * initlock ("Initialise Lock") initialises a lock passed by pointer.
 * 'initlock' sets up sensible initial values which the locking
 * functions expect.
 *
 * @param lk - A pointer to the lock to initialise.
 * @param name - Optional lock name for debugging. (NULL is acceptable.)
 */
void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}


/**
 * Acquires a lock.
 *
 * 'acquire' will loop (or "spin") until the lock can be acquired.
 *
 * 'acquire' is a blocking function, so holding a lock for a long time
 * may cause other CPUs or threads to waste time spinning to acquire
 * the lock.
 *
 * @warning 'acquire' will disable interrupts until the lock is released.
 * Critical sections should be kept short and be guaranteed to exit.
 *
 * @note - The spin part of this spinlock appears to be missing. Bugged?
 *
 * @param lk - The lock to acquire.
 */
void acquire(struct spinlock *lk)
{
    /* 'pushcli' disables interrupts to avoid deadlock and data races. */
    pushcli();
     if(holding(lk)){
        cprintf("lock name: %s, locked: %d, cpu: %x CPSR: %x\n", lk->name, lk->locked, lk->cpu, readcpsr());
        panic("acquire");
    }
    lk->locked = 1;
    /* Record info about lock acquisition for debugging. */
    lk->cpu = curr_cpu;
}


/**
 * Releases a lock.
 *
 * 'release' will release a lock and pop the disable interrupt counter.
 *
 * 'release' will re-enable interrupts if no other locks are held system wide.
 *
 * @param lk - The lock to release.
 */
void release(struct spinlock *lk)
{
    if(!holding(lk)) {
        panic("release");
    }
    lk->pcs[0] = 0;
    lk->cpu = 0;
    lk->locked = 0;
    popcli();
}


/**
 * Records the current call stack and base pointer chain.
 *
 * getcallerpcs ("Get Caller Process Call Stack") should record the curent
 * call stack in pcs, followed by the chain of base pointers.
 *
 * @warning getcallerpcs is not implemented in ARM xv6.
 *
 * @param v - Unused: Apparently an index relative to the base pointer.
 * @param pcs - Unused: Memory to return the process call stack in,
 */
void getcallerpcs(void *v, u_int32 pcs[])
{
}


/**
 * Checks if the current CPU holds the lock.
 *
 * 'holding' tests if the lock is held by the current CPU.
 * 'holding' will return false if another CPU holds the lock.
 *
 * @param lock - Test if this lock is held by the current CPU.
 * @return - Non-zero if the lock is held by the current CPU, zero otherwise.
 */
int holding(struct spinlock *lock)
{
    int rv;
    rv = lock->locked && lock->cpu == curr_cpu;
    /*
    if(rv){
        cprintf("The held lock: %s, locked: %d, cpu: %x\n", lock->name, lock->locked, lock->cpu);
    }
    */
    return rv;
}


/**
 * Increments the interrupt mask counter, disabling interrupts.
 *
 * 'pushcli' ("Push Clear Interrupts") works like 'cli,' except
 * 'pushcli' is matched to 'popcli.' If n calls are made to
 * 'pushcli', n calls must be made to 'popcli,' to re-enable
 * interrupts.
 *
 * 'pushcli' saves the state if CPSR IRQ disable bit, so if
 * interrupts were already disabled pushcli(); popcli() will
 * not re-enable interrupts.
 */
void pushcli(void)
{
    u_int32 cpsr;
    cpsr = readcpsr();
    cli();
    if(curr_cpu->ncli++ == 0) {
        curr_cpu->irq_enabled = (cpsr & PSR_DISABLE_IRQ) ? 0 : 1;
    }
}


/**
 * Decrements the interrupt mask counter, potentially enabling interrupts.
 *
 * 'popcli' (Pop Clear Interrupts) works like 'sti', except 'popcli' is
 * paired to 'pushcli.' To re-enable interrupts, n calls to 'popcli' must
 * be made if n calls were made to 'pushcli' previously.
 */
void popcli(void) {
    if (!(readcpsr() & PSR_DISABLE_IRQ)) {
        panic("popcli - interruptible");
    }
    if (--curr_cpu->ncli < 0) {
        panic("popcli");
    }
    if (curr_cpu->ncli == 0 && curr_cpu->irq_enabled) {
        sti();
    }
}
