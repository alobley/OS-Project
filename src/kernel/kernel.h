#ifndef KERNEL_H
#define KERNEL_H

#include <common.h>
#include <multitasking.h>

enum System_Calls {
    SYS_DBG = 1,                            // Debug system call

    // Keyboard management
    SYS_INSTALL_KBD_HANDLE,                 // Install a keyboard callback
    SYS_REMOVE_KBD_HANDLE,                  // Remove a keyboard callback

    // Filesystem I/O
    SYS_WRITE,                              // Write to a file
    SYS_READ,                               // Read from a file
    SYS_OPEN,                               // Open a file
    SYS_CLOSE,                              // Close a file
    SYS_SEEK,                               // Seek to a position in a file
    SYS_STAT,                               // Get file information
    SYS_CHMOD,                              // Change file permissions
    SYS_CHOWN,                              // Change file ownership
    SYS_UNLINK,                             // Unlink a file
    SYS_MKDIR,                              // Make a directory
    SYS_RMDIR,                              // Remove a directory
    SYS_RENAME,                             // Rename a file
    SYS_GETCWD,                             // Get the current working directory
    SYS_CHDIR,                              // Change the current working directory
    SYS_REPDUP,                             // Replace and Duplicate a file descriptor - replaces the original file descriptor pointer with a new one (like dup2 on Linux)

    // Process management
    SYS_EXIT,                               // Exit the current process
    SYS_FORK,                               // Fork the current process
    SYS_EXEC,                               // Execute a new process which replaces the current one
    SYS_WAIT_PID,                           // Wait for a process to exit
    SYS_GET_PID,                            // Get the PID of the current process
    SYS_GET_PPID,                           // Get the PID of the parent process
    SYS_KILL,                               // Kill a process
    SYS_YIELD,                              // Voluntarily yield the CPU
    SYS_PIPE,                               // Create a pipe (returns a file descriptor, to remove the pipe, simply use close)
    SYS_SHMGET,                             // Get a shared memory segment
    SYS_SHMAT,                              // Attach to a shared memory segment
    SYS_SHMDT,                              // Detach from a shared memory segment
    SYS_MSGGET,                             // Get a message queue
    SYS_MSGSND,                             // Send a message

    // Time management
    SYS_SLEEP,                              // Sleep for a certain amount of time
    SYS_GET_TIME,                           // Get the current time of day
    SYS_SET_TIME,                           // Set the current time of day
    SYS_INSTALL_TIMER_HANDLE,               // Install a timer callback
    SYS_REMOVE_TIMER_HANDLE,                // Remove a timer callback

    // Networking
    SYS_SOCKET,                             // Create a socket
    SYS_BIND,                               // Bind a socket
    SYS_LISTEN,                             // Listen on a socket
    SYS_ACCEPT,                             // Accept a connection on a socket
    SYS_CONNECT,                            // Connect to a socket
    SYS_SEND,                               // Send data on a socket
    SYS_RECV,                               // Receive data on a socket
    SYS_CLOSE_SOCKET,                       // Close a socket

    // Memory management
    SYS_MMAP,                               // Map files or devices into memory
    SYS_MUNMAP,                             // Unmap a region of memory
    SYS_BRK,                                // Change the heap size
    SYS_MPROTECT,                           // Change the protection of memory pages

    // System/Device I/O
    SYS_REGDUMP,                            // Dump the registers to the console
    SYS_SYSINFO,                            // Get system information
    SYS_OPEN_DEVICE,                        // Open a device
    SYS_CLOSE_DEVICE,                       // Close a device
    SYS_DEVICE_READ,                        // Read from a given device
    SYS_DEVICE_WRITE,                       // Write to a given device
    SYS_DEVICE_IOCTL,                       // Perform an ioctl on a given device

    // Priveliged system calls for drivers and kernel modules (privelige check required, will check PCB)
    // Note - these will always be the highest system calls
    SYS_MODULE_LOAD,                        // Load a kernel module
    SYS_MODULE_UNLOAD,                      // Unload a kernel module
    SYS_ADD_VFS_DEV,                        // Add a device to the VFS
    SYS_MODULE_QUERY,                       // Query a kernel module
    SYS_FIND_MODULE,                        // Find a kernel module by its supported device type
    SYS_REGISTER_DEVICE,                    // Register a device
    SYS_UNREGISTER_DEVICE,                  // Unregister a device
    SYS_GET_DEVICE,                         // Get a device by its device ID
    SYS_REQUEST_IRQ,                        // Request an IRQ
    SYS_RELEASE_IRQ,                        // Release an IRQ
    SYS_DRIVER_MMAP,                        // Memory-map a region of MMIO to userland for shared access
    SYS_DRIVER_MUNMAP,                      // Unmap a region of MMIO from userland
    SYS_IO_PORT_READ,                       // Read from an I/O port
    SYS_IO_PORT_WRITE,                      // Write to an I/O port
    SYS_ENTER_V86_MODE,                     // Set the CPU into virtual 8086 mode
    SYS_ENTER_RING0,                        // Set the CPU into ring 0 (kernel mode)
    SYS_SHUTDOWN,                           // Shutdown the system
    SYS_REBOOT,                             // Reboot the system
};

static const char* kernelRelease = "Alpha";

extern version_t kernelVersion;

#define SYSCALL_INT 0x30

// Simple wrapper for system calls
#define do_syscall(num, arg1, arg2, arg3, arg4, arg5) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5) : "memory");


#endif // KERNEL_H