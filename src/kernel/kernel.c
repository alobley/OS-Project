#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <vga.h>
#include <keyboard.h>
#include <time.h>
#include <fpu.h>
#include <pcspkr.h>
#include <string.h>
#include <ata.h>
#include <multiboot.h>
#include <fat.h>
#include <vfs.h>
#include <acpi.h>
#include <memmanage.h>
//#include <alloc.h>
//#include <paging.h>

#define MULTIBOOT_MAGIC 0x2BADB002

// To update:
// Do git add [filename], or git add .
// Do git commit -m "Say what you did"
// To push, do git push -u origin main

// Reference the integrated shell
extern int CliHandler();

// 8042 reset
void reboot(){
    uint8 good = 0x02;
    while(good & 0x02){
        good = inb(0x64);
    }
    outb(0x64, 0xFE);

    // Just in case that didn't work
    asm volatile("lidt 0");         // Load a null IDT
    asm volatile("int $0x30");      // Triple fault
}

// QEMU and Bochs only.
void shutdown(){
    // Try QEMU shutdown
    outw(0x604, 0x2000);

    // Try Bochs shutdown
    outw(0xB004, 0x2000);
}

// Initializes all the required components
void InitializeHardware(){
    InitIDT();
    InitISR();
    InitializeFPU();
    InitIRQ();
    InitializePIT();
    //InitializeACPI();
    //InitializeKeyboard();
    InitializeDisks();
}

int32 ExecuteProgram(file_t* program) {
    if (program == NULL) {
        printk("Error: Program is NULL!\n");
        return -1;
    }

    // Copy the program to physical address 0
    memcpy((void*)0, program->data, program->size);

    // Cast the program data to a function pointer
    int (*func)() = (int (*)()) 0;

    // Execute the program and return its result
    int result = func();
    memset((void*)0, 0, program->size);
    return result;
}

extern uint32 __kernel_end;

// The kernel's main function
void kernel_main(uint32 magic, mboot_info_t* multibootInfo){
    PageKernel((multibootInfo->memLower + multibootInfo->memUpper + 1024) * 1024);
    InitVGA();
    WriteStr("Dedication OS Version 0.0.1\n");
    STOP;
    InitializeHardware();
    if(magic != MULTIBOOT_MAGIC){
        // There was a problem, may not be vital
        WriteStr("WARNING: no multiboot magic number.\n");
    }
    // Launch the shell
    int value = CliHandler(multibootInfo);

    // After protected memory is done, put a task scheduler here

    reboot();

    for(;;) asm("hlt");
}