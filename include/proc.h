/**
 * @file proc.h
 *
 * proc.h provides data structures and variables used to describe
 * and run executable processes.
 *
 * proc.h also provides some of the high level variables used to
 * describe and control multiprocessing.
 *
 * @note Changes to proc.h should be copied to uprogs/prog.h before
 * building user programs. 
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation, styling, and refactoring.)
 *
 * @date 06/11/2018
 */


/*
 * Process memory is laid out contiguously, low addresses first:
 *   text
 *   original data and bss
 *   fixed-size stack
 *   expandable heap
 */


/**
 * @struct cpu - The local state per CPU.
 *
 * The CPU states contains identification for each CPU, a
 * pointer to it's scheduler, and information about the
 * process currently executing.
 *
 * @todo - Reimpliment cpu.gdt, which is used for x86 multiprocessing?
 */
struct cpu {
    u_char8 id;                 /**< Local CPU ID from the ARM co-processor. */
    struct context* scheduler;  /**< swtch() here to enter scheduler. */
    volatile u_int32 started;   /**< Indicates if the CPU is started. */
    int ncli;                   /**< depth of pushcli() nesting. @see spinlock.c. */
    int irq_enabled;            /**< Indicates if interrupts were enabled before pushcli(). */
    /* Cpu-local storage variables; see below. (curr_cpu & curr_proc) Not implemented properly in ARM xv6. */
    struct cpu* cpu;            /**< A self-reference to the CPU, used for CPU local storage. */
    struct proc* proc;          /**< The currently running process. */
};


/**
 * cpus is a global list of all CPUs in the system.
 *
 * cpus may include CPUS which do not exist, depending on
 * the actual number of CPUS and NCPU. CPUs which are not
 * started should not be accessed through 'cpus', as they
 * may not exist.
 * */
struct cpu cpus[NCPU];


/**
 * @var ncpu - The number of actual processors present
 * on the system.
 *
 * @todo impliment mp.c including ncpu.
 */
/* extern int ncpu; */


/** @todo - Impliment these for MP.
 * x86 version:  seginit sets up the
 * %gs segment register so that %gs refers to the memory
 * holding those two variables in the local cpu's struct cpu.
 * This is similar to how thread-local variables are implemented
 * in thread libraries such as Linux pthreads.
 * extern struct cpu *cpu asm("%gs:0");     was &cpus[cpunum()]
 * extern struct proc *proc asm("%gs:4");   was cpus[cpunum()].proc
 */


/**
 * @def curr_cpu (current_cpu) - a pointer the the CPU which
 * retrieves the variable.
 *
 * @todo Implement a multiprocessor version of curr_cpu. This could
 * be stored on a processor local register (if one exists), a macro
 * script, or use a MMU segment to redirect the variables to the
 * variable in the local CPU's struct (see the x86 version.)
 *
 * @todo Depending on the approach, this may require cpunum() to be
 * implemented.
 */
#define curr_cpu (&cpus[0])


/**
 * @def curr_proc (current_cpu) - a pointer the the PCB for the
 * process which the current CPU is running.
 *
 * @todo Implement a multiprocessor version of curr_pcb. This could
 * be stored on a processor local register (if one exists), a macro
 * script, or use a MMU segment to redirect the variables to the
 * variable in the local CPU's struct (see the x86 version.)
 *
 * @todo Depending on the approach, this may require cpunum() to be
 * implemented.
 */
#define curr_proc   (cpus[0].proc)


/**
 * @struct context - Saved registers for context switches
 * inside the kernel.
 *
 * context doesn't need to save r0-r3 because the ARM
 * convention is that the callee saves and restores them,
 * including before and after a system call/trap which
 * could cause a context switch.
 *
 * Contexts are stored on the top (low addresses) of the
 * stack they describe. The stack pointer is the address
 * of the context, and struct context matches the layout
 * of the stack in the "swtch" routine.
 *
 * struct context is used for changing context from process
 * to process (including from 'user' process to scheduler
 * inside the kernel - thus the registers may include
 * operating system data and variables from the kernel
 * code. Contexts are switched by the assembly routine
 * 'swtch.'
 *
 * @see swtch in exception.S
 */
struct context {
  u_int32 r4;
  u_int32 r5;
  u_int32 r6;
  u_int32 r7;
  u_int32 r8;
  u_int32 r9;
  u_int32 r10;
  u_int32 r11;
  u_int32 r12;
  u_int32 lr;
  u_int32 pc;
};


/**
 * @enum proc_state describes the state of a process in
 * the execution lifecycle.
 *
 * UNUSED   - PBC not in use.
 * EMBRYO   - PBC allocated, but process is still being created.
 *            The PBC variables can not be assumed initialised.
 * SLEEPING - Process waiting on external events (I/O, ect...)
 * RUNNABLE - Process is ready to run when a CPU is avalable.
 * RUNNING  - Process is currently being run on a CPU.
 * ZOMBIE   - Process had exited; waiting for parent to wait() to
 *            destroy the process.
 */
enum proc_state {
    UNUSED=0,
    EMBRYO,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};


/**
 * @struct proc - The PBC block containing information about
 * a process.
 *
 * The Process Control Block (PCB) contains the state of a
 * process. The information in the PCB is either required
 * to run the process, or makes process execution faster.
 */
struct proc {
    /* Does sz refer to the process' heap size, or heap + stack + text + data? */
    u_int32 sz;                  /**< Size of memory allocated to the process. */
    pde_t* pgdir;                /**< Page table (Page Directory. */
    char* kstack;                /**< 'Top' of the process' kernel stack (lowest address.) */
    enum proc_state state;       /**< Process state. */
    volatile int pid;            /**< Process ID. */
    struct proc* parent;         /**< Parent process. */
    struct trapframe* tf;        /**< Trap frame for the current system call. */
    struct context* context;     /**< swtch() to the process stack (here) to run. */
    void* channel;                  /**< If not 0, process is sleeping until wakeup is called on 'chan.' */
    int killed;                  /**< Non-zero if the process has been killed. */
    struct file* ofile[NOFILE];  /**< Index of files opened by the process. */
    struct inode* cwd;           /**< Current working directory of the process. */
    char name[16];               /**< Process name, for debugging only. */
};
