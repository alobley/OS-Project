#ifndef BOOTUTIL_H
#define BOOTUTIL_H 1

#include "efi.h"
#include <stdarg.h>

#define PACKED __attribute__((packed))

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;
typedef uint64 qword;

#define NULL ((void*)0)

#define asm __asm__

typedef struct {
    UINTN rows;
    UINTN cols;
} timer_context_t;

/* For reference in the kernel (best not to use this in the bootloader)
typedef enum {
    EfiReservedMemoryType      = 0,  // Reserved memory (e.g., firmware, I/O)
    EfiLoaderCode              = 1,  // Code used by the bootloader
    EfiLoaderData              = 2,  // Data used by the bootloader
    EfiBootServicesCode        = 3,  // Code used by UEFI boot services
    EfiBootServicesData        = 4,  // Data used by UEFI boot services
    EfiRuntimeServicesCode     = 5,  // Code used by UEFI runtime services
    EfiRuntimeServicesData     = 6,  // Data used by UEFI runtime services
    EfiConventionalMemory      = 7,  // Usable memory (can be used by OS)
    EfiUnusableMemory          = 8,  // Memory that is unusable (e.g., hardware failure)
    EfiACPIReclaimMemory       = 9,  // Memory used by ACPI tables and reclaimable by OS
    EfiACPINVSMemory           = 10, // Memory for non-volatile storage used by ACPI
    EfiMemoryMappedIO          = 11, // Memory mapped I/O
    EfiMemoryMappedIOPortSpace = 12, // I/O Port Space
    EfiPalCode                 = 13, // Processor-specific code (used by PAL)
    EfiPersistentMemory        = 14  // Memory intended to be persistent across boots (e.g., NVRAM)
    EfiUnacceptedMemory        = 15  // Memory that is unaccepted by the OS (if applicable)
} EFI_MEMORY_TYPE;

typedef struct {
    uint32                          Type;           // Field size is 32 bits followed by 32 bit pad
    uint32                          Pad;
    void*                           PhysicalStart;  // Field size is 64 bits
    void*                           VirtualStart;   // Field size is 64 bits
    uint64                          NumberOfPages;  // Field size is 64 bits
    uint64                          Attribute;      // Field size is 64 bits
} EFI_MEMORY_DESCRIPTOR;

typedef struct memory_map {
    EFI_MEMORY_DESCRIPTOR* memoryMap;       // Pointer to the memory map array
    size_t memoryMapSize;                   // Number of entries in the memory map
    uint32 memoryMapDescriptorSize;         // Size of a memory descriptor
} memory_map_t;

// The struct passed to the kernel
typedef struct Boot_Utilities {
    // Screen output
    void* framebuffer;                      // UEFI framebuffer
    void* backBuffer;                       // Back buffer allocated by the bootloader for double-buffering
    uint32 screenWidth;                     // Screen width in pixels
    uint32 screenHeight;                    // Screen height in pixels
    uint32 screenBpp;                       // Screen bytes per pixel

    // Memory
    void* pageTables;                       // Pointer to an aligned region of memory for the page tables (nothing is there though)
    memory_map_t memmap;                    // Memory map
    size_t totalmem;                        // Total memory in bytes the system has
    uintptr_t kernelBase;                   // Starting address of the kernel

    // ACPI tables
    uint8 acpiRevision;                     // ACPI version
    void* rsdp;
    void* xsdt;
    void* madt;
    void* fadt;
    void* dsdt;
    void* ssdt;
    void* hpet;
    void* mcfg;

    // System configuration
    uint32 cpuCount;                        // Number of CPU cores
    uint32 cpuSpeed;                        // CPU speed in MHz

    // Misc UEFI
    EFI_SYSTEM_TABLE* st;                   // System table
} bootutil_t;
*/

#define BGR(b, g, r) (r | (g << 8) | b << 16)

#endif // BOOTUTIL_H
