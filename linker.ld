ENTRY(_start)

SECTIONS
{   
    . = 1M;
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