/***********************************************************************************************************************************************************************
 * Copyright (c) 2025, xWatexx. All rights reserved.
 * (put some licensing stuff here idk, look at the LICENSE file for details)
 * 
 * This file is part of Dedication OS.
 * This file is the main entry point for the kernel of Dedication OS, a hobby operating system.
 * It is responsible for initializing the system and starting the built-in shell.
 ***********************************************************************************************************************************************************************/
#include <common.h>                     // Common utilities needed for most files

// These commented out includes are now in common.h
//#include <util.h>
//#include <stdint.h>
//#include <string.h>
//#include <stddef.h>
//#include <alloc.h>
//#include <kernel.h>

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
#include <mbr.h>                        // MBR structures
#include <fat.h>                        // FAT filesystem driver
#include <pcspkr.h>                     // PC speaker driver
#include <acpi.h>                       // ACPI support

size_t memSize = 0;                     // Local variable for total memory size in bytes
size_t memSizeMiB = 0;                  // Local variable for total memory size in 1024-based megabytes (Do they have a special name?)

// Reference the built-in shell
extern int shell(void);

// The current version of the kernel (major, minor, patch) - 0.5.0
version_t kernelVersion = {0, 6, 0};

// A copy of the multiboot info structure (so that we don't have to mess with paging)
multiboot_info_t mbootCopy;

// Notes:
// - I need lookup tables for files and processes for proper management of them

/* Short-Term TODO:
 * - Implement a proper command parser in KISh (done)
 * - Finish up the driver/module implementation (in progress)
 * - Implement initrd (optional)
 * - Create a driver to be loaded as a module
 * - Improve the memory manager
 * - Complete the VFS and add full disk drivers  (nearly done, have reading and mounting)
 * - Implement file then program loading (done)
 * - Implement a proper task scheduler
 * - Read up on UNIX philosophy and more closely follow it (in progress)
*/

// The kernel's process control block
pcb_t* kernelPCB = NULL;

// Get the pointers for the stack from the assembly bootstrapping code
extern uint8_t stack_begin;
extern uint8_t stack;

