OUTPUT_FORMAT("pei-x86-64")
ENTRY(efi_main)

SECTIONS {
    .text : {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(COMMON)
        *(.bss)
    }
    .reloc : {
        *(.reloc)
    }
}
