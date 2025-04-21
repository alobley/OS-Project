/***********************************************************************************************************************************************************************
 * Copyright (c) 2025, xWatexx. All rights reserved.
 * (put some licensing stuff here idk, look at the LICENSE file for details)
 * 
 * This file is part of Dedication OS.
 * This file is the main entry point for the kernel of Dedication OS, a hobby operating system.
 * It is responsible for initializing the system and starting the built-in shell.
 ***********************************************************************************************************************************************************************/
#include <common.h>                     // Common utilities needed for most files
#include <kernel.h>                     // Kernel structures and definitions
#include <gdt.h>                        // GDT structures and definitions

// Kernel subsystems
#include <console.h>                    // VGA console functions
#include <multiboot.h>                  // Multiboot header
#include <interrupts.h>                 // Interrupt handling
#include <fpu.h>                        // Floating point unit handling
#include <time.h>                       // Timer subsystem
#include <paging.h>                     // Paging subsystem
#include <multitasking.h>               // Multitasking subsystem
#include <users.h>                      // User management
#include <vfs.h>                        // Virtual filesystem
#include <devices.h>                    // Device/driver management
#include <system.h>                     // System calls

// Drivers built into the kernel
#include <keyboard.h>                   // Entire 8042 subsystem (not just keyboard - should fix)
#include <tty.h>                        // TTY subsystem
#include <ata.h>                        // PATA driver
#include <fat.h>                        // FAT filesystem driver
#include <pcspkr.h>                     // PC speaker driver
#include <acpi.h>                       // ACPI support

size_t memSize = 0;                     // Local variable for total memory size in bytes
size_t memSizeMiB = 0;                  // Local variable for total memory size in 1024-based megabytes (Do they have a special name?)

// The current version of the kernel (major, minor, patch)
//
// This is my reminder to update the GRUB menu entry when I update this
//
version_t kernelVersion = {0, 12, 1};

// A copy of the multiboot info structure (so that we don't have to mess with paging)
multiboot_info_t mbootCopy;

// Notes:
// - I need lookup tables or bitmaps or something for files and processes for proper management of them (they are linked lists and trees currently, respectively)

/* Short-Term TODO:
 * - Implement a proper command parser in KISh (done)
 * - Finish up the driver/module implementation (done)
 * - Implement file then program loading (done)
 * - Complete the VFS and add full disk drivers  (nearly done, have reading and mounting)
 * - Implement a proper task scheduler and multitasking (current goal and REQUIRED)
 * - Add FAT subdirectory support
 * - Add FAT12/16 support and LFN support
 * - Create a driver to be loaded as a module
 * - Improve the memory manager (done)
 * - Read up on UNIX philosophy and more closely follow it (in progress)
 * - Make a new filesystem driver (which one? I was thinking ISO9660 or EXT2)
*/


/* Most recent accomplishments:
 * - Program loading and execution
 * - Proper implementation of SYS_EXEC and SYS_EXIT, allowing for proper singletasking and context switching
 * - Finally added the user mode GDT and proper iret-based context switching (after much difficulty)
 * - Shell in userland
*/

// Get the pointers for the stack from the assembly bootstrapping code
extern uint8_t stack_begin;
extern uint8_t stack;

extern uint8_t intstack_base;
extern uint8_t intstack;

static ALIGNED(16) struct GDTPointer gdtp;
static ALIGNED(16) struct GDT gdt;

extern void LoadNewGDT(uint32_t gdtp);
void FlushTSS(uint16_t tss_segment) {
    asm volatile("ltr %0" : : "r" (tss_segment));
}

// Create a new, better GDT than the one defined in assembly
void CreateGDT(){
    cli
    memset(&tss, 0, sizeof(struct TSS_Entry));                 // Clear the TSS

    // Create the GDT descriptors for the kernel and user segments
    gdt.null = GDT_NULL_SEGMENT;
    gdt.kernelCode = CreateDescriptor(0, 0xFFFFFFFF, GDT_CODE_PL0);
    gdt.kernelData = CreateDescriptor(0, 0xFFFFFFFF, GDT_DATA_PL0);
    gdt.userCode = CreateDescriptor(0, 0xFFFFFFFF, GDT_CODE_PL3);
    gdt.userData = CreateDescriptor(0, 0xFFFFFFFF, GDT_DATA_PL3);
    gdt.tss = CreateDescriptor((uint32_t)&tss, sizeof(struct TSS_Entry), GDT_TSS_SEGMENT);

    // Set the GDT pointer to the GDT
    gdtp.limit = sizeof(struct GDT) - 1;
    gdtp.base = (uint32_t)&gdt;

    // Set the only required entries of the TSS
    tss.ss0 = GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_DATA);
    tss.esp0 = (uint32_t)&intstack;                                // Should this be replaced at any point? I'd assume so.

    // Load t he GDT
    LoadNewGDT((uint32_t)&gdtp);

    // Load the TSS
    FlushTSS(GDT_RING0_SEGMENT_POINTER(GDT_TSS));
    sti
}

extern int shell();

