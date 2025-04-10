#ifndef SYSTEM_H
#define SYSTEM_H

// I would like to keep this but I should also make a unistd.h

// Header file containing macros and functions for system calls and other system-level functions
// This file is meant to be used both in the kernel and in userland

#define ANSI_ESCAPE "\033[2J\033[H"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// More readable success and failure codes for functions
#define STANDARD_FAILURE -1
#define STANDARD_SUCCESS 0

// System call returns
#define SYSCALL_SUCCESS 0
#define SYSCALL_FAILURE -1
#define SYSCALL_ACCESS_DENIED -2
#define SYSCALL_INVALID_ARGUMENT -3
#define SYSCALL_NOT_FOUND -4
#define SYSCALL_OUT_OF_MEMORY -5
#define SYSCALL_TASKING_FAILURE -999              // Specifically for syscalls like exec
#define SYSCALL_FAULT_DETECTED -1000

typedef struct Date_Time {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} datetime_t;

// Keyboard events located here (should probably be moved to a separate header file or something)
typedef struct KeyboardEvent {
    unsigned char scanCode;
    char ascii;
    _Bool keyUp;
} KeyboardEvent_t;

typedef struct Version {
    unsigned char major;
    unsigned char minor;
    unsigned char patch;
} version_t;

typedef void (*timer_callback_t)(void);

typedef int device_id_t;
typedef unsigned short vendor_id_t;

typedef enum {
    DEVICE_TYPE_ROOTDEV,                        // Motherboards and daughterboards (essentially the root of the device tree) (needed?)
    DEVICE_TYPE_BLOCK,                          // Block device (i.e. hard drive, SSD)
    DEVICE_TYPE_CHAR,                           // Character device (i.e. keyboard, mouse, serial port)
    DEVICE_TYPE_NETWORK,                        // Network device (i.e. Ethernet, Wi-Fi)
    DEVICE_TYPE_INPUT,                          // Input device (i.e. keyboard, mouse)
    DEVICE_TYPE_DISPLAY,                        // Display device (i.e. GPU, framebuffer)
    DEVICE_TYPE_SOUND,                          // Sound device (i.e. sound card)
    DEVICE_TYPE_USB,                            // USB device
    DEVICE_TYPE_PCI,                            // PCI device
    DEVICE_TYPE_PCIe,                           // PCIe device
    DEVICE_TYPE_VIRTUAL,                        // Virtual device (i.e. a ramdisk)
    DEVICE_TYPE_FILESYSTEM,                     // Filesystem device (i.e. a mounted filesystem)
    DEVICE_TYPE_OTHER,                          // Other device type (not defined by the kernel and may not be supported)
    DEVICE_TYPE_UNKNOWN,                        // Unknown device type
} DEVICE_TYPE;

// A more compact device struct for userland applications
typedef struct user_device {
    device_id_t id;                             // Device ID
    vendor_id_t vendorId;                       // Vendor ID
    const char* name;                           // Name of the device
    const char* description;                    // Description of the device
    const char* devName;                        // String that will be presented in /dev (i.e. kb0, sda, etc.)
    char last_error[64];                        // Last error message from the device in human-readable format
    DEVICE_TYPE type;                           // Type of device
    void* deviceInfo;                           // Device-specific data (if any)
    struct user_device* parent;                 // Pointer to the parent device (if any)
    struct user_device* next;                   // Pointer to the next user device (if any)
    struct user_device* firstChild;             // Pointer to the first child device (if any)
} user_device_t;

typedef void (*KeyboardCallback)(KeyboardEvent_t event);

typedef unsigned int pid_t;

struct Node_Data {
    unsigned int size;                         // Size of the node in bytes
    unsigned int type;                         // Type of the node (file, directory, etc.)
    unsigned int permissions;                  // Permissions for the node (read, write, etc.)
    pid_t owner;                               // Owner of the node
    char name[12];                             // Name of the node (8.3 format)
};

#define NODE_TYPE_FILE 0
#define NODE_TYPE_DIRECTORY 1
#define NODE_TYPE_DEVICE 2
#define NODE_TYPE_OTHER 10

