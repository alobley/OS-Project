#ifndef UTIL_H
#define UTIL_H

#define NORET __attribute__((noreturn))                 // Function will not return
#define PACKED __attribute__((packed))                  // Packs the struct so there is no padding
#define ALIGNED(x) __attribute__((aligned(x)))          // Aligns the variable to x bytes
#define UNUSED __attribute__((unused))                  // Variable is unused
#define DEPRECATED __attribute__((deprecated))          // Function is deprecated
#define HOT __attribute__((hot))                        // Function is hot (frequently called) - tells the compiler to optimize for speed
#define COLD __attribute__((cold))                      // Function is cold (infrequently called) - tells the compiler to optimize for size
#define FORCE_INLINE __attribute__((always_inline))     // Forces the compiler to inline the function

#define MALLOC __attribute__((malloc))

#define asm __asm__

#define cli asm("cli");
#define sti asm("sti");

#define hlt asm("hlt");

#define STOP cli hlt

FORCE_INLINE static inline unsigned char inb(unsigned short port){
    unsigned char value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

FORCE_INLINE static inline void outb(unsigned short port, unsigned char value){
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

FORCE_INLINE static inline unsigned short inw(unsigned short port){
    unsigned short value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

FORCE_INLINE static inline void outw(unsigned short port, unsigned short value){
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

FORCE_INLINE static inline unsigned int inl(unsigned short port){
    unsigned int value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

FORCE_INLINE static inline void outl(unsigned short port, unsigned int value){
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

#endif