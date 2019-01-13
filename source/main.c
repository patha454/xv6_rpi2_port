/**
 * @file main.c
 *
 * main.c provides a function, 'cmain', to initialise the
 * kernel and begin executing user processes, after the
 * boot-loader and entry assembly have prepared the
 * hardware to execute bare-metal C code.
 *
 * main.c also contains several miscellaneous functions which
 * support cmain()'s initialisation efforts.
 *
 * @author Zhiyi Huang, University of Otago, hzy@cs.otago.ac.nz
 * (Adaption from MIT XV6.)
 *
 * @author H Paterson, University of Otago, patha454@student.otago.ac.nz
 * (Documentation and refactoring.)
 *
 * @date 13/01/2018
 */

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"
#include "mailbox.h"


/**
 * @var kernel_bin_end
 *
 * 'kernel_bin_end' ("Kernel Binary End") is the first
 * the end of the kernel binary, as loaded from the ELF
 * file.
 */
extern char kernel_bin_end[];

/**
 * @var kernel_page_dir
 *
 * 'kernel_page_dir' is a global pointer to the page
 * directory used to store the kernel's memory mappings.
 */
extern pde_t *kernel_page_dir;


/**
 * @var mail_buffer
 *
 * 'mail_buffer' is a pointer to memory used to buffer
 * mailbox communication between the kernel and hardware.
 */
extern volatile u_int32* mail_buffer;


/**
 * @var pm_size
 *
 * 'pm_size' ("Physical Memory Size") provides a global
 * indication of the amount of physical memory present on
 * the hardware, or in early stages of kernel
 * initialisation, assumed to be present.
 */
extern unsigned int pm_size;


/**
 * Provides a hardware-level indication that the OS is
 * operating normally.
 *
 * 'ok_loop' flashes a hardware LED at a high frequency
 * (2.5 Hz) to indicate the OS is ok, even if other
 * I/O (such as a video or UART console) is unavailable.
 *
 * @note'ok_loop' is not currently used by XV6, but
 * could be enabled for debugging and development.
 *
 * @warning 'ok_loop' is a blocking infinite loop. It
 * should be run in its own thread, if used while the OS is
 * operating.
 *
 * @todo Move the magic numbers 18, indicating the LED GPIO
 * , into an IO header
 * file.
 */
void ok_loop()
{
    /* GPIO 18 indicates the OK LED in output mode */
    setgpiofunc(18, 1);
    while (1) {
         setgpioval(18, 0);
         delay(2000000);
         setgpioval(18, 1);
         delay(2000000);
    }
}


/**
 * Provides a hardware-level indication that the OS is
 * in an error state.
 *
 * 'not_ok_loop' flashes a hardware LED at a low frequency
 * (1 Hz) to indicate the OS has encountered a (typically
 * unrecoverable) error.
 *
 * 'not_ok_loop' is called when the kernel has detected an
 * abnormal condition which prevents execution from
 * safely continuing. This is usually the result of a bug
 * in the code.
 *
 * @warning 'ok_loop' is a blocking, infinite loop.
 *
 * @todo Move the magic numbers 18, indicating the LED GPIO
 * ,and 0, indicating set GPIO as output, into an IO header
 * file.
 */
void not_ok_loop()
{
    /* GPIO 18 indicates the OK LED in output mode */
    setgpiofunc(18, 1);
    while (1) {
         setgpioval(18, 0);
         delay(500000);
         setgpioval(18, 1);
         delay(500000);
    }
}


/**
 * Queries the amount of physical memory present on the
 * system.
 *
 * 'get_pm_size' uses the mailbox system to query the
 * the hardware, to determine how much physical memory the
 * system is configured with.
 *
 * @note 'get_pm_size' requires the MMIO devices to be
 * mapped into the address space before it will work.
 */
unsigned int get_pm_size()
{
    create_request(mail_buffer, MPI_TAG_GET_ARM_MEMORY, 8, 0, 0);
    writemailbox((u_int32*) mail_buffer, 8);
    readmailbox(8);
    if (mail_buffer[1] != 0x80000000) {
        cprintf("Error readmailbox: %x\n", MPI_TAG_GET_ARM_MEMORY);
    }
    return mail_buffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH + 1];
}


/**
 * Initialises data structures for multiple CPUs or cores.
 *
 * 'machinit' initialises memory used to store the state of
 * each CPU in a multiprocessing system.
 *
 * @see struct cpu, in proc.c
 */
void machinit(void)
{
    memset(cpus, 0, sizeof(struct cpu) * NCPU);
}


/**
 * cmain() performs OS initialisation and enters the task
 * scheduler.
 *
 * 'cmain' is the first C function executed after the
 * bootloader and OS assembly routines have executed to
 * ready the hardware for baremetal C execution.
 *
 * 'cmain' initialises each of XV6's subsystems then calls
 * the scheduler: userinit() has been used to setup a single
 * process which will be executed by the scheduler to give
 * the user a shell.
 *
 * @return 'cmain' should never return. Both the scheduler
 * and 'not_ok_loop', which are called at the end of 'cmain'
 * are infinite loops.
 */
int cmain()
{
    mmu_init_stage1();
    machinit();
    #if defined (RPI1) || defined (RPI2)
    uartinit();
    #elif defined (FVP)
    uartinit_fvp();
    #endif
    dsb_barrier();
    consoleinit();
    cprintf("\nHello World from xv6\n");
    kinit1(kernel_bin_end, P2V((8 * 1024 * 1024) + PHYSTART));
    // collect some free space (8 MB) for imminent use
    // the physical space below 0x8000 is reserved for PGDIR and kernel stack
    kernel_page_dir = p2v(K_PDX_BASE);
    mailboxinit();
    pm_size = get_pm_size();
    cprintf("ARM memory is %x\n", pm_size);
    mmu_init_stage2();
    gpuinit();
    pinit();
    tv_init();
    cprintf("%s: Ok after tv_init\n", __func__);
    binit();
    cprintf("%s: Ok after binit\n", __func__);
    fileinit();
    cprintf("%s: Ok after fileinit\n", __func__);
    iinit();
    cprintf("%s: Ok after iinit\n", __func__);
    ideinit();
    cprintf("%s: Ok after ideinit\n", __func__);
    kinit2(P2V((8 * 1024 * 1024) + PHYSTART), P2V(pm_size));
    cprintf("%s: Ok after kinit2\n", __func__);
    userinit();
    cprintf("%s: Ok after userinit\n", __func__);
    timer3init();
    cprintf("%s: Ok after timer3init\n", __func__);
    scheduler();
    not_ok_loop();
    return 0;
}

