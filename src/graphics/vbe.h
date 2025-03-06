#ifndef VBE_H
#define VBE_H

#include <stdint.h>
#include <util.h>
#include <multiboot.h>
#include <devices.h>

// Protected mode interface for VBE 2.0+

typedef struct {
    char signature[4];              // "VESA"
    uint16_t version;               // VBE version
    uint16_t oemStringPtr[2];       // Pointer to OEM string
    uint8_t capabilities[4];        // Capabilities of the graphics controller
    uint16_t videoModePtr[2];       // Pointer to supported video modes
    uint16_t totalMemory;           // Number of 64KB memory blocks
    uint8_t reserved[492];          // Reserved for future use
} PACKED vbe_info_blk;

typedef struct {
    DEPRECATED uint16_t attributes;
    DEPRECATED uint8_t windowA;
    DEPRECATED uint8_t windowB;
    DEPRECATED uint16_t granularity;

    uint16_t windowSize;
    uint16_t segmentA;
    uint16_t segmentB;

    DEPRECATED uint32_t winFuncPtr;

    uint16_t pitch;
    uint16_t width;
    uint16_t height;

    UNUSED uint8_t wChar;
    UNUSED uint8_t yChar;

    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memoryModel;
    uint8_t bankSize;
    uint8_t imagePages;
    uint8_t reserved0;

    uint8_t redMask;
    uint8_t redPosition;
    uint8_t greenMask;
    uint8_t greenPosition;
    uint8_t blueMask;
    uint8_t bluePosition;
    uint8_t reservedMask;
    uint8_t reservedPosition;
    uint8_t directColorAttributes;

    uintptr_t framebuffer;
    uint32_t offScreenMemOffset;
    uint16_t offScreenMemSize;
    uint8_t reserved1[206];
} PACKED vbe_mode_info;

uint16_t GetBestMode(uint16_t width, uint16_t height, uint8_t bpp, multiboot_info_t* mbootInfo);
void SetVbeMode(uint16_t mode, framebuffer_t* fb);

#endif