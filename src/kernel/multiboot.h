#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>
#include <util.h>

#define MULTIBOOT_MAGIC 0x1BADB002           // The magic number passed to the kernel by GRUB with multiboot 1 (deprecated)
#define MULTIBOOT2_MAGIC 0x2BADB002          // The magic number passed to the kernel by GRUB with multiboot 2

// These entries will be an array of mmap_entry_t structures in contiguous memory (allegedly)
typedef struct MemoryMap {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} PACKED mmap_entry_t;

typedef struct Module {
    uintptr_t start;
    uintptr_t end;
    const char* desc;
    uint32_t __reserved;
} PACKED multiboot_module_t;

typedef struct MultibootInfo {
    // System information flags
    uint32_t flags;

    // Memory information (in kilobytes)
    uint32_t mem_lower;
    uint32_t mem_upper;

    // Boot device information
    uint32_t boot_device;           // BIOS device number

    // Command line string address
    char* cmdline;

    // Module information
    uint32_t mods_count;            // Number of (loaded) modules
    multiboot_module_t* mods_addr;  // Address of the first module structure

    // Symbol table and ELF section header information
    union {
        struct {
            uint32_t tabSize;
            uint32_t strSize;
            uintptr_t addr;
            uint32_t __reserved;
        } PACKED aout_sym;
        struct {
            uint32_t num;
            uint32_t size;
            uint32_t addr;
            uint32_t shndx;
        } PACKED elf_sec;
    } PACKED syms;

    uint32_t mmap_length;           // Length of the memory map (in bytes)
    mmap_entry_t* mmap_addr;        // Address of the first memory map entry

    // Drive information
    uint32_t drives_length;         // Length of the drive information
    uintptr_t drives_addr;          // Address of the drive information

    // ROM configuration table
    uintptr_t config_table;

    // Boot loader name
    const char* boot_loader_name;

    // APM table
    uintptr_t apm_table;

    // VBE control information
    uintptr_t vbe_control_info;
    uintptr_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    // Framebuffer information
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} PACKED multiboot_info_t;

#endif