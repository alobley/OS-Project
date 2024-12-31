#ifndef UTIL_H
#define UTIL_H

#include <types.h>

// Assembly
#define hlt asm("hlt")
#define cli asm volatile("cli")
#define sti asm volatile("sti")
#define MEMORY_BARRIER() asm volatile("" ::: "memory")

// Attributes
#define PACKED __attribute__((packed))
#define ALIGNED(num) __attribute__((aligned(num)))
#define NORETURN __attribute__((noreturn))
#define INLINE __attribute__((always_inline)) inline

static inline void memset(void *dst, uint8 value, size_t n) {
    uint8 *d = dst;

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