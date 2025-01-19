#include <kernel.h>

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
    InitializeKeyboard();
    InitializeDisks();               // Reading more than one sector from the disk causes the strangest bug I've ever seen
}


// DOES NOT SUPPORT PAGING YET
int32 ExecuteProgram(file_t* program){
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

pcb_t* currentProcess = NULL;

void TaskScheduler(){
    // This is where the task scheduler will go
}

version_t version = {0, 0, 1};

// The kernel's main function
void kernel_main(uint32 magic, mboot_info_t* multibootInfo){
    InitializeACPI();
    // Getting paging to work took me FIFTY HOURS. PAGING ISN'T EVEN FULLY IMPLEMENTED YET.
    PageKernel((multibootInfo->memLower + multibootInfo->memUpper + 1024) * 1024, multibootInfo->mmapAddr, multibootInfo->mmapLen);
    InitVGA();

    printk("Dedication OS Version %u.%u.%u\n", version.major, version.minor, version.patch);
    InitializeHardware();

    // Launch the shell
    int value = CliHandler(multibootInfo);

    // After protected memory is done, put a task scheduler here

    // Well, it's done. Time to put a task scheduler here.

    reboot();

    for(;;) asm("hlt");
}