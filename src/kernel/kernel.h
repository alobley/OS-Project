#ifndef KERNEL_H
#define KERNEL_H

/* TODO: 
 * - Create unistd.h instead of kernel.h?
 * - Create wrapper functions for all system calls
*/

#include <stdint.h>
#include <console.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define ANSI_ESCAPE "\033[2J\033[H"

#define STDERR_FILENO 2
#define STDOUT_FILENO 1
#define STDIN_FILENO 0

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
    SYS_KILL,                               // Kill a process
    SYS_YIELD,                              // Voluntarily yield the CPU
    SYS_MMAP,                               // Map memory pages
    SYS_MUNMAP,                             // Unmap memory pages
    SYS_BRK,                                // Change the heap size
    SYS_MPROTECT,                           // Change the protection of memory pages
    SYS_REGDUMP,                            // Dump the registers to the console

    // Priveliged system calls for drivers and kernel modules (privelige check required, will check PCB)
    // Note - these will always be the highest system calls
    SYS_MODULE_LOAD,                        // Load a kernel module
    SYS_MODULE_UNLOAD,                      // Unload a kernel module
    SYS_ADD_VFS_DEV,                        // Add a device to the VFS
    SYS_MODULE_QUERY,                       // Query a kernel module
    SYS_REGISTER_DEVICE,                    // Register a device
    SYS_UNREGISTER_DEVICE,                  // Unregister a device
    SYS_GET_DEVICE,                         // Get a device by its device ID
    SYS_GET_FIRST_DEVICE,                   // Get the first device of a certain type (can search through children/next)
    SYS_REQUEST_IRQ,                        // Request an IRQ
    SYS_RELEASE_IRQ,                        // Release an IRQ
    SYS_DRIVER_SET_STATUS,                  // Set the status of a driver
    SYS_DRIVER_MMAP,                        // Memory-map a region of MMIO to userland for shared access
    SYS_DRIVER_MUNMAP,                      // Unmap a region of MMIO from userland
    SYS_IO_PORT_READ,                       // Read from an I/O port
    SYS_IO_PORT_WRITE,                      // Write to an I/O port
    SYS_BLOCK_READ,                         // Read from a block device
    SYS_BLOCK_WRITE,                        // Write to a block device
    SYS_ENTER_V86_MODE,                     // Set the CPU into V86 mode
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

#define SYSCALL_INT 0x30

// Simple wrapper for system calls
#define do_syscall(num, arg1, arg2, arg3, arg4, arg5) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5))


#endif // KERNEL_H