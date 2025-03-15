#include <bootutil.h>
#include <bootfs.h>

#define LINE(x) (x*160)
#define DEFAULT_ATTR 0x07

uint16_t currentLine = 9;
boot_info_t bootinfo;

void PutChar(char c, uint8_t attr, uint16_t offset){
    pushgs
    setgs(0xB800);
    setdi(offset);
    setal(c);
    setah(attr);
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

chs_address LbaToChs(uint16_t lba, bpb* bootfs){
    chs_address chs;
    uint16_t sectorsPerTrack = bootfs->sectorsPerTrack;
    uint16_t numHeads = bootfs->numHeads;

    if(sectorsPerTrack == 0 || numHeads == 0){
        WriteString("Invalid geometry", LINE(currentLine));
        currentLine++;
        STOP
    }
    
    // Calculate sector (1-based)
    chs.sector = (lba % sectorsPerTrack) + 1;
    
    // Calculate head and cylinder in steps to avoid potential overflow
    uint16_t headCylSectors = lba / sectorsPerTrack;
    chs.head = headCylSectors % numHeads;
    chs.cylinder = headCylSectors / numHeads;
    
    return chs;
}

// Read up to 256 sectors from a disk with a 16-bit LBA
void ReadSectors(uint16_t lba, uint8_t count, uint16_t offset, bpb* bootfs){
    chs_address chs = LbaToChs(lba, bootfs);
    setah(0x02);
    setal(count);
    setch(chs.cylinder);
    setcl(chs.sector);
    setdh(chs.head);
    setdl(bootfs->exboot.fat12_16.driveNumber);             // Set by the stage 0 bootloader
    setbx(offset);
    doint(0x13);

    uint8_t status;
    getah(status);
    if(status != 0){
        WriteString("Error reading sectors", LINE(currentLine));
        currentLine++;
        STOP
    }
}

// For finding file names
inline int strncmp(const char* a, const char* b, int n){
    while(n--){
        if(*a != *b){
            return 1;
        }
        a++;
        b++;
    }
    return 0;
}

uint8_t buffer[512];

void ReadFile(const char* filename, bpb* bootfs){
    // Read the root directory...
    // Find the file...
    // Read the file...
}

// Copy the buffer at ES to a near pointer
void ReadBuffer(){
    uint16_t di = 0;
    uint8_t al = 0;
    setdi(di);
    for(int i = 0; i < 512; i++){
        asm("movb %es:(%di), %al");
        getal(al);
        buffer[i] = al;
        di++;
    }
}

// Memory map is located at 0xC0000 in linear memory
// ES is at 0x200000 for kernel loading
// Number of entries is passed in the CX register (easier than messing with the stack)
FASTCALL NORET void stage2_main(uint16_t mmapEntries) {
    WriteString("Booting OS...", LINE(currentLine));
    bpb* bootfs = (bpb*)0x7C00;                 // The first sector is already loaded, no need to do it again

    bootinfo.memmap_address = 0xC0000;          // Defined in assembly
    bootinfo.memmap_length = mmapEntries;

    // Read the kernel from the root directory...

    ReadSectors(0, 1, 0, bootfs);        // Test read one sector from the disk

    ReadBuffer();

    if(buffer[511] != 0xAA || buffer[512] != 0x55){
        WriteString("Invalid boot signature", LINE(currentLine));
        currentLine++;
        STOP
    }else{
        WriteString("Boot signature valid", LINE(currentLine));
        currentLine++;
    }

    // The following will likely be done in assembly

    // Enter protected mode...

    // Jump to the kernel...

    currentLine++;

    STOP
    UNREACHABLE
}
