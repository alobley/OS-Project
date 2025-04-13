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
#include <mbr.h>                        // MBR structures
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

// The kernel's process control block
volatile pcb_t* kernelPCB = NULL;

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

    // Do a debug syscall to test the syscall interface
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);

    //uint32_t usedMem = totalPages * PAGE_SIZE;                  // Calculate the current amount of memory used by the system (this will change - maybe add a timer handler to update it?)
    //printk("Used memory: %d MiB\n", usedMem / 1024 / 1024);     // Print the amount of used memory in MiB

    // Stress test the memory allocator
    printk("Stress testing the heap allocator...\n");
    for(int i = 1; i < 100; i++){
        // Allocate an increasingly large amount of memory
        //printk("Allocating %u bytes\n", PAGE_SIZE * i);
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

    //STOP

    // Set the current process to the kernel (we don't need a proper context since the kernel won't actually "run" per se)
    // Create the kernel's PCB
    kernelPCB = CreateProcess(NULL, "syscore", GetFullPath(VfsFindNode(VFS_ROOT)), ROOT_UID, true, true, true, KERNEL, 0, NULL);
    if(kernelPCB == NULL){
        // No PCB means no multitasking - the kernel can't run
        printk("KERNEL PANIC: Failed to create kernel PCB!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    //kernelPCB->pageDirectory = (physaddr_t)currentPageDir;          // The physical and virtual address of the page directory are the same
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

    // Search for non-removable devices
    while(ataDevice != NULL){
        if(ataDevice->type == DEVICE_TYPE_BLOCK && ((blkdev_t*)ataDevice->deviceInfo)->removable == false){
            break;
        }
        ataDevice = ataDevice->next;
    }

    if(ataDevice == NULL){
        printk("No ATA device found!\n");
        STOP
    }
    if(((blkdev_t*)ataDevice->deviceInfo)->removable){
        printk("ATA device is removable!\n");
        STOP
    }
    printk("ATA device found: %s\n", ataDevice->devName);

    // Get the sector to read from and the amount of sectors to read. These are stored in the buffer so that different commands can easily be sent to different devices.
    uint64_t* buf = (uint64_t*)buffer;
    buf[0] = 0;                                                     // Set the LBA to read
    buf[1] = 1;                                                     // Set the sector count to 1
    file_result devRes = open(ataDevice->path, 0);                  // Open the device for reading
    if(read(devRes.fd, buf, ((blkdev_t*)ataDevice->deviceInfo)->sectorSize) != FILE_READ_SUCCESS){
        printk("Failed to read from disk!\n");
        STOP
    }

   // STOP

    printk("Successfully read from disk!\n");

    // Check if the disk is an MBR disk by checking the magic number in the first sector
    mbr_t* mbr = (mbr_t*)buffer;
    if(IsValidMBR(mbr)){
        printk("Valid MBR found!\n");
        printk("MBR Signature: 0x%x\n", mbr->signature);
    }else{
        printk("This disk may not be MBR!\n");
    }

    //STOP

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
            //STOP
        }else{
            // If the partitions were successfully retrieved, they were added to the devices in the device tree
            printk("Partitions successfully retrieved from MBR!\n");
        }

        // Go to the next ATA device and repeat
        ataDevice = ataDevice->next;
        while(ataDevice != NULL && ataDevice->type != DEVICE_TYPE_BLOCK){
            ataDevice = ataDevice->next;
            if(ataDevice == NULL){
                break;
            }
        }
    }

    printk("Initializing FAT driver...\n");
    //STOP

    // Initialize the FAT driver
    InitializeFAT();

    printk("FAT driver initialized successfully!\n");

    // Initialization - fourth stage


    // Assign the FAT driver by probing the disks
    ataDevice = GetDeviceFromVfs("/dev/pat0");
    
    printk("Mounting filesystems...\n");
    // Probe the drivers to find filesystem support for the disks
    while(ataDevice != NULL){
        printk("Looking for driver for device %s\n", ataDevice->devName);
        driver_t* fsDriver = FindDriver(ataDevice, DEVICE_TYPE_FILESYSTEM);
        // If the driver aquired the device, it is expected to have made a filesystem device
        if(fsDriver == NULL){
            printk("Driver not found for device %s\n", ataDevice->devName);
        }else if(((blkdev_t*)ataDevice->deviceInfo)->removable == false){
            // Just mount any found non-removable filesystem for now (this will always mount the LAST valid one)
            if(((filesystem_t*)ataDevice->firstChild->deviceInfo)->mount(ataDevice->firstChild, "/root") == DRIVER_SUCCESS){
                printk("Filesystem mounted successfully!\n");
                break;
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

    close(devRes.fd);

    // Configure the VGA hardware

    printk("Boot complete. Starting shell...\n");

    // Load a users file and create the users...

    // Other final initialization steps...

    // Initialization complete - start the shell

    SetCurrentProcess(kernelPCB);                               // Set the current process to the kernel PCB

    // Search for the shell in the root directory and execute it
    exec("/root/SHELL.ELF", NULL, NULL, 0);

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