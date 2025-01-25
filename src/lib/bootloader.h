#ifndef BOOTLOADER_H
#define BOOTLOADER_H 1

#include "stdint.h"
#include "stddef.h"

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
    EfiPersistentMemory        = 14, // Memory intended to be persistent across boots (e.g., NVRAM)
    EfiUnacceptedMemory        = 15  // Memory that is unaccepted by the OS (if applicable)
} EFI_MEMORY_TYPE;

typedef struct {
    uint32_t                          Type;           // Field size is 32 bits followed by 32 bit pad
    uint32_t                          Pad;
    void*                           PhysicalStart;  // Field size is 64 bits
    void*                           VirtualStart;   // Field size is 64 bits
    uint64_t                          NumberOfPages;  // Field size is 64 bits
    uint64_t                          Attribute;      // Field size is 64 bits
} EFI_MEMORY_DESCRIPTOR;

typedef struct memory_map {
    EFI_MEMORY_DESCRIPTOR* memoryMap;       // Pointer to the memory map array
    size_t memoryMapSize;                   // Number of entries in the memory map
    uint32_t memoryMapDescriptorSize;         // Size of a memory descriptor
} memory_map_t;

// The struct passed to the kernel
typedef struct Boot_Utilities {
    // Screen output
    void* framebuffer;                      // UEFI framebuffer
    void* backBuffer;                       // Back buffer allocated by the bootloader for double-buffering
    uint32_t screenWidth;                     // Screen width in pixels
    uint32_t screenHeight;                    // Screen height in pixels
    uint32_t screenBpp;                       // Screen bytes per pixel

    // Memory
    void* pageTables;                       // Pointer to an aligned region of memory for the page tables (nothing is there though)
    memory_map_t memmap;                    // Memory map
    size_t totalmem;                        // Total memory in bytes the system has
    uintptr_t kernelBase;                   // Starting address of the kernel

    // ACPI tables
    uint8_t acpiRevision;                     // ACPI version
    void* rsdp;
    void* xsdt;
    void* madt;
    void* fadt;
    void* dsdt;
    void* ssdt;
    void* hpet;
    void* mcfg;

    // System configuration
    uint32_t cpuCount;                        // Number of CPU cores
    uint32_t cpuSpeed;                        // CPU speed in MHz

    // Misc UEFI
    void* st;                   // System table
} bootutil_t;

#endif