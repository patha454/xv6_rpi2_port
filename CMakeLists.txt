#[[
This is a dummy CMakeLists.txt to placate the CLion IDE.
To build this version of xv6, use the makefile provided.
]]
cmake_minimum_required(VERSION 3.10.3)
project(xv6_dummy)
set(C_STANDARD 90)

include_directories(include)

add_executable(dummy_kernel
        source/bio.c
        source/console.c
        source/entry.S
        source/exception.S
        source/exec.c
        source/file.c
        source/fs.c
        source/kalloc.c
        source/log.c
        source/mailbox.c
        source/main.c
        source/memide.c
        source/mmu.c
        source/pipe.c
        source/proc.c
        source/spinlock.c
        source/string.c
        source/syscall.c
        source/sysfile.c
        source/sysproc.c
        source/timer.c
        source/trap.c
        source/uart.c
        source/uart_pl011.c
        source/vm.c
        include/arm.h
        include/buf.h
        include/defs.h
        include/elf.h
        include/fcntl.h
        include/file.h
        include/fs.h
        include/fvp.h
        include/mailbox.h
        include/memlayout.h
        include/mmu.h
        include/param.h
        include/proc.h
        include/spinlock.h
        include/stat.h
        include/syscall.h
        include/types.h
        include/traps.h
        include/user.h)
