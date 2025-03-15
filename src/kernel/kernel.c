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

size_t memSize = 0;
size_t memSizeMiB = 0;

// Reference the built-in shell
extern int shell(void);

version_t kernelVersion = {0, 3, 0};

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
 * - Should the drivers run in userland as a microkernel environment?
 * - If the drivers don't run in kernelspace, does that require them to be "servers"? What about the single CPU this OS is designed for?
 * - Should the drivers be able to communicate with each other? (probably)
 * - Will any drivers need code that constantly runs and therefore should be scheduled? That should be avoided.
 * - What is the most efficient way for communication? Should a device type only have the one command function pointer?
 * - How should userland functions request access to a device?
 * - Should the kernel just have a wholly standardized interface, or should access to devices be done through their symbolic existence in the VFS, like UNIX?
 * - How do I abstract specifics like that? Should the OS have a GOP for graphics, for example? Should I write drivers with OpenGL support?
 * 
 * After drivers, when in userland:
 * - Create a standard system for interaction with the kernel (say a graphics library) (OpenGL? Framebuffer access?)
 * - Create a libc
 * - Create a shell
 */

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

    // Test the timer
    printf("Testing the timer...\n");
    sleep(1000);    

    printf("Creating device registry...\n");
    if(CreateDeviceRegistry() != DRIVER_SUCCESS){
        printf("Failed to create device registry!\n");
        STOP
    }
    printf("Device registry created successfully!\n");

    // Test the PC speaker
    //PCSP_Beep();

    // Create the kernel's PCB
    pcb_t* kernelPCB = CreateProcess(NULL, "syscore", VFS_ROOT, ROOT_UID, true, true, true, KERNEL, 0);
    SwitchProcess(kernelPCB);

    InitializeVfs(mbootInfo);

    InitializeKeyboard();
    InitializeTTY();
    InitializeAta();

    // Read from the first sector of the disk to see what happens
    uint8_t* buffer = halloc(512);
    if(buffer == NULL){
        printf("Failed to allocate memory for buffer!\n");
        STOP
    }
    memset(buffer, 0, 512);
    *(uint64_t*)buffer = 0;
    *(buffer + 8) = 1;
    device_t* ataDevice = GetDeviceFromVfs("/dev/pat0");
    if(ataDevice->read(ataDevice, buffer, 512) != DRIVER_SUCCESS){
        printf("Failed to read from disk!\n");
        STOP
    }

    printf("Successfully read from disk!\n");

    if(*(buffer + 511) == 0xAA && *(buffer + 510) == 0x55){
        printf("Boot Signature: 0x%x%x\n", *(buffer + 511), *(buffer + 510));
    }

    hfree(buffer);

    // Load a users file and create the users...

    // Other system initialization...

    // Setup the drivers for the TTYs and keyboard...

    // Create a dummy PCB for the shell
    pcb_t* shellPCB = CreateProcess(shell, "shell", VFS_ROOT, ROOT_UID, true, false, true, NORMAL, PROCESS_DEFAULT_TIME_SLICE);
    // The kernel is the steward of all processes
    kernelPCB->firstChild = shellPCB;
    SwitchProcess(shellPCB);
    
    // Jump to the built-in debug shell
    // TODO:
    // - Load the shell from the filesystem
    // - Make the shell a userland application
    int result = shellPCB->EntryPoint();

    DestroyProcess(shellPCB);
    SwitchProcess(kernelPCB);

    // Schedule the first process
    Scheduler();

    for(;;){
        hlt
    }
    UNREACHABLE
}