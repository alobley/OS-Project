#include <system.h>
//#include <kernel.h>

#define asm __asm__

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

#define do_syscall(num, arg1, arg2, arg3, arg4, arg5) \
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5));

typedef unsigned int _ADDRESS;
#define getresult(x) asm volatile("mov %%eax, %0" : "=r" (x));

int sys_debug(void){
    int result = 0;
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int install_keyboard_handler(KeyboardCallback callback){
    int result = 0;
    do_syscall(SYS_INSTALL_KBD_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int remove_keyboard_handler(KeyboardCallback callback){
    int result = 0;
    do_syscall(SYS_REMOVE_KBD_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int install_timer_handler(timer_callback_t callback, unsigned int interval){
    int result = 0;
    do_syscall(SYS_INSTALL_TIMER_HANDLE, (_ADDRESS)callback, (unsigned int)interval, 0, 0, 0);
    getresult(result);
    return result;
}

int remove_timer_handler(timer_callback_t callback){
    int result = 0;
    do_syscall(SYS_REMOVE_TIMER_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

FILESTATUS write(int fd, const void* buf, unsigned int count){
    int result = 0;
    do_syscall(SYS_WRITE, fd, (_ADDRESS)buf, count, 0, 0);
    getresult(result);
    return result;
}

FILESTATUS read(int fd, void* buf, unsigned int count){
    int result = 0;
    do_syscall(SYS_READ, fd, (_ADDRESS)buf, count, 0, 0);
    getresult(result);
    return result;
}

void exit(int status){
    do_syscall(SYS_EXIT, status, 0, 0, 0, 0);
}

int fork(void){
    int result = 0;
    do_syscall(SYS_FORK, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int exec(const char* path, const char* argv[], const char* envp[], int argc){
    int result = 0;
    do_syscall(SYS_EXEC, (_ADDRESS)path, (_ADDRESS)argv, (_ADDRESS)envp, argc, 0);
    getresult(result);

    return result;
}

int waitpid(pid_t pid){
    int result = 0;
    do_syscall(SYS_WAIT_PID, pid, 0, 0, 0, 0);
    getresult(result);
    return result;
}

pid_t getpid(void){
    int result = 0;
    do_syscall(SYS_GET_PID, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

pid_t getppid(void){
    pid_t result = 0;
    do_syscall(SYS_GET_PPID, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int getcwd(char* buf, unsigned int size){
    int result = 0;
    do_syscall(SYS_GETCWD, (_ADDRESS)buf, size, 0, 0, 0);
    getresult(result);
    return result;
}

int chdir(const char* path){
    int result = 0;
    do_syscall(SYS_CHDIR, (_ADDRESS)path, 0, 0, 0, 0);
    getresult(result);
    return result;
}

file_result open(const char* path){
    file_result result = {0, 0};
    do_syscall(SYS_OPEN, (_ADDRESS)path, 0, 0, 0, 0);
    getresult(result.status);
    asm volatile("mov %%ebx, %0" : "=r" (result.fd));
    return result;
}

FILESTATUS close(int fd){
    FILESTATUS result = 0;
    do_syscall(SYS_CLOSE, fd, 0, 0, 0, 0);
    getresult(result);
    return result;
}

unsigned int seek(int fd, unsigned int* offset, int whence){
    int result = 0;
    do_syscall(SYS_SEEK, fd, offset, whence, 0, 0);
    getresult(result);
    return result;
}

int sleep(unsigned long long milliseconds){
    int result = 0;
    do_syscall(SYS_SLEEP, (unsigned int)(milliseconds & 0xFFFFFFFF), (unsigned int)((milliseconds >> 32) & 0xFFFFFFFF), 0, 0, 0);
    getresult(result);
    return result;
}

int gettime(datetime_t* datetime){
    int result = 0;
    do_syscall(SYS_GET_TIME, datetime, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int kill(pid_t pid){
    int result = 0;
    do_syscall(SYS_KILL, pid, 0, 0, 0, 0);
    getresult(result);
    return result;
}

void yield(void){
    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

int mmap(void* addr, unsigned int length, unsigned int flags){
    int result = 0;
    do_syscall(SYS_MMAP, (unsigned int)addr, length, flags, 0, 0);
    getresult(result);
    return result;
}

int munmap(void* addr, unsigned int length){
    int result = 0;
    do_syscall(SYS_MUNMAP, (unsigned int)addr, length, 0, 0, 0);
    getresult(result);
    return result;
}

int brk(unsigned int size){
    int result = 0;
    do_syscall(SYS_BRK, size, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int mprotect(void* addr, unsigned int length, unsigned int flags){
    int result = 0;
    do_syscall(SYS_MPROTECT, (_ADDRESS)addr, length, flags, 0, 0);
    getresult(result);
    return result;
}

int sys_dumpregs(){
    int result = 0;
    do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int sysinfo(struct sysinfo* info){
    int result = 0;
    do_syscall(SYS_SYSINFO, info, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int open_device(char* path, user_device_t* device){
    int result = 0;
    do_syscall(SYS_OPEN_DEVICE, (_ADDRESS)path, (_ADDRESS)device, 0, 0, 0);
    getresult(result);
    return result;
}

int device_read(int deviceID, void* buffer, unsigned int size){
    int result = 0;
    do_syscall(SYS_DEVICE_READ, deviceID, (_ADDRESS)buffer, size, 0, 0);
    getresult(result);
    return result;
}

int device_write(int deviceID, const void* buffer, unsigned int size){
    int result = 0;
    do_syscall(SYS_DEVICE_WRITE, deviceID, (_ADDRESS)buffer, size, 0, 0);
    getresult(result);
    return result;
}

int shutdown(void){
    int result = 0;
    do_syscall(SYS_SHUTDOWN, 0, 0, 0, 0, 0);
    getresult(result);
    return result;              // In case shutdown fails
}

int reboot(void){
    int result = 0;
    do_syscall(SYS_REBOOT, 0, 0, 0, 0, 0);
    getresult(result);
    return result;              // In case reboot fails
}