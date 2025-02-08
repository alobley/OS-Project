<<<<<<< HEAD
OUTPUT_FORMAT("elf64-x86-64")
=======
>>>>>>> a9a2e67 (Rewrote again, back at 32-bit and implemented a fully functional paging setup that is totally flawless and has no flaws at all)
ENTRY(_start)

SECTIONS
{   
    . = 2M;
    __kernel_start = .;

    .text BLOCK(4k) : ALIGN(4K)
    {
        *(.multiboot)

        *(.text)

        *(.text.interrupts)
    }

    .rodata BLOCK(4k) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4k) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4k) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    __kernel_end = .;
}