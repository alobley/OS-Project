#include <util.h>
#include <stdint.h>
#include <console.h>
#include <stddef.h>
#include <multiboot.h>
#include <string.h>
#include <interrupts.h>
#include <fpu.h>
#include <time.h>
#include <paging.h>
#include <alloc.h>
#include <keyboard.h>
#include <kernel.h>
#include <multitasking.h>
#include <pcspkr.h>
#include <acpi.h>
#include <users.h>
#include <vfs.h>
#include <devices.h>
#include <tty.h>
#include <ata.h>
#include <mbr.h>
#include <fat.h>

size_t memSize = 0;
size_t memSizeMiB = 0;

// Reference the built-in shell
extern int shell(void);

version_t kernelVersion = {0, 5, 0};

multiboot_info_t mbootCopy;

/* Short-Term TODO:
 * - Implement a proper command parser in KISh (done)
 * - Finish up the driver/module implementation (in progress)
 * - Implement initrd (optional)
 * - Create a driver to be loaded as a module
 * - Improve the memory manager
 * - Complete the VFS and add full disk drivers  (in progress)
 * - Implement file then program loading
 * - Implement a proper task scheduler
 * - Read up on UNIX philosophy and more closely follow it
*/


/* Driver design considerations:
 * - Each driver should be a loadable module
 * - Drivers are just regular executables. When they need to interact with their device, the kernel must context switch to them.
 * - Calling the function pointers from drivers directly is NOT RECOMMENDED because synchronization is done through the system calls (process switching to drivers messes with locks)
 * 
 * After drivers, when in userland:
 * - Create a standard system for interaction with the kernel (say a graphics library) (OpenGL? Framebuffer access?)
 * - Create a libc
 * - Create a shell
 */

 pcb_t* kernelPCB = NULL;

