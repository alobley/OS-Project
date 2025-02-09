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

size_t memSize = 0;
size_t memSizeMiB = 0;

NORET void kernel_main(UNUSED uint32_t magic, multiboot_info_t* mbootInfo){
    memSize = ((mbootInfo->mem_upper + mbootInfo->mem_lower) + 1024) * 1024;      // Total memory in bytes
    memSizeMiB = memSize / 1024 / 1024;

    printf("Multiboot magic: 0x%x\n", magic);
    printf("Memory: %u MiB\n", memSizeMiB);

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

    printf("Used memory: %f MiB\n", (((double)usedMem) / 1024) / 1024);

    // Test the memory allocation mechanism
    uint8_t* test = halloc(PAGE_SIZE * 6);
    if(test == NULL){
        printf("Failed to allocate memory!\n");
        STOP
    }

    printf("Allocated memory at 0x%x\n", test);

    memset(test, 1, PAGE_SIZE * 6);

    printf("Completed successfully without a fault!\n");

    for(;;);
    __builtin_unreachable();
}