#include <bootutil.h>
#include <bootfs.h>

#define LINE(x) (x*160)
#define DEFAULT_ATTR 0x07

void PutChar(char c, uint8_t attr, uint16_t offset){
    pushgs
    setes(0xB800);
    setdi(offset);
    asm("movb %0, %%al"::"g"(c));
    asm("movb %0, %%ah"::"g"(attr));
    asm("movw %ax, %gs:(%di)");
    popgs
}

void WriteString(const char *str, uint16_t offset){
    int i = offset;
    while(*str){
        PutChar(*str, DEFAULT_ATTR, i);
        str++;
        i += 2;
    }
}

void Clear(){
    int i;
    for(i = 0; i < 80*25; i++){
        PutChar(' ', DEFAULT_ATTR, i*2);
    }
}

uint16_t currentLine = 9;
boot_info_t bootinfo;

ALIGNED(16) gdt_t gdt;

ALIGNED(16) gdtptr_t gdtptr = {
    .limit = sizeof(gdt) - 1,
    .base = (uint32_t)&gdt
};


// Memory map is located at 0xC0000 in linear memory
// Number of entries is passed in the CX (easier than messing with the stack)
FASTCALL NORET void stage2_main(uint16_t mmapEntries) {
    WriteString("Booting OS...", LINE(currentLine));
    ebr* bootfs = (ebr*)0x7C00;                 // The first sector is already loaded, no need to do it again

    bootinfo.memmap_address = 0xC0000;          // Defined in assembly
    bootinfo.memmap_length = mmapEntries;



    currentLine++;
    STOP
    UNREACHABLE
}