device_t* stdin = NULL;
device_t* stdout = NULL;
device_t* stderr = NULL;

/// @brief The main entry point for the kernel
/// @param magic The magic number provided by the bootloader (must be multiboot-compliant)
/// @param mbootInfo A pointer to the multiboot info structure provided by the bootloader
/// @return Doesn't return
NORET void kmain(uint32_t magic, multiboot_info_t* mbootInfo){
    if(magic != MULTIBOOT2_MAGIC && magic != MULTIBOOT_MAGIC){
        // Check if the magic number is valid (if not, boot may have failed)
        printk("KERNEL PANIC: Invalid multiboot magic number: 0x%x\n", magic);
        STOP
    }

    // Parse the memory map and map it to a bitmap for easier page frame allocation/deallocation
    printk("Parsing memory map...\n");
    MapBitmap(mbootInfo->mmap_addr, mbootInfo->mmap_length);

    // The result variable that initialization functions will return with
    int result = 0;

    // Initialization - first stage

    CreateGDT();            // Create the permanent GDT

    // Get the memory size in bytes and MiB and save those values
    memSize = ((mbootInfo->mem_upper + mbootInfo->mem_lower) + 1024) * 1024;
    memSizeMiB = memSize / 1024 / 1024;

    // Print bootloader and memory information
    printk("Bootloader: %s\n", mbootInfo->boot_loader_name);
    printk("Multiboot magic: 0x%x\n", magic);
    printk("Memory: %u MiB\n", memSizeMiB);

    InitIDT();              // Initialize the IDT
    InitISR();              // Initialize the ISRs
    InitFPU();              // Initialize the FPU
    InitIRQ();              // Initialize the IRQs
    InitTimer();            // Initialize the timer

    InitializeACPI();       // Get ACPI information

    // Copy the multiboot info structure
    memcpy(&mbootCopy, mbootInfo, sizeof(multiboot_info_t));

    // Initialization - second stage

    // Initialize the page allocator and page the kernel
    printk("Paging memory...\n");
    if(PageKernel(memSize) != PAGE_OK){
        printk("KERNEL PANIC: PAGING WAS NOT ENABLED!\n");
        STOP
    }

    printk("Used (paged) memory: %u MiB\n", ((mappedPages * PAGE_SIZE) / 1024) / 1024);

    // Initialize the heap allocator
    printk("Initializing heap allocator...\n");
    InitializeAllocator();

    printk("Testing system calls...\n");
    // Do a debug syscall to test the syscall interface
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);
    printk("Syscall debug test completed successfully!\n");

    //uint32_t usedMem = totalPages * PAGE_SIZE;                  // Calculate the current amount of memory used by the system (this will change - maybe add a timer handler to update it?)
    //printk("Used memory: %d MiB\n", usedMem / 1024 / 1024);     // Print the amount of used memory in MiB

    // Stress test the memory allocator
    printk("Stress testing the heap allocator...\n");
    for(int i = 1; i < 100; i++){
        // Allocate an increasingly large amount of memory
        uint8_t* test = halloc(PAGE_SIZE * i);
        if(test == NULL){
            // Memory allocation failed
            printk("KERNEL PANIC: Heap allocation error!\n");
            STOP
        }
    
        memset(test, 1, (PAGE_SIZE * i));
    
        hfree(test);
    }

    printk("Memory stress test completed successfully!\n");

    // Get the system time from the CMOS
    printk("Getting system time from CMOS...\n");
    SetTime();

    // Test the PC speaker (removed because it is loud af)
    //printk("Testing PC speaker...\n");
    //PCSP_Beep();

    // Initialization - third stage

    // Initialize the virtual filesystem
    result = InitializeVfs(mbootInfo);
    if(result != STANDARD_SUCCESS){
        // If there was a failure, the system can't continue as the VFS is needed for most operations
        printk("KERNEL PANIC: Failed to initialize VFS!\n");
        STOP
    }
    printk("VFS initialized successfully!\n");

    // Initialize the device and driver registries
    printk("Creating system registries...\n");
    if(CreateDeviceRegistry() != STANDARD_SUCCESS){
        // If there was a failure, the system can't continue as drivers can't be loaded
        printk("KERNEL PANIC: Failed to create device registry!\n");
        STOP
    }
    if(CreateDriverRegistry() != STANDARD_SUCCESS){
        printk("KERNEL PANIC: Failed to create driver registry!\n");
        STOP
    }
    printk("System registries created successfully!\n");


    // Make STDIN, STDOUT, and STDERR
    // I just recently completely overhauled half the system, so the old code no longer worked
    stdin = halloc(sizeof(device_t));
    if(stdin == NULL){
        // Failed to allocate memory for stdin
        printk("KERNEL PANIC: Failed to allocate memory for stdin!\n");
        STOP
    }
    memset(stdin, 0, sizeof(device_t));
    stdin->name = "stdin";
    stdin->class = DEVICE_CLASS_CHAR | DEVICE_CLASS_INPUT | DEVICE_CLASS_VIRTUAL;
    RegisterDevice(stdin, "/dev/stdin", S_IROTH);

    stdout = halloc(sizeof(device_t));
    if(stdout == NULL){
        // Failed to allocate memory for stdin
        printk("KERNEL PANIC: Failed to allocate memory for stdin!\n");
        STOP
    }
    memset(stdout, 0, sizeof(device_t));
    stdout->name = "stdout";
    stdout->class = DEVICE_CLASS_CHAR | DEVICE_CLASS_VIRTUAL;
    RegisterDevice(stdout, "/dev/stdout", S_IROTH);

    stderr = halloc(sizeof(device_t));
    if(stderr == NULL){
        // Failed to allocate memory for stdin
        printk("KERNEL PANIC: Failed to allocate memory for stdin!\n");
        STOP
    }
    memset(stderr, 0, sizeof(device_t));
    stderr->name = "stderr";
    stderr->class = DEVICE_CLASS_CHAR | DEVICE_CLASS_VIRTUAL;
    RegisterDevice(stderr, "/dev/stderr", S_IROTH);

    // Set the current process to the kernel (we don't need a proper context since the kernel won't actually "run" per se)
    // Create the kernel's PCB
    kernelPCB = CreateProcess("syscore", 0, RESOURCE_LIMITS(0, 0, 0, 0, 0, 0, 0, 0, 0), ROOT_UID, 0, 0, 0, VfsFindNode(VFS_ROOT), NULL);
    if(kernelPCB == NULL){
        // No PCB means no multitasking - the kernel can't run
        printk("KERNEL PANIC: Failed to create kernel PCB!\n");
        STOP
    }
    SetCurrentProcess(kernelPCB);                                   // Set the current process to the kernel PCB
    printk("Kernel PCB created successfully!\n");

    InitializeKeyboard();                                           // Initialize the keyboard driver (TODO: turn into a real driver)
    InitializeTTY();                                                // Initialize the TTY subsystem
    
    InitializeAta();                                                // Initialize the built-in PATA driver (will likely be replaced by a module later in boot when the filesystem is mounted)

    printk("Initializing FAT driver...\n");

    // Initialize the FAT driver
    if(InitializeFAT() != DRIVER_SUCCESS){
        // If there was a failure, the system can't continue as the FAT driver is needed for most operations
        printk("KERNEL PANIC: Failed to initialize FAT driver!\n");
        STOP
    }

    printk("FAT driver initialized successfully! Mounting filesystems...\n");

    // Search for disk devices and assign them filesystem drivers (if any)
    for(size_t i = 0; i < GetNumDevices(); i++){
        device_t* device = GetDeviceByID(i);
        if(device == NULL || device->driver == NULL){
            continue;
        }

        if(device->class & DEVICE_CLASS_BLOCK && device->type & DEVICE_TYPE_STORAGE){
            // This is a block device and a storage device
            FindFsDriver(device);
        }
    }

    // Find the root filesystem and mount it
    for(size_t i = 0; i < GetNumDevices(); i++){
        device_t* device = GetDeviceByID(i);
        if(device == NULL){
            continue;
        }

        if(device->type == (DEVICE_TYPE_STORAGE | DEVICE_TYPE_PARTITION | DEVICE_TYPE_FILESYSTEM)){
            // This is a block device and a storage device
            fs_driver_t* driver = (fs_driver_t*)device->driver;
            if(driver != NULL && (driver->driver.type & DEVICE_TYPE_FILESYSTEM)){
                // The driver was found, mount the filesystem
                //printk("Mounting filesystem on device %s...\n", device->name);
                driver->mount(device->id, "/root");
            }
        }
    }
    printk("Root filesystem mounted successfully!\n");

    // Initialization - fourth stage

    // Load a users file and create the users...

    // Other final initialization steps...

    // Initialization complete - start the shell

    printk("Boot complete. Starting shell...\n");

    // Search for the shell in the root directory and execute it (Should I also try the chroot? It might be a good idea to use init as well...)
    //pcb_t* shellPCB = Load("/root/SHELL.ELF", NULL, NULL, 0);
    //ScheduleProcess(shellPCB);
    // Do a SYS_YIELD
    //do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);

    /* This was to test the new driver interface and ATA driver. It works.
    vfs_node_t* node = VfsFindNode("/dev/pat0");
    device_t* device = node->device;

    uint64_t* buffer = halloc(512);

    // Read a total of 1 sector starting at sector 0
    buffer[0] = 0;
    buffer[1] = 1;
    device->ops.read(device->id, buffer, 512, 0);

    uint16_t* data = (uint16_t*)buffer;

    if(data[255] == 0xAA55){
        // The first sector is a valid boot sector
        printk("Valid boot sector found!\n");
        printk("Data at the end of the sector: 0x%x\n", data[255]);
        STOP
    }else{
        // The first sector is not a valid boot sector
        printk("Invalid boot sector found!\n");
        STOP
    }
    */

    shell();

    printk("ERROR: No shell or init loaded!");

    for(;;){
        // Infinite halt
        hlt
    }

    // This should never be reached
    STOP
    UNREACHABLE
}