/**
 * @file proc.c
 *
 * proc.c provides process handling mechanisms used by the kernel.
 *
 *
 * proc.c provides the mechanisms to create, kill, and manipulate
 * processes in the operating system. Higher level system calls
 * such as sys_fork and sys_yeild are build on top of the kernel
 * level functions is proc.c. proc.c also manages the lower
 * level aspects such as regulating access to the process table
 * with locks.
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
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "arm.h"
#include "proc.h"
#include "spinlock.h"


/**
 * @struct ptable - The process table.
 *
 * ptable ("Process Table") describes all the processes
 * managed by the kernel, and provides a lock for
 * synchronising access to the table.
 *
 * The page table contains NPROC processes, the maximum
 * number of processes allowed by the system configuration.
 * ptable can contain many unused processes.
 */
struct {
    struct spinlock lock;       /**< Lock to synchronize ptable. */
    struct proc proc[NPROC];    /**< All system processes. */
} ptable;


/**
 * A local pointer to the initial process.
 *
 * init_proc ("Initial Process") is a reference to the
 * first userspace process.
 *
 * initproc should continue running for as long as
 * the operating system is active.
 */
static struct proc* init_proc;


/** Indicates if scheduler() has not been called previously. */
int first_sched = 1;


/**
 * The next process ID free for assignment.
 *
 * next_pid indicates the next PID number avalible to be
 * assigned to a process.
 *
 * In xv6, next_pid is generated from an integer
 * incremented each time a PID is assigned. This will
 * not recycle the PIDs of processes which exit, and
 * the overflow could cause two processes to have the
 * same PID eventually. However, for a simple, small
 * system, this approach is acceptable.
 */
int next_pid = 1;


extern void fork_return(void);
extern void trapret(void);

static void wakeup_1(void *chan);
static struct proc* spawn_proc(struct proc *p);


/**
 * Initialises the process table.
 *
 * pinit ("Process initialise") initialises the process
 * management system by clearing the process table and
 * initialising the process table lock.
 */
void pinit(void)
{
    memset(&ptable, 0, sizeof(ptable));
    initlock(&ptable.lock, "ptable");
}


/**
 * Attempts to allocate a new process in ptable.
 *
 * allocproc ("Allocate Process") attempts to create a new
 * process by searching the page table for an unused
 * process table entry, and initializing the state required
 * for the new process to run in kernel space.
 *
 * @return A pointer to the new process, or 0 on failure.
 */
static struct proc* allocproc(void)
{
    struct proc* p;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED) {
            return spawn_proc(p);
        }
    }
    release(&ptable.lock);
    return 0;
}


/**
 * Initialises a new process' state.
 *
 * spawn_proc ("Spawn Process") sets up an embryo process
 * with the state required to run in kernel space (p->
 * context->pc = forkret) and begin executing in user space
 * (p->context->lr = trapret).
 *
 * spawn_proc expects the calling function to already hold
 * the page table lock, which spawn_proc will release.
 *
 * @todo Refactor the locking here. The side effects
 * suggests bad style.
 *
 * @param p - A UNUSED process to initalize, from the ptable.
 * @return A pointer to the new process, or 0 on failure.
 */
static struct proc* spawn_proc(struct proc *p)
{
    char* sp;
    p->state = EMBRYO;
    p->pid = next_pid++;
    release(&ptable.lock);
    /* Allocate a kernel stack for the process. */
    if ((p->kstack = kalloc()) == 0){
        p->state = UNUSED;
        return 0;
    }
    memset(p->kstack, 0, PGSIZE);
    sp = p->kstack + KSTACKSIZE;
    /* Allocate stack space for the trap frame. */
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe*)sp;
    /* Set up new context to start executing at fork_return,
     * which returns to trapret. */
    sp -= sizeof *p->context;
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->pc = (u_int32) fork_return;
    p->context->lr = (u_int32) trapret;
    return p;
}


/**
 * Set up the first user process.
 *
 * userinit ("User process Initialise") creates the first
 * user-space process.
 *
 * The procedure to create the first process is:
 *  - Allocate a process in the process table.
 *  - Allocate kernel virtual memory.
 *  - Map the initcode executable into the user virtual memory.
 *  - Configure the PCB for a swtch() into the process.
 *  - Mark the process runnable.
 *
 *  The process begins executing uprogs/initcode.S, which in
 *  bootstraps into executing uprogs/init.c. uprogs/init.c forks
 *  and executes a shell.
 *
 * @see uprogs/initcode.S.
 *
 * @see entry.S, which includes the initcode executable at
 * the label _binary_initcode_start.
 */
