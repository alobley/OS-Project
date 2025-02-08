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

size_t memSize = 0;
size_t memSizeMiB = 0;

NORET void kernel_main(uint32_t magic, multiboot_info_t* mbootInfo){
    memSize = ((mbootInfo->mem_upper + mbootInfo->mem_lower) + 1024) * 1024;      // Total memory in bytes
    memSizeMiB = memSize / 1024 / 1024;

    printf("Multiboot magic: %x\n", magic);
    printf("Memory: %u MiB\n", memSizeMiB);

    InitIDT();
    InitISR();
    InitFPU();
    InitIRQ();
    InitTimer();

    MapBitmap(memSize, mbootInfo->mmap_addr, mbootInfo->mmap_length / sizeof(mmap_entry_t));

    PageKernel(memSize);

    asm volatile("mov $1, %eax");
    DO_SYSCALL

    printf("Used memory: %u KiB\n", totalPages * 4);

    for(;;);
    __builtin_unreachable();
}