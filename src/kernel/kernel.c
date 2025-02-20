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
#include <disk.h>
#include <devices.h>

size_t memSize = 0;
size_t memSizeMiB = 0;

// Reference the built-in shell
extern int shell(void);

version_t kernelVersion = {0, 2, 3};

/* Short-Term TODO:
 * - Implement a proper command parser in KISh
 * - Finish up the driver/module implementation
 * - Implement initrd
 * - Create a driver to be loaded as a module
 * - Improve the memory manager
 * - Complete the VFS and add full disk drivers
 * - Implement file then program loading
 * - Implement a proper task scheduler
 * - Read up on UNIX philosophy and more closely follow it
*/


/* Expected driver setup:
 * - VGA driver (integrated into kernel)
 * - Ramdisk driver (integrated into kernel)
 * - Keyboard Driver (loadable module in initrd)
 * - Mouse Driver (loadable module)
 * - PCI/PCIe Driver (Integrated into kernel?)
 * - ACPI Driver (Integrated into kernel)
 * - PATA Driver (loadable module in initrd)
 * - AHCI Driver (loadable module in initrd)
 * - VBE driver (loadable module)
 * - Network Driver (loadable module)
 * - Sound Driver (loadable module)
 * - USB Driver (loadable module in initrd)
 * - Filesystem Drivers (loadable modules in initrd)
 * - i915 driver (loadable module)
 * - Page kernel to the higher half of memory instead of at 2MiB
 *  
 * How drivers work
 * - Physical devices can also have virtual children (i.e. a disk device can have a child device as a filesystem driver, and each one for a partition)
 * - Monolithic design is typically easier
 * - Physical and virtual devices should be considered as entirely separate devices (i.e. sda is not the same as sda1)
 * - Some filesystem and disk drivers should be directly integrated into the kernel
 * - The kernel should be able to identify all the devices on the system
 * 
 * How I might set them up:
 * - Create a device registry
 * - Create a root bus
 * - Scan for all hardware devices
 * - Create a media descriptor for each device
 * - Load the appropriate driver for each device (if one exists)
 * - Add the device to the device registry
 * - Create virtual devices as needed, such as for filesystems. One for each partition most likely
 * - Add the virtual devices to the device registry
 * - Initialize the VFS
 * - Add each device as a symbolic file in /dev
 * 
 * Driver design considerations:
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
 * I'm kind of overwhelmed. Not sure where to start or what to do first.
*/

NORET void kernel_main(uint32_t magic, multiboot_info_t* mbootInfo){
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

    // Do some stuff for the (future) VBE driver...

    printf("Parsing memory map...\n");
    MapBitmap(memSize, mbootInfo->mmap_addr, mbootInfo->mmap_length / sizeof(mmap_entry_t));

    printf("Paging memory...\n");
    PageKernel(memSize);

    InitializeAllocator();

    do_syscall(1, 0, 0, 0);

    uint32_t usedMem = totalPages * PAGE_SIZE;

    printf("Used memory: %d MiB\n", usedMem / 1024 / 1024);

    InitializeKeyboard();

    // Stress test the memory allocator
    printf("Stress testing the heap allocator...\n");
    for(int i = 1; i <= 1000; i++){
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

    // Test the PC speaker
    PCSP_Beep();

    // Create device registry...

    // Create the root bus...

    // Create the ramfs media descriptor...

    // Load the ramfs driver...

    // Add the ramfs device to the device registry...

    // Initialize the VFS
    vfs_init(mbootInfo);

    InitializeDeviceRegistry();

    // Load modules and drivers from initrd...

    // Load a users file and create the users...

    // Other system initialization...

    // Create the kernel's PCB
    pcb_t* kernelPCB = CreateProcess(NULL, "syscore", VFS_ROOT, ROOT_UID, true, true, true, KERNEL, 0);

    // Create a dummy PCB for the shell
    pcb_t* shellPCB = CreateProcess(shell, "shell", VFS_ROOT, ROOT_UID, true, false, true, NORMAL, PROCESS_DEFAULT_TIME_SLICE);
    SwitchProcess(shellPCB);
    
    // Jump to the built-in debug shell
    // TODO:
    // - Load the shell from the filesystem
    // - Make the shell a userland application
    int result = shellPCB->EntryPoint();
    DestroyProcess(shellPCB);

    // Schedule the first process
    Scheduler();

    for(;;){
        hlt
    }
    __builtin_unreachable();
}