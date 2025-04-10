OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    /* Define the starting address where the program will be loaded */
    . = 0x1000;
    
    /* Read-only sections first */
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