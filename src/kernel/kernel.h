#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <console.h>
#include <stddef.h>

#define STDOUT 1
#define STDIN 0

enum System_Calls {
    SYS_DBG = 1,                            // Debug system call
    SYS_INSTALL_KBD_HANDLE,                 // Install a keyboard callback
    SYS_REMOVE_KBD_HANDLE,                  // Remove a keyboard callback
    SYS_WRITE,                              // Write to a file
    SYS_READ,                               // Read from a file
    SYS_EXIT,                               // Exit the current process
    SYS_FORK,                               // Fork the current process
    SYS_EXEC,                               // Execute a new process which replaces the current one
    SYS_WAIT,                               // Wait for a process to exit
    SYS_GET_PID,                            // Get the PID of the current process
    SYS_GET_PCB,                            // Get the PCB of the current process (privelige level?)
    SYS_OPEN,                               // Open a file
    SYS_CLOSE,                              // Close a file
    SYS_SEEK,                               // Seek to a position in a file
    SYS_SLEEP,                              // Sleep for a certain amount of time
    SYS_GET_TIME,                           // Get the current time
    SYS_KILL                                // Kill a process
};

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


// This function is primarily here to make debugging easier
static inline void write(const char* str, uint32_t fileDescriptor){
    do_syscall(SYS_WRITE, fileDescriptor, (uint32_t)str, 0);
}

#endif // KERNEL_H