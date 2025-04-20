#ifndef SYSTEM_H
#define SYSTEM_H

#include <kernel.h>

#ifndef __size_t_defined
#define __size_t_defined
typedef unsigned int size_t;
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef signed int ssize_t;
#endif

// Internal uint8_t type to make variables more readable
typedef unsigned char __uint8_t;

typedef struct Version {
    __uint8_t major;
    __uint8_t minor;
    __uint8_t patch;
} version_t;

#define SYSTEM_API_VERSION ((version_t){.major = 0, .minor = 1, .patch = 0}) // The version of the system API

#define ANSI_ESCAPE "\033[2J\033[H"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// More readable success and failure codes for functions
#define OUT_OF_MEMORY -2
#define STANDARD_FAILURE -1
#define STANDARD_SUCCESS 0

// System call returns
#define SYSCALL_SUCCESS 0                           // System call returned without issue
#define SYSCALL_FAILURE -1                          // System call failed for some reason
#define SYSCALL_ACCESS_DENIED -2                    // Access denied upon a system call
#define SYSCALL_INVALID_ARGUMENT -3                 // Invalid argument on a system call
#define SYSCALL_NOT_FOUND -4                        // System call not found
#define SYSCALL_OUT_OF_MEMORY -5                    // The system is out of memory!
#define SYSCALL_NOT_IMPLEMENTED -6                  // This syscall isn't implemented yet
#define SYSCALL_TASKING_FAILURE -999                // Specifically for syscalls like exec
#define SYSCALL_FAULT_DETECTED -1000                // CPU fault detected upon a system call

// These are equal to the internal flags of the VFS nodes (makes things easier)
#define O_RDONLY (1 << 2)                           // File is read-only
#define O_WRONLY (1 << 3)                           // File is write-only
#define O_RDWR ((~(O_RDONLY)) | (~(O_WRONLY)))      // File is opened for reading and writing
#define O_CREAT (1 << 9)                            // Create a new file
#define O_TRUNC (1 << 10)                           // If the file exists and is open for writing, truncate it to zero.
#define O_APPEND (1 << 11)                          // All writes shall be appended to the end of the file
#define O_NONBLOCK (1 << 12)                        // Open the file in a non-blocking  way (hmmm... I may need to change some designs)
#define O_SYNC (1 << 13)                            // Writes are all written to the disk before returning
#define O_CLOEXEC (1 << 14)                         // File is closed upon exec
#define O_NOFOLLOW (1 << 15)                        // Fail if the path is a symbolic link

// File permissions
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100

#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010

#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01

typedef int device_id_t;
typedef unsigned short vendor_id_t;

typedef unsigned int uid_t;
typedef unsigned int gid_t;

typedef struct Date_Time {
    __uint8_t second;
    __uint8_t minute;
    __uint8_t hour;
    __uint8_t day;
    __uint8_t month;
    unsigned short year;
} datetime_t;

// General-purpose input event for devices
struct ievent {
    datetime_t time;            // The time this event occurred
    unsigned short type;        // The type of event this is
    unsigned short code;        // Special code for the evtent  
    int value;                  // Value returned
};

typedef unsigned short pid_t;

// Simple directory entry struct.
struct dirent {
    unsigned char type;                         // The type of entry this is
    unsigned int len;                           // The length of this structure
    char name[];                                // The name of this directory entry (remember to allocate sizeof(struct dirent) + strlen(name))
};

struct stat {
    device_id_t device;                         // If a device, this is its device ID
    unsigned int permissions;                   // The permissions of this VFS entry
    unsigned int flags;
    uid_t ownerUID;                             // The owner user of this entry
    gid_t ownerGID;                             // The owner group of this entry
    unsigned int size;                          // The size (in bytes) of this entry
    unsigned int blockSize;                     // What do I use this for?
    unsigned int blockCount;                    // The number of 512-byte blocks on this device
    datetime_t lastAccess;                      // Last access date and time
    datetime_t lastModified;                    // Last modification date and time
    datetime_t lastStatChange;                  // Last status change date and time
};

struct framebuffer {
    void* address;                              // The address of the framebuffer
    unsigned int size;                          // Size in bytes
    unsigned short width;                       // Width in pixels (or characters)
    unsigned short height;                      // Height in pixels (or characters)
    unsigned short pitch;                       // Bytes per line
    __uint8_t bpp;                              // Bits per pixel
    _Bool text;                                 // True if the framebuffer is in text mode (kernel will assume VGA-like text/attribute)
    __uint8_t red_mask_size;                    // Number of bits used for red
    __uint8_t green_mask_size;                  // Number of bits used for green
    __uint8_t blue_mask_size;                   // Number of bits used for blue
    __uint8_t reserved_mask_size;               // Either bits used for alpha or reserved
};

// For Semaphore operations
#define SEM_DEADLOCK_DETECTED -999              // A deadlock was detected!
#define SEM_INTERNAL_ERROR -3                   // There was a kernel error!
#define SEM_INVALID_LOCK -2                     // A lock value was not 0 or 1
#define SEM_UNLOCKED 0                          // Semaphore is currently unlocked
#define SEM_LOCKED 1                            // Semaphore is already locked
#define SEM_AQUIRED 2                           // Semaphore was aquired by this process

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef int fd_t;

struct uname {
    unsigned int totalMemory;                   // Total memory in bytes
    unsigned int usedMemory;                    // Used memory in bytes
    unsigned int freeMemory;                    // Free memory in bytes
    unsigned long long uptime;                  // Uptime in seconds
    int numProcesses;                           // Number of processes
    version_t kernelVersion;                    // Kernel version information
    char kernelRelease[64];                     // Kernel release string
    char cpuOEM[16];                            // CPU ID string
    _Bool acpiSupported;                        // Whether ACPI is supported
};

// TODO: Make a complete set of system call wrappers and helper functions to put here

#endif      // SYSTEM_H