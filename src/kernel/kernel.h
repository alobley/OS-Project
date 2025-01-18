#ifndef KERNEL_H
#define KERNEL_H

#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <vga.h>
#include <keyboard.h>
#include <time.h>
#include <fpu.h>
#include <pcspkr.h>
#include <string.h>
#include <ata.h>
#include <multiboot.h>
#include <fat.h>
#include <vfs.h>
#include <acpi.h>
#include <memmanage.h>
#include "multitasking.h"

#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct version {
    uint8 major;
    uint8 minor;
    uint8 patch;
} version_t;

extern uint32 __kernel_end;
extern uint32 __kernel_start;

void reboot();
int32 ExecuteProgram(file_t* program);

#endif