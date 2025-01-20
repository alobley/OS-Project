#ifndef TEXT_H
#define TEXT_H

#include "vga.h"
#include <types.h>

void ClearTerminal();

void printk(const char *str, ...);

void WriteStr(const char* str);

void WriteStrSize(const char* str, size_t size);

void MoveCursor(uint16 x, uint16 y);

int16 GetX();

void regdump();

#endif