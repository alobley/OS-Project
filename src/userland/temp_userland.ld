OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

/* This linker script is a temporary measure until I have proper paging */

SECTIONS
{
    . = 0x60000000;
    
    .text ALIGN(0x1000) :
    {
        *(.text)
    }

    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Read-write data */
    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(0x1000) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    
    end = .;
}