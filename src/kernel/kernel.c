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

size_t memSize = 0;
size_t memSizeMiB = 0;

// Reference the built-in shell
extern int shell(void);

version_t kernelVersion = {0, 2, 0};

NORET void kernel_main(UNUSED uint32_t magic, multiboot_info_t* mbootInfo){
    memSize = ((mbootInfo->mem_upper + mbootInfo->mem_lower) + 1024) * 1024;      // Total memory in bytes
    memSizeMiB = memSize / 1024 / 1024;

    printf("Multiboot magic: 0x%x\n", magic);
    printf("Memory: %u MiB\n", memSizeMiB);

    InitializeACPI();

    InitIDT();
    InitISR();
    InitFPU();
    InitIRQ();
    InitTimer();

    printf("Parsing memory map...\n");
    MapBitmap(memSize, mbootInfo->mmap_addr, mbootInfo->mmap_length / sizeof(mmap_entry_t));

    printf("Paging memory...\n");
    PageKernel(memSize);

    InitializeAllocator();

    asm volatile("mov $1, %eax");
    DO_SYSCALL

    uint32_t usedMem = totalPages * PAGE_SIZE;

    printf("Used memory: %d MiB\n", usedMem / 1024 / 1024);

    InitializeKeyboard();

    // Stress test the memory allocator
    for(int i = 0; i < 1000; i++){
        uint8_t* test = halloc(PAGE_SIZE * 6);
        if(test == NULL){
            printf("Failed to allocate memory!\n");
            STOP
        }

        memset(test, 1, PAGE_SIZE * 6);

        hfree(test);
    }

    printf("Memory stress test completed successfully!\n");

    printf("Synchronizing system time...\n");
    SetTime();
    printf("Current time: %d:%d:%d %d/%d/%d\n", currentTime.hour, currentTime.minute, currentTime.second, currentTime.day, currentTime.month, currentTime.year);

    // Test the PC speaker
    PCSP_Beep();

    // Create a dummy PCB for the shell
    pcb_t* shellPCB = CreateProcess(shell, "shell", ROOT_UID, true, false, true);
    SwitchProcess(shellPCB);

    // Test the timer
    sleep(1000);

    int result = shell();

    DestroyProcess(shellPCB);
    if(result == 0){
        printf("Shell exited successfully! Idling...\n");
    } else {
        printf("Shell exited with error code %d!\n", result);
        STOP
    }

    for(;;){
        hlt
    }
    __builtin_unreachable();
}