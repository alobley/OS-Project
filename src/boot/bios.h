#ifndef BIOS_H
#define BIOS_H

#include "bootutil.h"

typedef struct mmap_entry {
    uint32 baseLow;
    uint32 baseHigh;
    uint32 lengthLow;
    uint32 lengthHigh;
    uint32 type;
    uint32 acpi;
} __attribute__((packed)) mmap_entry_t;

#define MEMTYPE_AVAILABLE 1
#define MEMTYPE_RESERVED 2
#define MEMTYPE_ACPI_RECLAIMABLE 3
#define MEMTYPE_ACPI_NVS 4
#define MEMTYPE_BAD 5

#define BIOS_INTERRUPT(x) __asm__("int %0" : : "i"(x))

// Disk services
#define BIOS_DISK_SERVICE 0x13
#define BIOS_READ_SECTORS 0x02
#define BIOS_WRITE_SECTORS 0x03             // Not used in this project

// Video services
#define BIOS_VIDEO_SERVICE 0x10
#define BIOS_SET_VIDEO_MODE 0x00
#define BIOS_VIDEO_SET_CURSOR 0x02
#define BIOS_VIDEO_GET_CURSOR 0x03
#define BIOS_VIDEO_WRITE_CHAR 0x09
#define BIOS_VIDEO_WRITE_STRING 0x13

// Memory services
#define BIOS_GET_LOWMEM 0x12                // Get low memory size in KB. Result in AX
#define BIOS_HARDWARE_INTERRUPT 0x15        // Hardware interrupt


#endif