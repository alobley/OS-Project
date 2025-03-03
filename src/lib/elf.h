#ifndef ELF_H
#define ELF_H

// ELF files

#include <stdint.h>
#include <stddef.h>
#include <util.h>

// These are the only architectures I need to worry about for now
#define ELF_ARCH_X86 0x03
#define ELF_ARCH_X86_64 0x3E

#define ELF_PT_NULL 0
#define ELF_PT_LOAD 1
#define ELF_PT_DYNAMIC 2
#define ELF_PT_INTERP 3
#define ELF_PT_NOTE 4
#define ELF_PT_SHARED 5
#define ELF_PT_PHDR 6
#define ELF_PT_TLS 7

#define ELF_MAGIC (((0x7F << 8 | 'E') << 8 | 'L') << 8 | 'F')

#define ELF_FLAG_EXEC 1
#define ELF_FLAG_WRITE 2
#define ELF_FLAG_READ 4

struct PACKED ELF_Header {
    uint32_t magic;                 // 0x7F, 'E', 'L', 'F'
    uint8_t bits;                   // 1 = 32-bit, 2 = 64-bit
    uint8_t endianness;             // 1 = little, 2 = big
    uint8_t headerVersion;          // 1 = original
    uint8_t osABI;                  // 0 = System V
    uint8_t[8] padding;
    uint16_t type;                  // 1 = relocatable, 2 = executable, 3 = shared, 4 = core
    uint16_t instructionSet;        // The supported architecture
    uint32_t version;               // 1 = original
    uintptr_t entryPoint;           // Entry point
    uintptr_t programHeaderOffset;  // Offset to the program header table
    uintptr_t sectionHeaderOffset;  // Offset to the section header table
    uint32_t flags;                 // Flags (ignored on x86)
    uint16_t headerSize;            // Size of this header
    uint16_t entrySize;             // Size of a program header entry
    uint16_t numEntries;            // Number of program header entries
    uint16_t sectionSize;           // Size of a section header entry
    uint16_t numSections;           // Number of section header entries
    uint16_t sectionNameIndex;      // Index of the section name string table
};

// Defines segments in the ELF file (is abn array of size numEntries)
struct PACKED ELF_Program_Header {
    uint32_t type;                  // 0 = NULL, 1 = load, 2 = dynamic, 3 = interpreter, 4 = note, 5 = shared, 6 = program header, 7 = TLS
    uint32_t flags;                 // Flags
    uintptr_t offset;               // Offset in the file where this segment can be found
    uintptr_t virtualAddress;       // Virtual address in memory
    uintptr_t physicalAddress;      // Physical address in memory
    size_t fileSize;                // Size of the file image
    uint32_t memorySize;            // Size of the memory image
    uint32_t alignment;             // Alignment
};

// Check to see if the executable is a valid 32-bit x86 ELF file
static inline bool IsValidELF(struct ELF_Header* header){
    return header->magic == ELF_MAGIC && header->bits == 1 && header->endianness == 1 && header->instructionSet == ELF_ARCH_X86;
}

#endif