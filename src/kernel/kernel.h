#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <console.h>
#include <stddef.h>

#define SYS_DBG 1
#define SYS_INSTALL_KBD_HANDLE 2

typedef struct Version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

static const char* kernelRelease = "Alpha";

extern size_t memSize;
extern size_t memSizeMiB;
extern version_t kernelVersion;

// Simple wrapper for system calls
#define do_syscall(num, arg1, arg2, arg3) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3))

#endif // KERNEL_H