// For peeking a mutex
typedef int MUTEXSTATUS;
#define MUTEX_IS_UNLOCKED 0
#define MUTEX_AQUIRED 1
#define MUTEX_IS_LOCKED -1
#define MUTEX_FAILURE -2

typedef int FILESTATUS;
#define FILE_INVALID_OFFSET -1
#define FILE_LOCKED -2
#define FILE_NOT_FOUND -3
#define FILE_OPEN 1
#define FILE_CLOSED 2
#define FILE_EXISTS 3
#define FILE_READ_ONLY 4
#define FILE_WRITE_ONLY 5
#define FILE_WRITE_SUCCESS 6
#define FILE_READ_SUCCESS 7
#define FILE_READ_INCOMPLETE 8
#define FILE_NOT_RESIZEABLE 9

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
    FILESTATUS status;          // Result of the open operation
    int fd;                     // File descriptor
} file_result;

struct sysinfo {
    unsigned int totalMemory;             // Total memory in bytes
    unsigned int usedMemory;              // Used memory in bytes
    unsigned int freeMemory;              // Free memory in bytes
    unsigned long long uptime;            // Uptime in seconds
    int numProcesses;                     // Number of processes
    version_t kernelVersion;              // Kernel version string
    char kernelRelease[64];               // Kernel release string
    char cpuID[16];                       // CPU ID string
    _Bool acpiSupported;                  // Whether ACPI is supported
};

// Debug system call - prints a confirmation to the screen
int sys_debug(void);

// Install a keyboard handler to /dev/kb0 (TODO: support more keyboards)
int install_keyboard_handler(KeyboardCallback callback);

// Remove a keyboard handler from /dev/kb0
int remove_keyboard_handler(KeyboardCallback callback);

int install_timer_handler(timer_callback_t callback, unsigned int interval);

int remove_timer_handler(timer_callback_t callback);

// Write to a file descriptor
FILESTATUS write(int fd, const void* buf, unsigned int count);

// Read from a file descriptor
FILESTATUS read(int fd, void* buf, unsigned int count);

// Read file data
FILESTATUS stat(const char* directory, unsigned int number, struct Node_Data* buf);

// Exit the current process
void exit(int status);

// Fork the current process (returns the PID of the child process)
int fork(void);

// Execute a new process (replaces the current one)
int exec(const char* path, const char* argv[], const char* envp[], int argc);

// Wait for a process to exit
int waitpid(pid_t pid);

// Get the PID of the current process
pid_t getpid(void);

// Get the PID of the parent process
pid_t getppid(void);

// Get the current working directory
int getcwd(char* buf, unsigned int size);

// Change the current working directory
int chdir(const char* path);

// Open a file
file_result open(const char* path);

// Close a file
FILESTATUS close(int fd);

// Seek to a position in a file
unsigned int seek(int fd, unsigned int* offset, int whence);

// Sleep for a certain amount of time (in milliseconds)
int sleep(unsigned long long milliseconds);

// Get the current date and time
int gettime(datetime_t* datetime);

// Kill a process owned by the current user (if running as root, any non-kernel process can be killed)
int kill(pid_t pid);

// Yield the CPU for cooperative multitasking
void yield(void);

// Page a new section of memory to a specific virtual address and of a specific size
int mmap(void* addr, unsigned int length, unsigned int flags);

// Unpage a section of memory owned by the process (excludes kernel and processes have their own page directories)
int munmap(void* addr, unsigned int length);

// Change the size of this application's heap
int brk(unsigned int size);

// Change the flags of an area of the application's memory
int mprotect(void* addr, unsigned int length, unsigned int flags);

int sys_dumpregs();

int sysinfo(struct sysinfo* info);

// Open a device from the VFS
int open_device(char* path, user_device_t* device);

// Call a device's read function
int device_read(int deviceID, void* buffer, unsigned int size);

// Call a device's write function
int device_write(int deviceID, const void* buffer, unsigned int size);

// Priveliged system call. Shuts down the system.
int shutdown(void);

// Priveliged system call. Reboots the system.
int reboot(void);

#endif      // SYSTEM_H