void userinit(void)
{
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_end[];
    u_int32 _binary_initcode_size;
    _binary_initcode_size = (u_int32) _binary_initcode_end - (u_int32) _binary_initcode_start;
     p = allocproc();
    init_proc = p;
    if ((p->pgdir = setupkvm()) == 0) {
        panic("userinit: out of memory?");
    }
    inituvm(p->pgdir, _binary_initcode_start, _binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->spsr = 0x10;
    p->tf->sp = PGSIZE;
    p->tf->pc = 0;   /*< beginning of initcode.S */
    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");
    p->state = RUNNABLE;
}



/**
 * Change the current process's memory by n bytes.
 *
 * growproc ("Grow Process Memory") allows us to
 * increase or decrease the amount of virtual memory
 * allocated and mapped into the current process.
 *
 * @param n - Expand the memory by 'n' bytes (or
 *            decrease with a negative 'n'.
 *
 * @return 0 on success, -1 on failure.
 */
int growproc(int n)
{
    u_int32 sz;
    sz = curr_proc->sz;
    if (n > 0){
        if ((sz = allocuvm(curr_proc->pgdir, sz, sz + n)) == 0) {
            return -1;
        }
    } else if (n < 0) {
        if ((sz = deallocuvm(curr_proc->pgdir, sz, sz + n)) == 0) {
            return -1;
        }
    }
    curr_proc->sz = sz;
    switchuvm(curr_proc);
    return 0;
}


/**
 * Creates a new process as a duplicate of the parent.
 *
 * The new process will duplicate the current process'
 * memory and state, except the child will have a
 * unique PID. The new process will become the child of the
 * calling process.
 *
 * fork() will set up the new process' stack to return as
 * if from a system call.
 *
 * @note According to MIT documentation, the "caller must set
 * state of returned proc to RUNNABLE." However, the
 * child (returned) process is clearly set to runnable
 * in fork(). I am unsure if MIT's documentation is
 * out of date, as their (x86) code also sets
 * new_proc->state = RUNNABLE. More study is required to
 * understand this. I suggest the user obey this instruction
 * to set the returned process' state unless they have
 * further information on the topic.
 *
 * @return 0 to the child process; Child's PID to the parent.
 */
int fork(void)
{
    int i, pid;
    struct proc* new_proc;
    /* Allocate process. */
    if ((new_proc = allocproc()) == 0) {
        return -1;
    }
    /* Copy process state from p. */
    if ((new_proc->pgdir = copyuvm(curr_proc->pgdir, curr_proc->sz)) == 0){
        kfree(new_proc->kstack);
        new_proc->kstack = 0;
        new_proc->state = UNUSED;
        return -1;
    }
    new_proc->sz = curr_proc->sz;
    new_proc->parent = curr_proc;
    *new_proc->tf = *curr_proc->tf;
    /* Clear r0 on the trap frame so the child will return
     * zero when run and switched to user space. */
    new_proc->tf->r0 = 0;
    for (i = 0; i < NOFILE; i++) {
        if (curr_proc->ofile[i]) {
            new_proc->ofile[i] = filedup(curr_proc->ofile[i]);
        }
    }
    new_proc->cwd = idup(curr_proc->cwd);
    pid = new_proc->pid;
    new_proc->state = RUNNABLE;
    safestrcpy(new_proc->name, curr_proc->name, sizeof(curr_proc->name));
    return pid;
}


/**
 * Exit the current process.
 *
 * exit terminates the current process by:
 *  - Closing all files opened by the process.
 *  - Close the inode used by the process.
 *  - Wakeup the parent, if it's sleeping.
 *  - Re-parent child processes to the initial process.
 *  - Set the process state to ZOMBIE, never to be
 *  executed again.
 *  - Enter the scheduler to start running another
 *  process.
 *
 *  exit effectively never returns.
 *
 *  The zombie process will be cleaned up when the
 *  parent process calls wait().
 */
void
exit(void)
{
    struct proc *p;
    int fd;
    if (curr_proc == init_proc) {
        panic("init exiting");
    }
    /* Close all open files. */
    for (fd = 0; fd < NOFILE; fd++){
        if (curr_proc->ofile[fd]){
            fileclose(curr_proc->ofile[fd]);
            curr_proc->ofile[fd] = 0;
         }
    }
    /* Free the inode used by the process. */
    iput(curr_proc->cwd);
    curr_proc->cwd = 0;
    acquire(&ptable.lock);
    /* Wakeup the parent, if parent is wait()int. */
    wakeup_1(curr_proc->parent);
    /* Pass the abandoned childred to the init process. */
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->parent == curr_proc){
            p->parent = init_proc;
            if (p->state == ZOMBIE) {
                wakeup_1(init_proc);
            }
        }
    }
    /* Jump into the scheduler, never to return. */
    curr_proc->state = ZOMBIE;
    sched();
    panic("zombie exit");
}


