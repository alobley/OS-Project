#ifndef ELF_H
#define ELF_H

// ELF files

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <util.h>

#define ELF_MAGIC 0x464C457F

enum ELF_ISAs {
    NO_ISA = 0,
    ISA_SPARC = 0x02,
    ISA_X86_32 = 0x03,
    ISA_MIPS = 0x08,
    ISA_ARM = 0x28,
    ISA_SUPERH = 0x2A,
    ISA_IA64 = 0x32,
    ISA_X86_64 = 0x3E,
    ISA_AARCH64 = 0xB7,
    ISA_RISCV = 0xF3,
};

enum ELF_ABIs {
    ABI_SYSV = 0,
    ABI_HPUX = 1,
    ABI_NETBSD = 2,
    ABI_LINUX = 3,
    ABI_SOLARIS = 6,
    ABI_AIX = 7,
    ABI_IRIX = 8,
    ABI_FREEBSD = 9,
    ABI_TRU64 = 10,
    ABI_MODESTO = 11,
    ABI_OPENBSD = 12,
};

enum ELF_FILE_TYPES {
    ELF_TYPE_NONE = 0,
    ELF_TYPE_RELOCATABLE = 1,
    ELF_TYPE_EXEC = 2,
    ELF_TYPE_SHARED = 3,
    ELF_TYPE_CORE = 4,
};

enum ELF_SEGMENT_TYPES {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    // Others...
};

typedef struct {
    uint32_t magic;
    uint8_t bits;
    uint8_t endian;
    uint8_t headerVersion;
    uint8_t osabi;
    uint8_t reserved[8];
    uint16_t type;
    uint16_t instructionSet;
    uint32_t elfVersion;
    uint32_t entry;
    uint32_t programHeaderOffset;
    uint32_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionHeaderStringIndex;
} PACKED Elf32_Ehdr;

typedef struct {
    uint32_t type;                      // Type of segment
    uint32_t offset;                    // Offset in the file
    uint32_t virtualAddress;            // Virtual address
    uint32_t physicalAddress;           // Physical address (not used in most cases)
    uint32_t fileSize;                  // Size of the segment in the file
    uint32_t memorySize;                // Size of the segment in memory
    uint32_t flags;                     // Flags (read, write, execute)
    uint32_t align;                     // Alignment
} PACKED Elf32_Phdr;

typedef struct {
    uint32_t magic;
    uint8_t bits;
    uint8_t endian;
    uint8_t headerVersion;
    uint8_t osabi;
    uint8_t _reserved[8];
    uint16_t type;
    uint16_t instructionSet;
    uint32_t elfVersion;
    uint64_t entry;
    uint64_t programHeaderOffset;
    uint64_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionHeaderStringIndex;
} PACKED Elf64_Ehdr;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtualAddress;
    uint64_t physicalAddress;
    uint64_t fileSize;
    uint64_t memorySize;
    uint64_t align;
} PACKED Elf64_Phdr;

#endif
