#ifndef BOOTUTIL_H
#define BOOTUTIL_H

#define NULL ((void *)0)

#define asm __asm__
#define sti asm("sti");
#define cli asm("cli");
#define hlt asm("hlt");
#define cld asm("cld");
#define std asm("std");
#define nop asm("nop");
#define clc asm("clc");
#define stc asm("stc");

#define STOP cli hlt

#define pushes asm("push %es");
#define pushfs asm("push %fs");
#define pushgs asm("push %gs");
#define pushds asm("push %ds");
#define pushss asm("push %ss");

#define popes asm("pop %es");
#define popfs asm("pop %fs");
#define popgs asm("pop %gs");
#define popds asm("pop %ds");
#define popss asm("pop %ss");

#define setes(x) asm("movw %0, %%ax" ::"g"(x)); asm("movw %ax, %es");
#define setfs(x) asm("movw %0, %%ax" ::"g"(x)); asm("movw %ax, %fs");
#define setgs(x) asm("movw %0, %%ax" ::"g"(x)); asm("movw %ax, %gs");
#define setds(x) asm("movw %0, %%ax" ::"g"(x)); asm("movw %ax, %ds");
#define setss(x) asm("movw %0, %%ax" ::"g"(x)); asm("movw %ax, %ss");

#define setax(x) asm("movw %0, %%ax" ::"g"(x));
#define setbx(x) asm("movw %0, %%bx" ::"g"(x));
#define setcx(x) asm("movw %0, %%cx" ::"g"(x));
#define setdx(x) asm("movw %0, %%dx" ::"g"(x));
#define setsi(x) asm("movw %0, %%si" ::"g"(x));
#define setdi(x) asm("movw %0, %%di" ::"g"(x));
#define setsp(x) asm("movw %0, %%sp" ::"g"(x));
#define setbp(x) asm("movw %0, %%bp" ::"g"(x));

#define getax(x) asm("movw %%ax, %0" :"=g"(x));
#define getbx(x) asm("movw %%bx, %0" :"=g"(x));
#define getcx(x) asm("movw %%cx, %0" :"=g"(x));
#define getdx(x) asm("movw %%dx, %0" :"=g"(x));
#define getsi(x) asm("movw %%si, %0" :"=g"(x));
#define getdi(x) asm("movw %%di, %0" :"=g"(x));
#define getsp(x) asm("movw %%sp, %0" :"=g"(x));
#define getbp(x) asm("movw %%bp, %0" :"=g"(x));

#define pusha asm("pusha");
#define popa asm("popa");

#define pushf asm("pushf");
#define popf asm("popf");

#define pushax asm("push %ax");
#define pushbx asm("push %bx");
#define pushcx asm("push %cx");
#define pushdx asm("push %dx");
#define pushsi asm("push %si");
#define pushdi asm("push %di");
#define pushsp asm("push %sp");
#define pushbp asm("push %bp");

#define popax asm("pop %ax");
#define popbx asm("pop %bx");
#define popcx asm("pop %cx");
#define popdx asm("pop %dx");
#define popsi asm("pop %si");
#define popdi asm("pop %di");
#define popsp asm("pop %sp");
#define popbp asm("pop %bp");

#define setal(x) asm("movb %0, %%al" ::"g"(x));
#define setah(x) asm("movb %0, %%ah" ::"g"(x));
#define setbl(x) asm("movb %0, %%bl" ::"g"(x));
#define setbh(x) asm("movb %0, %%bh" ::"g"(x));
#define setcl(x) asm("movb %0, %%cl" ::"g"(x));
#define setch(x) asm("movb %0, %%ch" ::"g"(x));
#define setdl(x) asm("movb %0, %%dl" ::"g"(x));
#define setdh(x) asm("movb %0, %%dh" ::"g"(x));

#define setcr0(x) asm("movl %0, %%cr0" ::"a"(x));
#define getcr0(x) asm("movl %%cr0, %0" :"=a"(x));

#define doint(x) asm("int %0" ::"N"(x));

#define stosw asm("stosw");
#define stosb asm("stosb");

#define lgdt(x) asm("lgdt %0" ::"m"(x));

#define NORET __attribute__((noreturn))                 // Function will not return
#define PACKED __attribute__((packed))                  // Packs the struct so there is no padding
#define ALIGNED(x) __attribute__((aligned(x)))          // Aligns the variable to x bytes
#define UNUSED __attribute__((unused))                  // Variable is unused
#define DEPRECATED __attribute__((deprecated))          // Function is deprecated
#define HOT __attribute__((hot))                        // Function is hot (frequently called) - tells the compiler to optimize for speed
#define COLD __attribute__((cold))                      // Function is cold (infrequently called) - tells the compiler to optimize for size
#define FORCE_INLINE __attribute__((always_inline))     // Forces the compiler to inline the function
#define MALLOC __attribute__((malloc))                  // Function returns a pointer to allocated memory
#define UNREACHABLE __builtin_unreachable();            // Tells the compiler that the code path is unreachable
#define SECTION(x) __attribute__((section(x)))          // Places the function or variable in the specified section

#define FASTCALL __attribute__((fastcall))              // Function uses fastcall calling convention

typedef unsigned int uintptr_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;

typedef unsigned char bool;
#define true 1
#define false 0

typedef struct {
    uint16_t segment;
    uint16_t offset;
} farptr_t;

static inline unsigned char inb(unsigned short port){
    unsigned char value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(unsigned short port, unsigned char value){
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned short inw(unsigned short port){
    unsigned short value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(unsigned short port, unsigned short value){
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned long inl(unsigned short port){
    unsigned long value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(unsigned short port, unsigned long value){
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

#define MMAP_TYPE_NORMAL 1
#define MMAP_TYPE_RESERVED 2
#define MMAP_TYPE_RECLAIMABLE 3
#define MMAP_TYPE_NVS 4
#define MMAP_TYPE_BAD 5

typedef struct PACKED {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;          // Reserved if ACPI version is less than 3.0
} PACKED memory_map_t;

typedef struct {
    uint32_t memmap_address;
    uint32_t memmap_length;

    uint8_t bootDev;

    const char* bootloaderName;
    const char* bootloaderVersion;

    uint32_t acpiRsdp;
    uint32_t acpiRsdt;

    uint32_t apm_table;         // If applicable
    
    
    // VESA info?
} boot_info_t;

typedef struct PACKED gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} PACKED gdt_entry_t;

typedef struct PACKED gdt {
    gdt_entry_t null;
    gdt_entry_t code;
    gdt_entry_t data;
    gdt_entry_t faraway;
} PACKED gdt_t;

typedef struct PACKED gdtptr {
    uint16_t limit;
    uint32_t base;
} PACKED gdtptr_t;


#endif