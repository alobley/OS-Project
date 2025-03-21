#ifndef ELF_H
#define ELF_H

// ELF files

#include <stdint.h>
#include <stddef.h>
#include <util.h>
#include <stdbool.h>

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

// Personal probing showed the value to be this
#define ELF_MAGIC 0x464C457F

#define ELF_FLAG_EXEC 1
#define ELF_FLAG_WRITE 2
#define ELF_FLAG_READ 4

typedef struct PACKED ELF_Header {
    uint32_t magic;                 // 0x7F 'E' 'L' 'F'
    uint8_t bits;                   // 1 = 32-bit, 2 = 64-bit
    uint8_t endianness;             // 1 = little, 2 = big
    uint8_t headerVersion;          // 1 = original
    uint8_t osABI;                  // 0 = System V
    uint8_t padding[8];
    uint16_t type;                  // 1 = relocatable, 2 = executable, 3 = shared, 4 = core
    uint16_t instructionSet;        // The supported architecture
    uint32_t version;               // 1 = original
    uintptr_t entryOffset;          // The entry point of the program(?)
    uintptr_t programHeaderOffset;  // The offset of the program header
    uintptr_t sectionHeaderOffset;  // The offset of the section header
    uint32_t flags;                 // Architecture-specific flags
    uint16_t headerSize;            // Size of this header
    uint16_t programHeaderSize;     // Size of the program header
    uint16_t numProgramHeaders;     // Number of entries in the program header
    uint16_t stringTableIndex;      // The index of the string table section
} PACKED elf_header_t;


#define ELF_HEADER_TYPE_NULL 0
#define ELF_HEADER_TYPE_PROGBITS 1
#define ELF_HEADER_TYPE_DYNAMIC 2
#define ELF_HEADER_TYPE_INTERPRETER 3
#define ELF_HEADER_TYPE_NOTE 4
#define ELF_HEADER_TYPE_SHARED 5
#define ELF_HEADER_TYPE_PROGHEADER 6
#define ELF_HEADER_TYPE_TLS 7
// Defines segments in the ELF file (is abn array of size numEntries)
typedef struct PACKED ELF_Program_Header {
    uint32_t type;                  // 0 = NULL, 1 = load, 2 = dynamic, 3 = interpreter, 4 = note, 5 = shared, 6 = program header, 7 = TLS
    uint32_t offset;                // Offset in the file where this segment can be found
    uintptr_t virtualAddress;       // Virtual address in memory where this segment should be loaded
    uintptr_t physicalAddress;      // Physical address (ignored on x86)
    uint32_t fileSize;              // Size of this segment in the file
    uint32_t flags;                 // Flags (read, write, execute)
    uint32_t alignment;             // Alignment of this segment in memory
} PACKED elf_program_header_t;

// Check to see if the executable is a valid 32-bit x86 ELF file
bool IsValidELF(elf_header_t* header);

#endif