#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <util.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Parameters are pushed in reverse order onto the stack
struct Registers {
    uint32_t __ignored, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, user_esp, ss;
};

#define PIC1 0x20
#define PIC1_OFFSET 0x20
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_OFFSET 0x28
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20
#define PIC_MODE_8086 0x01
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define TIMER_IRQ 0
#define KB_IRQ 1

int InstallIRQ(size_t i, void (*handler)(struct Registers*));
int RemoveIRQ(size_t i);
void InitIRQ();

// The maximum number of ISRs we can have (including number 0)
#define MAX_ISRS 256

// Define the IRQs reserved by Intel
typedef enum {
    DIVIDE_BY_ZERO = 0x00,
    DEBUG = 0x01,
    NON_MASKABLE_INTERRUPT = 0x02,
    BREAKPOINT = 0x03,
    OVERFLOW = 0x04,
    BOUND_RANGE_EXCEEDED = 0x05,
    INVALID_OPCODE = 0x06,
    DEVICE_NOT_AVAILABLE = 0x07,
    DOUBLE_FAULT = 0x08,
    COPROCESSOR_SEGMENT_OVERRUN = 0x09,
    INVALID_TSS = 0x0A,
    SEGMENT_NOT_PRESENT = 0x0B,
    STACK_SEGMENT_FAULT = 0x0C,
    GENERAL_PROTECTION_FAULT = 0x0D,
    PAGE_FAULT = 0x0E,
    RESERVED = 0x0F,
    X87_FPU_FLOATING_POINT_ERROR = 0x10,
    ALIGNMENT_CHECK = 0x11,
    MACHINE_CHECK = 0x12,
    SIMD_FLOATING_POINT_EXCEPTION = 0x13,
    VIRTUALIZATION_EXCEPTION = 0x14,
    CONTROL_PROTECTION_EXCEPTION = 0x15,
    RESERVED_2 = 0x16,
    RESERVED_3 = 0x17,
    RESERVED_4 = 0x18,
    RESERVED_5 = 0x19,
    RESERVED_6 = 0x1A,
    RESERVED_7 = 0x1B,
    RESERVED_8 = 0x1C,
    RESERVED_9 = 0x1D,
    RESERVED_10 = 0x1E,
    SECURITY_EXCEPTION = 0x1F
} exception_t;

// Define the OS's IRQs
typedef enum {
    KBD = 0x20,             // PS/2 Keyboard
    MOUSE = 0x21,           // PS/2 Mouse
    PIT = 0x22,             // Programmable interval timer
    ATA1 = 0x23,            // Primary ATA disk
    ATA2 = 0x24,            // Secondary ATA disk
    FLOPPY = 0x25,          // Floppy disk controller
    SPURIOUS = 0x26,        // Spurious interrupt (do not send EOI)
    SYSCALL = 0x30          // System call
} os_irq;

void InstallISR(size_t index, void (*handler)(struct Registers*));
void InitISR();

void SetIDT(uint8_t index, void (*base)(struct Registers*), uint16_t selector, uint8_t flags);

void InitIDT();

#define DO_SYSCALL asm volatile("int $0x30");

#endif