/**
 * Wait for a child process to exit, and return its PID.
 *
 * wait waits until a child process has exited, de-allocates
 * the child's process control block, and return the terminated
 * child's PID.
 *
 * @note Sleep will only return when the process has no more
 * child processes. If the process has no exited (ZOMBIE)
 * child processes, the process will sleep until a child
 * exits.
 *
 * @return The PID of a child which terminated, or -1 if the
 * process has no children.
 */
int wait(void)
{
    struct proc* p;
    int has_children, pid;
    acquire(&ptable.lock);
    for (;;) {
        /* Scan through table looking for zombie children. */
        has_children = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if (p->parent != curr_proc) {
                continue;
            }
            /* De-allocate the zombie child and return. */
            has_children = 1;
            if(p->state == ZOMBIE){
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->state = UNUSED;
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                release(&ptable.lock);
                return pid;
            }
        }
        /* No point waiting if we don't have any children. */
        if (!has_children || curr_proc->killed){
            release(&ptable.lock);
            return -1;
        }
        /* Release the lock and do not execute until a child exits.
         * See wakeup_1() call in exit, which wakes the parent when
         * a child terminates. */
        sleep(curr_proc, &ptable.lock);
    }
}


/**
 * Schedule a process for the current CPU.
 *
 * scheduler() is executed by the per-cpu scheduler
 * context. (&curr_cpu->scheduler.) Scheduler selects a
 * process to run, and switches to it in an endless loop.
 * Eventually the running process will return control to
 * the scheduler loop, either voluntarily or via a timer
 * interrupt, so the infinite scheduler loop will continue.
 *
 * Scheduler never returns. The loop is:
 *  - Choose a process to run.
 *  - swtch() to the process to run it.
 *  - When the process returns control to
 *  scheduler, repeat.
 *
 * @note Each CPU should call scheduler() after setting
 * itself up.
 *
 * @note Processes are responsible for releasing ptable.lock
 * when they are switched to, and re-acquiring it before
 * the swtch back to the scheduler, as well as changing
 * their state to and from RUNNABLE. Normally this would be
 * done by sleep() and yield() in the course of normal
 * operation.
 */
void scheduler(void)
{
    struct proc *p;
    for (;;) {
        /* Enable interrupts on this processor.
         * If this is the first call to scheduler,
         * IRQ was not disabled, unlike later calls
         * via the interrupt system. */
        if (first_sched) {
            first_sched = 0;
        } else {
            sti();
        }
        /* Loop over process table looking for process to run. */
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if (p->state != RUNNABLE) {
                continue;
            }
            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            curr_proc = p;
            switchuvm(p);
            p->state = RUNNING;
            swtch(&curr_cpu->scheduler, curr_proc->context);
            /* The context will switch back here after the
             * process is suspended running. */
            switchkvm();
            curr_proc = 0;
        }
        release(&ptable.lock);
    }
}


/**
 * Enter the scheduler for the current CPU.
 *
 * sched ("Schedule") context switches to the scheduler for
 * the current CPU. The process should hold ptable.lock,
 * and also hold no other locks, to prevent deadlocks.
 *
 * The calling process is also responsible for changing
 * it's process state from RUNNING to an appropriate state
 * such as RUNNABLE. Normally, both the locking and state
 * changing would be handled by yield() and sleep() in the
 * cause of both forced and voluntarily suspension of
 * execution.
 */
void sched(void)
{
    int irq_enabled;
    if (!holding(&ptable.lock)) {
        panic("sched ptable.lock");
    }
    if (curr_cpu->ncli != 1) {
        panic("sched locks");
    }
    if (curr_proc->state == RUNNING) {
        panic("sched running");
    }
    if (!(readcpsr() & PSR_DISABLE_IRQ)) {
        panic("sched interruptible");
    }
    irq_enabled = curr_cpu->irq_enabled;
    swtch(&curr_proc->context, curr_cpu->scheduler);
    curr_cpu->irq_enabled = irq_enabled;
}