/// @brief The main entry point for the kernel
/// @param magic The magic number provided by the bootloader (must be multiboot-compliant)
/// @param mbootInfo A pointer to the multiboot info structure provided by the bootloader
/// @return Doesn't return
NORET void kernel_main(uint32_t magic, multiboot_info_t* mbootInfo){
    if(magic != MULTIBOOT2_MAGIC && magic != MULTIBOOT_MAGIC){
        // Check if the magic number is valid (if not, boot may have failed)
        printk("KERNEL PANIC: Invalid multiboot magic number: 0x%x\n", magic);
        STOP
    }

    // The result variable that initialization functions will return with
    int result = 0;

    // Initialization - first stage

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

    // Parse the memory map and map it to a bitmap for easier page frame allocation/deallocation
    printk("Parsing memory map...\n");
    MapBitmap(memSize, mbootInfo->mmap_addr, mbootInfo->mmap_length / sizeof(mmap_entry_t));

    // Initialize the page allocator and page the kernel
    printk("Paging memory...\n");
    PageKernel(memSize);

    // Initialize the heap allocator
    printk("Initializing heap allocator...\n");
    InitializeAllocator();

    // Do a debug syscall to test the syscall interface
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);

    uint32_t usedMem = totalPages * PAGE_SIZE;                  // Calculate the current amount of memory used by the system (this will change - maybe add a timer handler to update it?)
    printk("Used memory: %d MiB\n", usedMem / 1024 / 1024);     // Print the amount of used memory in MiB

    // Stress test the memory allocator
    printk("Stress testing the heap allocator...\n");
    for(int i = 1; i < 1000; i++){
        // Perform 1,000 allocations and deallocations of 6 pages (24 KiB) each - this should be stable enough to not cause any issues
        uint8_t* test = halloc(PAGE_SIZE * 6);
        if(test == NULL){
            // Memory allocation failed
            printk("KERNEL PANIC: Heap allocation error!\n");
            STOP
        }

        memset(test, 1, PAGE_SIZE * 6);                         // Write to the memory to ensure it is allocated (if not it may corrupt memory or cause a page fault)

        hfree(test);
    }

    printk("Memory stress test completed successfully!\n");

    // Get the system time from the CMOS
    printk("Getting system time from CMOS...\n");
    SetTime();

    // Test the timer (removed for faster debugging - the timer works)
    //printk("Testing the timer...\n");
    //sleep(1000);

    // Initialization - third stage

    // Initialize the device registry
    printk("Creating device registry...\n");
    if(CreateDeviceRegistry() != DRIVER_SUCCESS){
        // If there was a failure, the system can't continue as drivers can't be loaded
        printk("KERNEL PANIC: Failed to create device registry!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    printk("Device registry created successfully!\n");

    // Initialize the virtual filesystem
    result = InitializeVfs(mbootInfo);
    if(result != STANDARD_SUCCESS){
        // If there was a failure, the system can't continue as the VFS is needed for most operations
        printk("KERNEL PANIC: Failed to initialize VFS!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    printk("VFS initialized successfully!\n");

    // Test the PC speaker (removed because it is loud af)
    //PCSP_Beep();


    // NOTE: The file contexts of files should remain constant. Only the file descriptors should change. However, that is for another time.
    // Create STDIN at /dev/stdin
    vfs_node_t* stdin = VfsMakeNode("stdin", false, false, false, false, 0, 0, ROOT_UID, NULL);
    if(stdin == NULL){
        printk("KERNEL PANIC: Failed to create stdin node!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    result = VfsAddChild(VfsFindNode("/dev"), stdin);
    if(result != STANDARD_SUCCESS){
        printk("KERNEL PANIC: Failed to add stdin node to /dev!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }

    // Create STDOUT at /dev/stdout
    vfs_node_t* stdout = VfsMakeNode("stdout", false, false, false, false, 0, 0, ROOT_UID, NULL);
    if(stdout == NULL){
        printk("KERNEL PANIC: Failed to create stdout node!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    result = VfsAddChild(VfsFindNode("/dev"), stdout);
    if(result != STANDARD_SUCCESS){
        printk("KERNEL PANIC: Failed to add stdout node to /dev!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }

    // Create STDERR at /dev/stderr
    vfs_node_t* stderr = VfsMakeNode("stderr", false, false, false, false, 0, 0, ROOT_UID, NULL);
    if(stderr == NULL){
        printk("KERNEL PANIC: Failed to create stderr node!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    result = VfsAddChild(VfsFindNode("/dev"), stderr);
    if(result != STANDARD_SUCCESS){
        printk("KERNEL PANIC: Failed to add stderr node to /dev!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    printk("STDIN, STDOUT, and STDERR created successfully!\n");

    // Set the current process to the kernel (we don't need a proper context since the kernel won't actually "run" per se)
    // Create the kernel's PCB
    kernelPCB = CreateProcess(NULL, "syscore", GetFullPath(VfsFindNode(VFS_ROOT)), ROOT_UID, true, true, true, KERNEL, 0, NULL);
    if(kernelPCB == NULL){
        // No PCB means no multitasking - the kernel can't run
        printk("KERNEL PANIC: Failed to create kernel PCB!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    kernelPCB->pageDirectory = (physaddr_t)currentPageDir;          // The physical and virtual address of the page directory are the same
    kernelPCB->stackBase = (uintptr_t)&stack_begin;                 // Set the stack base to the top of the stack
    kernelPCB->stackTop = (uintptr_t)&stack;                        // Set the stack top to the bottom of the stack
    kernelPCB->heapBase = heapStart;                                // Set the heap base to the start of the heap
    kernelPCB->heapEnd = memSize;                                   // Set the heap end to the total memory size of the system (the kernel has access to everything after all) (change?)
    printk("Kernel PCB created successfully!\n");
    SetCurrentProcess(kernelPCB);

    InitializeKeyboard();                                           // Initialize the keyboard driver
    InitializeTTY();                                                // Initialize the TTY subsystem
    InitializeAta();                                                // Initialize the built-in PATA driver (will likely be replaced by a module later in boot when the filesystem is mounted)

    // Read from the first sector of the first disk to test the built-in PATA driver
    device_t* ataDevice = GetDeviceFromVfs("/dev/pat0");            // Get the first ATA device from the VFS

    // Allocate a buffer to read into
    uint8_t* buffer = halloc(((blkdev_t*)ataDevice->deviceInfo)->sectorSize);
    if(buffer == NULL){
        printk("Failed to allocate memory for buffer!\n");
        STOP
    }
    memset(buffer, 0, 512);                                         // Clear the buffer

    // Get the sector to read from and the amount of sectors to read. These are stored in the buffer so that different commands can easily be sent to different devices.
    uint64_t* buf = (uint64_t*)buffer;
    buf[0] = 0;                                                     // Set the LBA to read
    buf[1] = 1;                                                     // Set the sector count to 1

    // Perform the read. Use a syscall instead of calling the driver directly to ensure this system call works
    do_syscall(SYS_DEVICE_READ, (uint32_t)ataDevice->id, (uint32_t)buffer, 512, 0, 0);

    // Get the result of the system call
    asm volatile("mov %%eax, %0" : "=r" (result));

    // Check if the read was successful
    if(result != DRIVER_SUCCESS){
        printk("Failed to read from disk! Error code: %d\n", result);
        STOP
    }

    printk("Successfully read from disk!\n");

    // Check if the disk is an MBR disk by checking the magic number in the first sector
    mbr_t* mbr = (mbr_t*)buffer;
    if(IsValidMBR(mbr)){
        printk("Valid MBR found!\n");
        printk("MBR Signature: 0x%x\n", mbr->signature);
    }else{
        printk("This disk may not be MBR!\n");
    }

    // Free the buffer after use
    hfree(buffer);

    // Perform the actual check for all disks in the system to see if they have an MBR partition scheme
    // Note - MBR partitions are not present on all disks. If so, the filesystem driver will have to implement it itself
    while(ataDevice != NULL){
        // Get the partitions, if any, from this disk
        result = GetPartitionsFromMBR(ataDevice);
        if(result != DRIVER_SUCCESS){
            // If there were no partitions on the disk, print that and continue
            printk("Could not detect partitions.\n");
        }else{
            // If the partitions were successfully retrieved, they were added to the devices in the device tree
            printk("Partitions successfully retrieved from MBR!\n");
        }

        // Go to the next ATA device and repeat
        ataDevice = ataDevice->next;
        while(ataDevice != NULL || ataDevice->type != DEVICE_TYPE_BLOCK){
            ataDevice = ataDevice->next;
            if(ataDevice == NULL){
                break;
            }
        }
    }

    // Initialize the FAT driver
    InitializeFAT();

    // Initialization - fourth stage


    // Assign the FAT driver by probing the disks
    ataDevice = GetDeviceFromVfs("/dev/pat0");
    
    // Probe the drivers to find filesystem support for the disks
    while(ataDevice != NULL){
        driver_t* fsDriver = FindDriver(ataDevice, DEVICE_TYPE_FILESYSTEM);
        // If the driver aquired the device, it is expected to have made a filesystem device
        if(fsDriver == NULL){
            printk("Driver not found for device %s\n", ataDevice->devName);
        }else{
            // Just mount any found filesystem for now (this will always mount the LAST valid one)
            if(((filesystem_t*)ataDevice->firstChild->deviceInfo)->mount(ataDevice->firstChild, "/root") == DRIVER_SUCCESS){
                printk("Filesystem mounted successfully!\n");
            }else{
                printk("Failed to mount filesystem!\n");
            }
        }

        // Go to the next ATA device and repeat
        ataDevice = ataDevice->next;
        while(ataDevice != NULL || ataDevice->type != DEVICE_TYPE_BLOCK){
            ataDevice = ataDevice->next;
            if(ataDevice == NULL){
                break;
            }
        }
    }

    // Load a users file and create the users...

    // Other final initialization steps...

    // Initialization complete - start the system

    // Create a dummy PCB for the shell
    pcb_t* shellPCB = CreateProcess(shell, "shell", GetFullPath(VfsFindNode(VFS_ROOT)), ROOT_UID, true, false, true, NORMAL, PROCESS_DEFAULT_TIME_SLICE, kernelPCB);
    // The kernel is the steward of all processes
    kernelPCB->firstChild = shellPCB;

    SetCurrentProcess(shellPCB);
    
    // Jump to the built-in debug shell
    // TODO:
    // - Load the shell from the filesystem
    // - Make the shell a userland application
    result = shellPCB->EntryPoint();

    DestroyProcess(shellPCB);
    SetCurrentProcess(kernelPCB);

    // Schedule the first process
    Scheduler();

    for(;;){
        // Infinite halt
        hlt
    }

    // This should never be reached
    STOP
    UNREACHABLE
}