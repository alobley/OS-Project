#ifndef UTIL_H
#define UTIL_H

#include <types.h>

// Assembly
#define hlt asm("hlt")
#define cli asm volatile("cli")
#define sti asm volatile("sti")
#define MEMORY_BARRIER() asm volatile("" ::: "memory")

#define eax(value) asm("mov %0, %%eax" :: "r"(value) : "eax")
#define ebx(value) asm("mov %0, %%ebx" :: "r"(value) : "ebx")
#define get_cr0(value) asm("movl %%cr0, %0" : "=r"(value))
#define cr0(value) asm("movl %0, %%cr0" :: "r"(value))
#define cr3(value) asm("movl %0, %%cr3" :: "r"(value))

#define STOP asm("cli\nhlt")

// Attributes
#define PACKED __attribute__((packed))
#define ALIGNED(num) __attribute__((aligned(num)))
#define NORETURN __attribute__((noreturn))
#define INLINE __attribute__((always_inline)) inline

static inline void memset(void *dst, uint8 value, size_t n) {
    volatile uint8* d = (volatile uint8*) dst;

    while (n-- > 0) {
        *d++ = value;
    }
}

static inline void* memcpy(void* dest, const void* src, size_t n){
    uint8 *d = (uint8 *)dest; // Cast dest to unsigned char pointer
    const uint8 *s = (const uint8 *)src; // Cast src to unsigned char pointer

    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i]; // Copy each byte from src to dest
    }

    return dest; // Return the destination pointer
}

#define SYSCALL_INT 0x30


#endif  // UTIL_H