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
#define UNREACHABLE __builtin_unreachable();

#define asm __asm__

#define cli asm("cli");
#define sti asm("sti");

#define hlt asm("hlt");

#define nop asm("nop");

#define cpuid(eax, ebx, edx, ecx) asm("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));

#define cpulock asm("lock");

#define cld asm("cld");
#define std asm("std");

#define STOP cli hlt

#define lidt(x) asm("lidt %0" : : "m"(x));

#define setes(x) asm("movl %0, %%eax" ::"g"(x)); asm("movl %eax, %es")
#define setfs(x) asm("movl %0, %%eax" ::"g"(x)); asm("movl %eax, %fs")
#define setgs(x) asm("movl %0, %%eax" ::"g"(x)); asm("movl %eax, %gs")
#define setds(x) asm("movl %0, %%eax" ::"g"(x)); asm("movl %eax, %ds")
#define setss(x) asm("movl %0, %%eax" ::"g"(x)); asm("movl %eax, %ss")

#define seteax(x) asm volatile("mov %0, %%eax" ::"r"(x))
#define setebx(x) asm volatile("mov %0, %%ebx" ::"r"(x))
#define setecx(x) asm volatile("mov %0, %%ecx" ::"r"(x))
#define setedx(x) asm volatile("mov %0, %%edx" ::"r"(x))
#define setedi(x) asm volatile("mov %0, %%edi" ::"r"(x))
#define setesi(x) asm volatile("mov %0, %%esi" ::"r"(x))
#define setesp(x) asm volatile("mov %0, %%ebp" : : "r"(x) : "memory")
#define setebp(x) asm volatile("mov %0, %%ebp" : : "r"(x) : "memory")
#define getesp(x) asm volatile("mov %%esp, %0" : "=r"(x) : : "memory")
#define getebp(x) asm volatile("mov %%ebp, %0" : "=r"(x) : : "memory")

#define wrmsr asm volatile("wrmsr");
#define sysexit asm volatile("sysexit");

#define IA32_SYSENTER_CS 0x174

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