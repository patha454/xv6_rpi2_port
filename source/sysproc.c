/**
 * @file sysproc.c
 *
 * sysproc.c provides implementations for the system calls related to
 * process management.
 *
 * @see proc.c for the kernel operations used to implement these
 * system calls.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and styling.)
 *
 * @date 23/11/2018
 */


#include "types.h"
#include "arm.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"


/**
 * Duplicates the current process, creating a new process
 * identical except for PID.
 *
 * @see fork() in proc.c
 *
 * @return 0 to the child process; the new process' PID
 * to the parent.
 */
int sys_fork(void)
{
    return fork();
}


/**
 * Flags the current process as exited, to be freed when
 * it's next scheduled.
 *
 * @see exit() in proc.c
 *
 * @return Nothing. The return statement is never executed.
 */
int sys_exit(void)
{
    exit();
    return 0;  /*< Never reached. */
}


/** Suspend the current process until a child process
 * exits.
 *
 * If the current process has no children, wait will
 * return immediately.
 *
 * @see wait() in proc.c
 *
 * @return The PID of the exiting child, or -1 if no
 * children exist.
 */
int sys_wait(void)
{
    return wait();
}


/**
 * Mark the current process as killed.
 *
 * The process will be terminated the next time it is
 * scheduled, and the PCB de-allocated.
 *
 * @return 0 on success, -1 if the PID does not exist.
 */
int sys_kill(void)
{
    int pid;
    if (argint(0, &pid) < 0) {
        return -1;
    }
    return kill(pid);
}


/**
 * Returns the process ID of the current process.
 *
 * @return The PID of the current process.
 */
int sys_getpid(void)
{
    return curr_proc->pid;
}


/**
 * Increase or decrease the system memory an a set ammount.
 *
 * @see growproc() in proc.c
 *
 * @return The ammount of memory allocated to the process
 * before the resize.
 */
int sys_sbrk(void)
{
    int addr;
    int n;
    if(argint(0, &n) < 0) {
        return -1;
    }
    addr = curr_proc->sz;
    if (growproc(n) < 0) {
        return -1;
    }
    return addr;
}


/** Suspend the current process for n ticks of the system
 * clock.
 *
 * @see sleep() in proc.c
 *
 * @return 0 on success, -1 on failure.
 */
int sys_sleep(void)
{
    int n;
    u_int32 ticks0;
    if (argint(0, &n) < 0) {
        return -1;
    }
    acquire(&ticks_lock);
    ticks0 = ticks;
    /*
     * This was "ticks - ticks0 < n".
     * In C, < has higher precedence than -.
     * Brackets added for clarity.
     */
    while (ticks - (ticks0 < n)){
        if (curr_proc->killed){
            release(&ticks_lock);
            return -1;
        }
        /* sleep will release ticks_lock until awoken. */
        sleep(&ticks, &ticks_lock);
    }
    release(&ticks_lock);
    return 0;
}


/**
 * Returns how many clock tick interrupts have occured
 * since the system was started.
 *
 * @return The number of ticks since the system started.
 */
int sys_uptime(void)
{
    u_int32 xticks;
    acquire(&ticks_lock);
    xticks = ticks;
    release(&ticks_lock);
    return xticks;
}
