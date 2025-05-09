#ifndef KERNEL_H
#define KERNEL_H

#ifndef asm
#define asm __asm__
#endif

enum System_Calls {
    SYS_DBG = 0,                            // Debug system call

    // Filesystem I/O (the file operations do include devices)
    SYS_WRITE,                              // Write to a file descriptor
    SYS_READ,                               // Read from a file descriptor (must make an EXT driver to see about symlink open/close)
    SYS_OPEN,                               // Open a file
    SYS_CLOSE,                              // Close a file
    SYS_SEEK,                               // Seek to a position in a file
    SYS_FSYNC,                              // Flush a file's buffers (write the data to disk)
    SYS_SYNC,                               // Flush all files' buffers (write the data to disk) (Just search the VFS and write the data using the mountpoint)
    SYS_TRUNCATE,                           // Truncate a file to a certain size
    SYS_STAT,                               // Get VFS node information from a given path (devices, files, and directories are allowed)
    SYS_FSTAT,                              // Get VFS node information from a file descriptor (devices, files, and directories are allowed)
    SYS_GETDENTS,                           // Get directory entry data
    SYS_CHMOD,                              // Change file permissions
    SYS_CHOWN,                              // Change file ownership
    SYS_SYMLINK,                            // Create a symbolic link
    SYS_UNLINK,                             // Delete a file or symbolic link
    SYS_MKDIR,                              // Make a directory
    SYS_RMDIR,                              // Remove a directory
    SYS_RENAME,                             // Rename a file
    SYS_GETCWD,                             // Get the current working directory
    SYS_CHDIR,                              // Change the current working directory
    SYS_MOUNT,                              // Mount a filesystem
    SYS_UMOUNT,                             // Unmount a filesystem
    SYS_DUP,                                // Duplicate a file descriptor - creates a new file descriptor that points to the same file
    SYS_DUP2,                               // Replace and Duplicate a file descriptor - replaces the original file at a descriptor pointer with a new one
    SYS_FCNTL,                              // File control - perform operations on a file descriptor
    SYS_IOCTL,                              // Perform an I/O control operation on a file descriptor
    SYS_POLL,                               // Wait for a file in a given set of file descriptors to update

    // Process management
    SYS_EXIT,                               // Exit the current process
    SYS_FORK,                               // Fork the current process
    SYS_EXEC,                               // Execute a new process which replaces the current one - does not copy env
    SYS_EXECVE,                             // Execute a new process which replaces the current one - copies env (need to figure out how I want to do environment variables)
    SYS_WAIT_PID,                           // Wait for a process to exit
    SYS_GET_PID,                            // Get the PID of the current process
    SYS_GET_PPID,                           // Get the PID of the parent process
    SYS_KILL,                               // Kill a process
    SYS_YIELD,                              // Voluntarily yield the CPU
    SYS_GETRLIMIT,                          // Get the resource limits of a process
    SYS_SETRLIMIT,                          // Set the resource limits of a process
    SYS_GETUID,                             // Get the UID of the current process
    SYS_GETGID,                             // Get the GID of the current process

    // Inter-process communication
    SYS_PIPE,                               // Create a pipe (returns a file descriptor, to remove the pipe, simply use close)
    SYS_SHMGET,                             // Get a shared memory segment
    SYS_SHMAT,                              // Attach to a shared memory segment
    SYS_SHMDT,                              // Detach from a shared memory segment
    SYS_MSGGET,                             // Get a message queue
    SYS_MSGSND,                             // Send a message
    SYS_MSGRCV,                             // Receive a message
    SYS_SETSID,                             // Set the session ID for this process
    SYS_GETSID,                             // Get the current session ID for this process

    // Time management
    SYS_SLEEP,                              // Sleep for a certain amount of time
    SYS_GET_TIME,                           // Get the current date and time of day

    // Networking
    SYS_SOCKET,                             // Create a socket (returns a file descriptor)
    SYS_BIND,                               // Bind a socket
    SYS_LISTEN,                             // Listen on a socket
    SYS_ACCEPT,                             // Accept a connection on a socket
    SYS_CONNECT,                            // Connect to a socket
    SYS_SENDTO,                             // Send data to a socket
    SYS_RECVFROM,                           // Receive data from a socket
    SYS_SETSOCKOPT,                         // Set socket options
    SYS_GETSOCKOPT,                         // Get socket options

    // Memory management
    SYS_MMAP,                               // Map files or devices into memory
    SYS_MUNMAP,                             // Unmap a region of memory
    SYS_BRK,                                // Change the heap size to X bytes. Returns pointer to the heap if X = 0.
    SYS_MPROTECT,                           // Change the protection of a memory region using given flags

    // Signal management
    SYS_SENDSIG,                            // Send a signal to a process
    SYS_SIGACTION,                          // Set a signal handler
    SYS_SIGPROCMASK,                        // Set the signal mask for a process
    SYS_ALARM,                              // Set an alarm for a process

    // Priveliged system calls for drivers and kernel modules or system management (privelige check required, will check UID or GID)
    // Note - these will always be the highest system calls
    SYS_MODULE_LOAD,                        // Load a kernel module (takes a path, specifically for kernelspace drivers and such)
    SYS_MODULE_UNLOAD,                      // Unload a kernel module
    SYS_MODULE_QUERY,                       // Query a kernel module
    SYS_REGISTER_DEVICE,                    // Register a device into the system (must provide a valid path)
    SYS_UNREGISTER_DEVICE,                  // Unregister a device from the system
    SYS_ACQUIRE_DEVICE,                     // Request control of a specific device (takes a path). Good for drivers that want to replace kernel drivers.
    SYS_REQUEST_IRQ,                        // Request an IRQ
    SYS_RELEASE_IRQ,                        // Release an IRQ
    SYS_GET_API,                            // Get the kernel API functions

    // User/Group management
    SYS_SETUID,                             // Set the UID of the current process
    SYS_SETGID,                             // Set the GID of the current process
    SYS_SETGROUPS,                          // Set the groups of the current process
    SYS_GETGROUPS,                          // Get the groups of the current process

    // System management
    SYS_SET_TIME,                           // Set the current date and time of day
    SYS_SHUTDOWN,                           // Shutdown the system
    SYS_REBOOT,                             // Reboot the system
    SYS_UNAME,                              // Get system information
    SYS_CHROOT,                             // Change the root directory of the VFS

    // Kernel querying
    SYS_KLOG_READ,                          // Read the kernel log (I will eventually need to change printk to use a buffer or file instead of directly to console)
    SYS_KLOG_FLUSH,                         // Flush the kernel log
};

static const char* kernelRelease = "Alpha";

#define SYSCALL_INT 0x30

// Simple wrapper for system calls
#define do_syscall(num, arg1, arg2, arg3, arg4, arg5) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5) : "memory");

typedef unsigned int _ADDRESS;
#define getresult(x) asm volatile("mov %%eax, %0" : "=r" (x) : : "eax", "memory");

#endif // KERNEL_H