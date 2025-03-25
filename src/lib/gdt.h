#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include <util.h>

#define SEGMENT_DESC_TYPE(x) ((x) << 0x04)                  // Descriptor type (0 = system, 1 = code/data)
#define SEGMENT_PRESENT(x) ((x) << 0x07)                    // Segment present
#define SEGMENT_SEG_AVL(x) ((x) << 0x0C)                    // Available for use by system software
#define SEGMENT_LONG_MODE(x) ((x) << 0x0D)                  // Long mode (0 = 16 or 32 bit, 1 = 64 bit)
#define SEGMENT_SIZE(x) ((x) << 0x0E)                       // Size (0 = 16 bit, 1 = 32 bit)
#define SEGMENT_GRANULARITY(x) ((x) << 0x0F)                // Granularity (0 = 1 byte, 1 = 4 KiB)
#define SEGMENT_PRIVILEGE(x) ((x) << 0x05)                  // Privilege level

#define SEGMENT_DATA_RO 0x00                                // Read-only data segment
#define SEGMENT_DATA_RO_ACCESSED 0x01                       // Read-only data segment, accessed
#define SEGMENT_DATA_RW 0x02                                // Read-write data segment,
#define SEGMENT_DATA_RW_ACCESSED 0x03                       // Read-write data segment, accessed
#define SEGMENT_DATA_RO_DOWN 0x04                           // Read-only data segment, expand-down
#define SEGMENT_DATA_RO_DOWN_ACCESSED 0x05                  // Read-only data segment, expand-down, accessed
#define SEGMENT_DATA_RW_DOWN 0x06                           // Read-write data segment, expand-down
#define SEGMENT_DATA_RW_DOWN_ACCESSED 0x07                  // Read-write data segment, expand-down, accessed
#define SEGMENT_CODE_XO 0x08                                // Execute-only code segment
#define SEGMENT_CODE_XO_ACCESSED 0x09                       // Execute-only code segment, accessed
#define SEGMENT_CODE_RX 0x0A                                // Execute-read code segment
#define SEGMENT_CODE_RX_ACCESSED 0x0B                       // Execute-read code segment, accessed
#define SEGMENT_CODE_XO_CONFORMING 0x0C                     // Execute-only code segment, conforming
#define SEGMENT_CODE_XO_CONFORMING_ACCESSED 0x0D            // Execute-only code segment, conforming, accessed
#define SEGMENT_CODE_RX_CONFORMING 0x0E                     // Execute-read code segment, conforming
#define SEGMENT_CODE_RX_CONFORMING_ACCESSED 0x0F            // Execute-read code segment, conforming, accessed

#define GDT_CODE_PL0 (SEGMENT_DESC_TYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SEG_AVL(0) | SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) | SEGMENT_PRIVILEGE(0) | SEGMENT_CODE_RX)
#define GDT_DATA_PL0 (SEGMENT_DESC_TYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SEG_AVL(0) | SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) | SEGMENT_PRIVILEGE(0) | SEGMENT_DATA_RW)

#define GDT_CODE_PL3 (SEGMENT_DESC_TYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SEG_AVL(0) | SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) | SEGMENT_PRIVILEGE(3) | SEGMENT_CODE_RX)
#define GDT_DATA_PL3 (SEGMENT_DESC_TYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SEG_AVL(0) | SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) | SEGMENT_PRIVILEGE(3) | SEGMENT_DATA_RW)

#define GDT_TSS_SEGMENT (SEGMENT_DESC_TYPE(0) | SEGMENT_PRESENT(1) | SEGMENT_SEG_AVL(0) | SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) | SEGMENT_PRIVILEGE(0) | 0x9)

#define GDT_RING0_SEGMENT_POINTER(x) (x * 8)
#define GDT_RING3_SEGMENT_POINTER(x) ((x * 8) | 3)

#define GDT_NULL_SEGMENT 0x0000000000000000ULL

enum Default_Segments {
    GDT_NULL = 0,
    GDT_KERNEL_CODE = 1,
    GDT_KERNEL_DATA = 2,
    GDT_USER_CODE = 3,
    GDT_USER_DATA = 4,
    GDT_TSS = 5
};

struct TSS_Entry {
    uint32_t prevTss;
    uint32_t esp0;
    uint32_t ss0;

    // Unused (hardware task switching is obsolete)
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomapBase;
} PACKED;

static ALIGNED(16) struct TSS_Entry tss;

struct GDT {
    uint64_t null;
    uint64_t kernelCode;
    uint64_t kernelData;
    uint64_t userCode;
    uint64_t userData;
    uint64_t tss;
} PACKED;

struct GDTPointer {
    uint16_t limit;
    uint32_t base;
} PACKED;

/// @brief Create a GDT entry based on the parameters provided
/// @param base The starting address of the segment
/// @param limit The ending address of the segment
/// @param flags Flags for the segment
/// @return a full GDT entry
static inline uint64_t CreateDescriptor(uint32_t base, uint32_t limit, uint16_t flags){
    uint64_t descriptor = 0;
    descriptor = limit & 0x000F0000;
    descriptor |= (flags << 8) & 0x00F0FF00;
    descriptor |= (base >> 16) & 0x000000FF;
    descriptor |= base & 0xFF000000;

    descriptor <<= 32;

    descriptor |= base << 16;
    descriptor |= limit & 0x0000FFFF;

    return descriptor;
}

#endif