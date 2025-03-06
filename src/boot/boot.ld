OUTPUT_FORMAT("binary")
ENTRY(bootmain)

SECTIONS
{
    . = 0x8000;
    .text :
    {
        *(.text.start)
        *(.text)
        *(.rodata)
        *(.data)
        *(.bss)
    }
}