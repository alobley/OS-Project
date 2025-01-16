#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <types.h>
#include <util.h>

// This code was written by me, but obviously I took it straight from the real thing. Credit where it is due: https://github.com/Jolicloud/grub2/blob/master/include/multiboot2.h

#define MBOOT_MODULE_ALIGN 0x00001000
#define MBOOT_INFO_ALIGN 0x00000004
#define MBOOT_PAGE_ALIGN 0x00000001
#define MBOOT_MEMORY_INFO 0x00000020
#define MBOOT_VIDEO_MODE 0x00000080

// Multiboot flags
#define MBOOT_INFO_MEMORY 0x00000001
#define MBOOT_INFO_BOOTDEV 0x00000002
#define MBOOT_INFO_CMDLINE 0x00000004
#define MBOOT_INFO_MODS 0x00000008
#define MBOOT_INFO_AOUT_SYMS 0x00000010
#define MBOOT_INFO_ELF_SHDR 0x00000020
#define MBOOT_INFO_MEM_MAP 0x00000040
#define MBOOT_INFO_DRIVE_INFO 0x00000080
#define MBOOT_INFO_CONFIG_TABLE 0x00000100
#define MBOOT_INFO_BOOT_LOADER_NAME 0x00000200
#define MBOOT_INFO_APM_TABLE 0x00000400
#define MBOOT_INFO_VBE_INFO 0x00000800
#define MBOOT_INFO_FRAMEBUFFER_INFO 0x00001000

// Framebuffer types
#define MBOOT_FRAMEBUFFER_INDEXED 0
#define MBOOT_FRAMEBUFFER_RGB 1
#define MBOOT_FRAMEBUFFER_EGA_TEXT 2

// Memory map types
#define MBOOT_MEMORY_AVAILABLE 1
#define MBOOT_MEMORY_RESERVED 2
#define MBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MBOOT_MEMORY_NVS 4

// File written by alobley, the author of this project. This is under the MIT license.

typedef struct MemoryMapEntry{
    uint32 size;
    uint64 addr;
    uint64 length;
    uint32 type;
} PACKED mboot_mmap_entry_t;

typedef struct {
    uintptr_t modStart;     // Module starting address
    uintptr_t modEnd;       // Module ending address
    char* string;           // String describing the module
    uint32 reserved;
} PACKED module_t;

typedef struct AoutSymbolTable{
    uint32 tabSize;
    uint32 strSize;
    uint32 addr;
    uint32 reserved;
} aout_symbol_table_t;

typedef struct ElfSectionHeaderTable{
    uint32 num;
    uint32 size;
    uint32 addr;
    uint32 shndx;
} elf_section_header_t;

// The multiboot info structure passed to the kernel. Hardly used now, will be utilized more later.
typedef struct MultibootInfo {
    // System information
    uint32 flags;           // Flags for which fields are valid

    // Memory information (in kb)
    uint32 memLower;
    uint32 memUpper;

    // BIOS number of the boot device
    uint32 bootDevice;

    // Command-line string address
    char* cmdline;

    // Module info
    uint32 modsCount;       // Number of loaded modules
    module_t* modsAddr;     // Address of the first module structure

    // Symbol table and ELF section header information
    union {
        aout_symbol_table_t aoutSym;
        elf_section_header_t elfSec;
    } syms;

    // Memory map information (this is important)
    uint32 mmapLen;         // Length of the memory map
    mboot_mmap_entry_t* mmapAddr; // Address of the first entry in the memory map

    // Drive information
    uint32 drivesLen;       // Length of the drives structure
    uintptr_t drivesAddr;   // Address of the first drive structure

    // ROM configuration table
    uint32 configTable;

    // Name of the bootloader (pointer to the string)
    char* bootloaderName;

    // Advanced power management table
    uintptr_t apm_table;

    // VESA BIOS Extensions (these could be very helpful)
    uint32 vbeCtrlInfo;
    uint32 vbeModeInfo;
    uint16 vbeMode;
    uint16 vbeInterfaceSeg;
    uint16 vbeInterfaceOff;
    uint16 vbeInterfaceLen;

    // VBE framebuffer information
    uint64 framebufferAddr;
    uint32 framebufferPitch;
    uint32 framebufferWidth;
    uint32 framebufferHeight;
    uint8 framebufferBpp;
    uint8 framebufferType;
    union{
        struct {
            uint32 framebufferPaletteAddr;
            uint16 framebufferPaletteNumColors;
        };
        struct {
            uint8 framebufferRedFieldPosition;
            uint8 framebufferRedMaskSize;
            uint8 framebufferGreenFieldPosition;
            uint8 framebufferGreenMaskSize;
            uint8 framebufferBlueFieldPosition;
            uint8 framebufferBlueMaskSize;
        };
    };
} mboot_info_t;


#endif