NORET void kernel_main(uint32_t magic, multiboot_info_t* mbootInfo){
    if(magic != MULTIBOOT2_MAGIC && magic != MULTIBOOT_MAGIC){
        printf("KERNEL PANIC: Invalid multiboot magic number: 0x%x\n", magic);
        STOP
    }
    memSize = ((mbootInfo->mem_upper + mbootInfo->mem_lower) + 1024) * 1024;      // Total memory in bytes
    memSizeMiB = memSize / 1024 / 1024;

    printf("Bootloader: %s\n", mbootInfo->boot_loader_name);
    printf("Multiboot magic: 0x%x\n", magic);
    printf("Memory: %u MiB\n", memSizeMiB);

    InitIDT();
    InitISR();
    InitFPU();
    InitIRQ();
    InitTimer();

    InitializeACPI();

    memcpy(&mbootCopy, mbootInfo, sizeof(multiboot_info_t));

    // Do some stuff for the VBE driver...

    printf("Parsing memory map...\n");
    MapBitmap(memSize, mbootInfo->mmap_addr, mbootInfo->mmap_length / sizeof(mmap_entry_t));

    printf("Paging memory...\n");
    PageKernel(memSize);

    InitializeAllocator();

    do_syscall(1, 0, 0, 0, 0, 0);

    uint32_t usedMem = totalPages * PAGE_SIZE;

    printf("Used memory: %d MiB\n", usedMem / 1024 / 1024);

    // Stress test the memory allocator
    printf("Stress testing the heap allocator...\n");
    for(int i = 1; i < 1000; i++){
        uint8_t* test = halloc(PAGE_SIZE * 6);
        if(test == NULL){
            printf("Failed to allocate memory!\n");
            STOP
        }

        memset(test, 1, PAGE_SIZE * 6);

        hfree(test);
    }

    printf("Memory stress test completed successfully!\n");

    printf("Getting system time from CMOS...\n");
    SetTime();

    // Test the timer (removed for faster debugging - the timer works)
    //printf("Testing the timer...\n");
    //sleep(1000);

    printf("Creating device registry...\n");
    if(CreateDeviceRegistry() != DRIVER_SUCCESS){
        printf("KERNEL PANIC: Failed to create device registry!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    printf("Device registry created successfully!\n");

    // Test the PC speaker (removed because it is loud af)
    //PCSP_Beep();

    // Create the kernel's PCB
    kernelPCB = CreateProcess(NULL, "syscore", VFS_ROOT, ROOT_UID, true, true, true, KERNEL, 0, NULL);
    if(kernelPCB == NULL){
        printf("KERNEL PANIC: Failed to create kernel PCB!\n");
        do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
        STOP
    }
    printf("Kernel PCB created successfully!\n");
    struct Registers* dummy = halloc(sizeof(struct Registers*));
    memset(dummy, 0, sizeof(struct Registers*));
    SwitchToSpecificProcess(kernelPCB, dummy);

    InitializeVfs(mbootInfo);

    InitializeKeyboard();
    InitializeTTY();
    InitializeAta();

    // Read from the first sector of the first disk to test the built-in PATA driver
    device_t* ataDevice = GetDeviceFromVfs("/dev/pat0");
    uint8_t* buffer = halloc(((blkdev_t*)ataDevice->deviceInfo)->sectorSize);
    if(buffer == NULL){
        printf("Failed to allocate memory for buffer!\n");
        STOP
    }
    memset(buffer, 0, 512);
    *(uint64_t*)buffer = 0;
    *(buffer + 8) = 1;
    do_syscall(SYS_DEVICE_READ, (uint32_t)ataDevice, (uint32_t)buffer, 512, 0, 0);
    int result = 0;
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != DRIVER_SUCCESS){
        printf("Failed to read from disk! Error code: %d\n", result);
        STOP
    }

    printf("Successfully read from disk!\n");

    mbr_t* mbr = (mbr_t*)buffer;
    if(IsValidMBR(mbr)){
        printf("Valid MBR found!\n");
        printf("MBR Signature: 0x%x\n", mbr->signature);
    }else{
        printf("This disk may not be MBR!\n");
    }

    hfree(buffer);

    while(ataDevice != NULL){
        result = GetPartitionsFromMBR(ataDevice);
        if(result != DRIVER_SUCCESS){
            printf("Could not detect partitions.\n");
        }else{
            printf("Partitions successfully retrieved from MBR!\n");
        }
        ataDevice = ataDevice->next;
        while(ataDevice != NULL && ataDevice->type != DEVICE_TYPE_BLOCK){
            ataDevice = ataDevice->next;
            if(ataDevice == NULL){
                break;
            }
        }
    }

    // Initialize the FAT driver
    InitializeFAT();
    // Assign the FAT driver by probing the disks
    ataDevice = GetDeviceFromVfs("/dev/pat0");
    
    // Probe the drivers to find filesystem support for the disks
    while(ataDevice != NULL){
        driver_t* fsDriver = FindDriver(ataDevice, DEVICE_TYPE_FILESYSTEM);
        // If the driver aquired the device, it is expected to have made a filesystem device
        if(fsDriver == NULL){
            printf("Driver not found for device %s\n", ataDevice->name);
        }
        ataDevice = ataDevice->next;
        while(ataDevice != NULL && ataDevice->type != DEVICE_TYPE_BLOCK){
            ataDevice = ataDevice->next;
            if(ataDevice == NULL){
                break;
            }
        }
    }

    // Load a users file and create the users...

    // Create a dummy PCB for the shell
    pcb_t* shellPCB = CreateProcess(shell, "shell", VFS_ROOT, ROOT_UID, true, false, true, NORMAL, PROCESS_DEFAULT_TIME_SLICE, kernelPCB);
    // The kernel is the steward of all processes
    kernelPCB->firstChild = shellPCB;
    SwitchToSpecificProcess(shellPCB, dummy);
    
    // Jump to the built-in debug shell
    // TODO:
    // - Load the shell from the filesystem
    // - Make the shell a userland application
    result = shellPCB->EntryPoint();

    hfree(dummy);

    DestroyProcess(shellPCB);
    SwitchToSpecificProcess(kernelPCB, dummy);

    // Schedule the first process
    Scheduler();

    for(;;){
        hlt
    }
    UNREACHABLE
}