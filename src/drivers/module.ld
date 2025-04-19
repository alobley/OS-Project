OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

/* MAKE SURE to use -fPIC when compiling! */

SECTIONS {
    . = 0;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}