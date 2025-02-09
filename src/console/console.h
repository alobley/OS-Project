#ifndef CONSOLE_H
#define CONSOLE_H

#include <util.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VGA_BYTES (VGA_SIZE * 2)

#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5

#define VGA_WHITE_ON_BLACK 0x0F                 // White text on a black background
#define VGA_BLACK_ON_WHITE 0xF0                 // Black text on a white background

// Clear the screen
HOT void ClearScreen(void);

// Write a single character to the screen without using printf
HOT void WriteChar(char c);

// Write a non-formatted string to the screen
HOT void WriteString(const char* str);

// Write a non-formatted string of a specific size to the screen
HOT void WriteStringSize(const char* str, size_t size);

// Print a formatted string to the screen
HOT void printf(const char* fmt, ...);

// Scroll the screen up one line
HOT void Scroll(void);

void RemapVGA(uintptr_t addr);

uintptr_t GetVGAAddress(void);

#endif