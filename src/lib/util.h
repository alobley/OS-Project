#ifndef UTIL_H
#define UTIL_H

// Parameters are pushed in reverse order onto the stack
struct Registers {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs;
    unsigned int eflags;
    unsigned int user_esp, ss;
};

#define NORET __attribute__((noreturn))                 // Function will not return
#define PACKED __attribute__((packed))                  // Packs the struct so there is no padding
#define ALIGNED(x) __attribute__((aligned(x)))          // Aligns the variable to x bytes
#define UNUSED __attribute__((unused))                  // Variable is unused
#define DEPRECATED __attribute__((deprecated))          // Function is deprecated
#define HOT __attribute__((hot))                        // Function is hot (frequently called) - tells the compiler to optimize for speed
#define COLD __attribute__((cold))                      // Function is cold (infrequently called) - tells the compiler to optimize for size
#define FORCE_INLINE __attribute__((always_inline))     // Forces the compiler to inline the function
#define NAKED __attribute__((naked))                    // Function has no prologue or epilogue (push ebp, mov ebp, esp, etc.)
#define WEAK __attribute__((weak))                      // Weak symbol - can be overridden by a stronger symbol (like function overrides in C#?)

#define MALLOC __attribute__((malloc))
#define UNREACHABLE __builtin_unreachable();

// An index of an object
typedef unsigned int index_t;

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
#define setesp(x) asm volatile("mov %0, %%esp" : : "r"(x))
#define setebp(x) asm volatile("mov %0, %%ebp" : : "r"(x))
#define getesp(x) asm volatile("mov %%esp, %0" : "=r"(x) : : "memory")
#define getebp(x) asm volatile("mov %%ebp, %0" : "=r"(x) : : "memory")

typedef union {
    struct {
        unsigned int protectionEnable : 1;                      // Enable protected mode
        unsigned int monitorCoprocessor : 1;                    // Something based on the task switching flag
        unsigned int emulation : 1;                             // Whether the CPU/software must emulate the FPU (if set, the FPU is not present)
        unsigned int taskSwitched : 1;                          // Allows the saving of the fancy SSE/MMX registers on task switch
        unsigned int extensionType : 1;                         // Will be hardcoded to 1 on supported platforms
        unsigned int numericError : 1;                          // Enables hardware detection of FPU errors
        unsigned int reserved0 : 10;
        unsigned int writeProtect : 1;                          // Prevents writes to read-only pages
        unsigned int reserved1 : 1;
        unsigned int alignmentMask : 1;                         // Automatic alignment checking
        unsigned int reserved2 : 10;
        unsigned int notWriteThrough : 1;                       // Enables write-through caching
        unsigned int cacheDisable : 1;                          // Disables the cache
        unsigned int paging : 1;                                // Enables paging
    } PACKED;
    unsigned int raw;
} CR0;

typedef union {
    struct {
        unsigned int reserved0 : 3;
        unsigned int pageWriteThrough : 1;                      // Write-through caching for pages
        unsigned int pageCacheDisable : 1;                      // Disables caching for pages
        unsigned int reserved1 : 7;
        unsigned int pageDirectoryBase : 20;                    // The base address of the page directory
    } PACKED;
    unsigned int raw;
} CR3;

typedef union {
    struct {
        unsigned int v86_extensions : 1;                        // Enable virtual 8086 mode extensions (like exceptions)
        unsigned int virtualInterrupts : 1;                     // Enable virtual interrupts
        unsigned int timeStampDisable : 1;                      // Disable the RDTSC instruction
        unsigned int debugExtensions : 1;                       // Enable debugging extensions
        unsigned int pageSizeExtensions : 1;                    // Enable 4MB pages
        unsigned int physicalAddressExtension : 1;              // Enable PAE
        unsigned int machineCheckEnable : 1;                    // Enable machine check exceptions
        unsigned int pageGlobalEnable : 1;                      // Enable global pages (may not be supported on all platforms)
        unsigned int performanceMonitor : 1;                    // Enable performance monitoring
        unsigned int osfxsr : 1;                                // Enable SSE/SSE2 instructions
        unsigned int osxmmexcpt : 1;                            // Enable SSE exceptions
        unsigned int userModeInstructionPrevention : 1;         // Prevent user mode from executing certain instructions
        unsigned int huge32BitAddressing : 1;                   // Allow for 57-bit linear addresses in IA-32e mode
        unsigned int vmxEnable : 1;                             // Enable Virtual Machine Extensions
        unsigned int smxEnable : 1;                             // Enable Safer Mode Extensions
        unsigned int FSGSBASEEnable : 1;                        // Enable the RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE instructions (whatever those do)
        unsigned int pcidEnable : 1;                            // Enable Process-Context Identifiers
        unsigned int osxsaveEnable : 1;                         // Enable XSAVE and XRSTOR instructions
        unsigned int keyLockerEnable : 1;                       // Enable the LOCK prefix
        unsigned int smepEnable : 1;                            // Enable Supervisor Mode Execution Protection (NOTE: I need to look at section 5.6)
        unsigned int smapEnable : 1;                            // Enable Supervisor Mode Access Protection (NOTE: I need to look at section 5.6)
        unsigned int pkeEnable : 1;                             // Enable Protection Key for User-mode pages
        unsigned int cetEnable : 1;                             // Enable Control-flow Enforcement Technology
        unsigned int reserved : 3;
    } PACKED;
    unsigned int raw;
} CR4;

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