/**
 * Gives up the CPU for a scheduling round.
 *
 * yield suspends the current process' execution until
 * the next execution round. This can be useful if the
 * process if performing a long task, or momentarily
 * waiting on I/O, to allow other processes to continue.
 *
 * yeild will acquire the process table lock and change
 * the process' state as required to call into the
 * scheduler.
 */
void yield(void)
{
    acquire(&ptable.lock);
    curr_proc->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}


/**
 * A forked child process will begin executing from here.
 *
 * New processes, created by fork(), will begin executing
 * from fork_return. fork_return returns to process to
 * user space, by returning - The link register is set
 * to the trapret subroutine, which loads the processes'
 * user mode trap frame as copied from the parent process.
 *
 * @see alloc_proc() and spawn_proc() in proc.c
 *
 * @see trapret in exception.S *
 */
void fork_return(void)
{
    static int first_proc = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);
    if (first_proc) {
        /* Some initialization functions must be run in the context
         * of a regular process (e.g., they call sleep), and thus cannot
         * be run from main(). */
        first_proc = 0;
        initlog();
    }
    return;
}


/**
 * Sleep on a channel while yielding a lock.
 *
 * sleep will release a lock passed in and sleep the
 * current process. While sleeping a process will not
 * be scheduled. The process can be woken up by calling
 * wakeup() on the specified channel.
 *
 * sleep will reacquire the lock when woken up, before
 * returning.
 *
 * @param chan - Sleep until woken up on this channel.
 * @param lk - The lock to release until wakeup.
 */
void sleep(void* chan, struct spinlock* lk)
{
    if (curr_proc == 0) {
        panic("sleep");
    }
    if (lk == 0) {
        panic("sleep without lk");
    }
    /* We must acquire ptable.lock in order to
     * change p->state and then call sched.
     * Once we hold ptable.lock, we can be
     * guaranteed that we won't miss any wakeup
     * (wakeup runs with ptable.lock locked),
     * so it's okay to release lk. */
    if (lk != &ptable.lock){
        acquire(&ptable.lock);
        release(lk);
    }
    curr_proc->channel = chan;
    curr_proc->state = SLEEPING;
    sched();
    /* Process resumes here after wakeup. Clean up. */
    curr_proc->channel = 0;
    /* Reacquire original lock. */
    if (lk != &ptable.lock){
        release(&ptable.lock);
        acquire(lk);
    }
}


/**
 * Wake up all process sleeping on a channel, without
 * locking the ptable.
 *
 * @warning wakeup_1() assumes the page table lock is held.
 * If a process does not hold the ptable.lock, use wakeup()
 * instead - which will acquire and release the lock.
 *
 * @param chan - Wakeup processes sleeping on this channel.
 */
static void wakeup_1(void *chan)
{
    struct proc* p;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == SLEEPING && p->channel == chan) {
            p->state = RUNNABLE;
        }
    }
}


/**
 * Wake up all process sleeping on a channel, after acquiring
 * the process table lock.
 *
 * wakeup will acquire the process table lock, and release
 * it after waking up the processes. The process table lock
 * is required to safely modify the process table, so you
 * should use wakeup and not wakeup_l unless you know you
 * already hold ptable.lock.
 *
 * @param chan - Sleep until woken up on this channel.
 */
void wakeup(void* chan)
{
    acquire(&ptable.lock);
    wakeup_1(chan);
    release(&ptable.lock);
}


/**
 * Kill the process with the given PID.
 *
 * kill sets the 'killed' flag on the process with PID, if
 * it exists. When the process returns to userspace, the
 * process will be terminated and the PCB freed by exit().
 *
 * @see trap() in trap.c, which exit()s the process upon a
 * return to userspace.
 *
 * @param pid - The Process ID to kill.
 * @return 0 on success, -1 on failure.
 */
int kill(int pid)
{
    struct proc* p;
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->pid == pid){
            p->killed = 1;
            /* Wake process from sleep if necessary.
             * A SLEEPING process will not be run to
             * return to userspace, and so can not
             * exit. */
            if (p->state == SLEEPING) {
                p->state = RUNNABLE;
            }
         release(&ptable.lock);
         return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}


/**
 * Print a process listing to the console, for debugging.
 *
 * procdump runs when the user types ^P on the console.
 * procdump is not locked to avoid wedging a stuck machine
 * further.
 *
 * @warning procdump is not implemented on ARM xv6.
 *
 * @todo Implement procdump.
 */
void procdump(void)
{
}
