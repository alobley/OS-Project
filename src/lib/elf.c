#include <elf.h>

bool IsValidELF(elf_header_t* header){
    return header->magic == ELF_MAGIC && header->bits == 1 && header->endianness == 1 && header->instructionSet == ELF_ARCH_X86;
}