#ifndef KERNEL_H
#define KERNEL_H

#define SYS_DBG 1
#define SYS_GETKEY 2
#define SYS_INSTALL_KBD_HANDLE 3

// Simple wrapper for system calls
#define do_syscall(num, arg1, arg2, arg3, return) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3)); \
    asm volatile("mov %%eax, %0" : "=r" (return))

#endif // KERNEL_H