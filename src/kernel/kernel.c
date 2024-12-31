#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <vga.h>
#include <keyboard.h>
#include <alloc.h>
#include <time.h>
#include <fpu.h>
#include <pcspkr.h>
#include <string.h>
#include <ata.h>
#include <multiboot.h>
#include <fat.h>
#include <paging.h>

#define MULTIBOOT_MAGIC 0x2BADB002

// To update:
// Do git add [filename], or git add .
// Do git commit -m "Say what you did"
// To push, do git push -u origin main

// Reference a small example for showcase
extern int CliHandler();

// 8042 reset
void reboot(){
    uint8 good = 0x02;
    while(good & 0x02){
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
}

// QEMU and Bochs only. ACPI support pending.
void shutdown(){
    // Try QEMU shutdown
    outw(0x604, 0x2000);

    // Try Bochs shutdown
    outw(0xB004, 0x2000);
}

// An array of pointers to all the ATA disks
disk_t* disks[MAX_DRIVES];

// An array of pointers to all possible FAT disks
fat_disk_t* fatdisks[MAX_DRIVES];

void InitializeDisks(){
    for(int disk = 0; disk < MAX_DRIVES; disk++){
        disks[disk] = IdentifyDisk(disk);
    }

    for(int i = 0; i < MAX_DRIVES; i++){
        fatdisks[i] = ParseFilesystem(disks[i]);
        fat_disk_t* fatdisk = fatdisks[i];
    }
}

// Initializes all the required components
void InitializeHardware(){
    InitIDT();
    InitISR();
    InitializeFPU();
    InitIRQ();
    InitializePIT();
    InitializeKeyboard();
    InitializeDisks();
}

int32 ExecuteProgram(void *program) {
    if (program == NULL) {
        printk("Error: Program is NULL!\n");
        return -1;
    }

    // Cast the program data to a function pointer
    int (*func)() = (int (*)()) program;

    // Execute the program and return its result
    int result = func();
    return result;
}

// The kernel's main function
void kernel_main(uint32 magic, mboot_info_t* multibootInfo){
    if(magic != MULTIBOOT_MAGIC){
        // There was a problem, reboot
        WriteStr("WARNING: no multiboot magic number.\n");
        reboot();
    }
    // Dynamic memory achieved!
    InitializeMemory((multibootInfo->memLower + multibootInfo->memUpper + 1024) * 1024);
    InitPaging((multibootInfo->memLower + multibootInfo->memUpper + 1024) * 1024);
    InitializeHardware();

    // Launch the shell
    int value = CliHandler(multibootInfo);

    // Temporary solution since the shell is built-in. We shouldn't return from it.
    printk("CRITICAL ERROR! Rebooting...\n");

    delay(1000);

    reboot();

    for(;;) asm("hlt");
}