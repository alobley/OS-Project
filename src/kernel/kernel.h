#ifndef KERNEL_H
#define KERNEL_H

#include <common.h>
#include <multitasking.h>

struct PACKED gdt_entry_bits {
    uint32_t limit_low : 16;            // Lower 16 bits of the limit
    uint32_t base_low : 24;             // Lower 24 bits of the base
    uint32_t accessed : 1;              // Set if the segment has been accessed
    uint32_t read_write : 1;            // Set if the segment is writable
    uint32_t conforming : 1;            // Set if the segment is conforming
    uint32_t code : 1;                  // Set if the segment is code (1) or data (0)
    uint32_t code_data_segment : 1;     // Should always be 1 for code/data segments
    uint32_t privelige : 2;             // Privilege level (0 = kernel, 3 = user)
    uint32_t present : 1;               // Set if the segment is present
    uint32_t limit_high : 4;            // Upper 4 bits of the limit
    uint32_t available : 1;             // Set if the segment is available for use by the OS
    uint32_t long_mode : 1;             // Set if the segment is in long mode (64-bit)
    uint32_t big : 1;                   // Set if the segment is 32-bit (1) or 16-bit (0)
    uint32_t granularity : 1;           // Set if the segment is granulated (1) or not (0) (1 = 4 KiB page addressing, 0 = byte addressing)
    uint32_t base_high : 8;             // Upper 8 bits of the base
} PACKED;

struct ALIGNED(16) PACKED gdt_ptr {
    uint16_t limit;                      // Size of the GDT - 1
    uintptr_t base;                      // Address of the GDT
} PACKED;

typedef struct PACKED tss_entry {
    uint32_t prevTSS;                    // Previous TSS (if any)
    uintptr_t esp0;                      // Stack pointer for ring 0
    uintptr_t ss0;                       // Stack segment for ring 0
    uintptr_t esp1;                      // Stack pointer for ring 1
    uintptr_t ss1;                       // Stack segment for ring 1
    uintptr_t esp2;                      // Stack pointer for ring 2
    uintptr_t ss2;                       // Stack segment for ring 2
    uintptr_t cr3;                       // Page directory base register
    uintptr_t eip;                       // Instruction pointer
    uint32_t eflags;                     // Flags register
    uintptr_t eax;                       // General-purpose register
    uintptr_t ecx;                       // General-purpose register
    uintptr_t edx;                       // General-purpose register
    uintptr_t ebx;                       // General-purpose register
    uintptr_t esp;                       // Stack pointer
    uintptr_t ebp;                       // Base pointer
    uintptr_t esi;                       // Source index
    uintptr_t edi;                       // Destination index
    uint32_t es;                         // Extra segment
    uint32_t cs;                         // Code segment
    uint32_t ss;                         // Stack segment
    uint32_t ds;                         // Data segment
    uint32_t fs;                         // Additional segment
    uint32_t gs;                         // Additional segment
    uint32_t ldt;                        // Local descriptor table
    uint16_t trap;                       // Trap flag
    uint16_t iomap_base;                // I/O map base address
} PACKED tss_entry_t;

static ALIGNED(16) struct gdt_entry_bits gdt[6];           // One null segment, two ring 0 segments, two ring 3 segments, and one TSS segment
static struct gdt_ptr gdtp;
static struct gdt_entry_bits* ring0Code = &gdt[1];
static struct gdt_entry_bits* ring0Data = &gdt[2];
static struct gdt_entry_bits* ring3Code = &gdt[3];
static struct gdt_entry_bits* ring3Data = &gdt[4];
static struct gdt_entry_bits* tss = &gdt[5];

static tss_entry_t tssEntry;

enum System_Calls {
    SYS_DBG = 1,                            // Debug system call
    SYS_INSTALL_KBD_HANDLE,                 // Install a keyboard callback
    SYS_REMOVE_KBD_HANDLE,                  // Remove a keyboard callback
    SYS_INSTALL_TIMER_HANDLE,               // Install a timer callback
    SYS_REMOVE_TIMER_HANDLE,                // Remove a timer callback
    SYS_WRITE,                              // Write to a file
    SYS_READ,                               // Read from a file
    SYS_EXIT,                               // Exit the current process
    SYS_FORK,                               // Fork the current process
    SYS_EXEC,                               // Execute a new process which replaces the current one
    SYS_WAIT_PID,                           // Wait for a process to exit
    SYS_GET_PID,                            // Get the PID of the current process
    SYS_GET_PPID,                           // Get the PID of the parent process
    SYS_GETCWD,                             // Get the current working directory
    SYS_CHDIR,                              // Change the current working directory
    SYS_OPEN,                               // Open a file
    SYS_CLOSE,                              // Close a file
    SYS_SEEK,                               // Seek to a position in a file
    SYS_SLEEP,                              // Sleep for a certain amount of time
    SYS_GET_TIME,                           // Get the current time
    SYS_KILL,                               // Kill a process
    SYS_YIELD,                              // Voluntarily yield the CPU
    SYS_PIPE,                               // Create a pipe (returns a file descriptor, to remove the pipe, simple use close)
    SYS_REPDUP,                             // Replace and Duplicate a file descriptor - replaces the original file descriptor pointer with a new one (like dup2 on Linux)
    SYS_MMAP,                               // Map memory pages
    SYS_MUNMAP,                             // Unmap memory pages
    SYS_BRK,                                // Change the heap size
    SYS_MPROTECT,                           // Change the protection of memory pages
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
    asm volatile("int $0x30" : : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5))


#endif // KERNEL_H