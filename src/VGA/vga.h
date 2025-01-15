#ifndef VGA_H
#define VGA_H

#include <types.h>
typedef uint8 color;

// Utilities
#include <io.h>

// VGA driver files
#include "text.h"
#include "pixel.h"

// VGA Graphics Registers
#define VGA_GRAPHICS_INDEX 0x3CE
#define VGA_GRAPHICS_DATA 0x3CF

// VGA Sequencer Registers
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5

// VGA attribute registers - for 16 color and 64 color palettes.
#define VGA_ATTR_INDEX_DATA 0x3C0   // Both index and data register (write for index, write again for data)
#define VGA_ATTR_READ 0x3C1

// VGA CRT Controller Registers
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5

// VGA color registers - for 256 color palettes. Also called digital-to-analog converter registers.
#define VGA_DAC_MASK 0x3C6
#define VGA_DAC_READ 0x3C7
#define VGA_DAC_STATE 0x3C7
#define VGA_DAC_WRITE 0x3C8
#define VGA_DAC_DATA 0x3C9


// VGA misc registers
#define VGA_MISC_OUT 0x3CC
#define VGA_ALT_MISC_OUT 0x3C2
#define VGA_FEATURE_CTRL 0x3CA
#define VGA_INPUT_STATUS_0 0x3C2
// Not needed, undefined location
//#define VGA_INPUT_STATUS_1 

#define VGA_WRITE_SEQ(index, data) \
    outb(VGA_SEQ_INDEX, index); \
    outb(VGA_SEQ_DATA, data)

#define VGA_WRITE_CRTC(index, data) \
    outb(VGA_CRTC_INDEX, index); \
    outb(VGA_CRTC_DATA, data)


// Note that the index automatically increments. This is for one color, not all of them. 
// It can be used for all of them, but efficiency is critical.
#define VGA_SET_COLOR(index, r, g, b)\
    outb(VGA_DAC_WRITE, index); \
    outb(VGA_DAC_DATA, r); \
    outb(VGA_DAC_DATA, g); \
    outb(VGA_DAC_DATA, b)

// Text mode information
#define VGA_MODE_TEXT 0x03          // 80x25x16 text mode
//#define VGA_TEXT_MODE_START 0xB8000
extern uintptr_t VGA_TEXT_MODE_START;
#define VGA_TEXT_MODE_SIZE (80 * 25)
#define VGA_TEXT_MODE_WIDTH 80
#define VGA_TEXT_MODE_HEIGHT 25
#define VGA_TEXT_MODE_END (VGA_TEXT_MODE_START + VGA_TEXT_MODE_SIZE)

#define VGA_REGION_SIZE 0x10000

// Graphics mode information
#define VGA_MODE_GRAPHICS 0x13      // 320x200x256 pixel mode
extern uintptr_t VGA_PIXEL_MODE_START;
//#define VGA_PIXEL_MODE_START 0xA0000
#define VGA_PIXEL_MODE_SIZE (320 * 200)
#define VGA_PIXEL_MODE_WIDTH 320
#define VGA_PIXEL_MODE_HEIGHT 200

#define VGA_PIXEL_MODE_END (VGA_PIXEL_MODE_START + VGA_PIXEL_MODE_SIZE)

typedef struct {
    color red;
    color green;
    color blue;
} color_rgb_t;

// Sets 8-bit rrrgggbb color to the 256-color palette.
void VGA_SetColorRGB8();

// Sets the VGA mode
void VGA_SetMode(uint8 mode);

// Sets a new color palette based on a loaded one (say from the disk)
void VGA_SetPalette(const color_rgb_t* palette);

void VGA_SaveState();

void InitVGA();

#endif