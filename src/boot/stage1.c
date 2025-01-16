#include "bootutil.h"
#include "bios.h"

extern void print();
extern void ClearScreen();
extern void disk_read();
extern void LoadKernel();

mmap_entry_t* memory_map = (mmap_entry_t*)0x80000;

void PrintString(char* str){
    // Point DI specifically to the string
    setdi((uint16)str);
    print();        // Call the ASM function
}

void Stage1_Start(){
    PrintString("Loading Kernel...");
    fat12_boot_sector_t* boot_sector = (fat12_boot_sector_t*)0x7C00;
    uint16 rootlba = boot_sector->reserved_sectors + (boot_sector->fat_count * boot_sector->sectors_per_fat);
    uint16 rootsize = (boot_sector->root_dir_entries * 32) / boot_sector->bytes_per_sector;
    if((boot_sector->root_dir_entries * 32) % boot_sector->bytes_per_sector != 0){
        rootsize++;
    }
    setcx((uint16)(rootsize & 0xFF));
    setdx((uint16)(boot_sector->driveNum & 0xFF));
    setbx((uint16)0);
    setax(rootlba);
    setes(0x2000);
    disk_read();

    fat12_entry_t* root = (fat12_entry_t*)0x20000;
    setax((uint16)(root->filename[0]));

    STOP;
}