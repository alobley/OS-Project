/* 
 * KISh (Kernel-Integrated Shell)
 * This is a simple shell built into the kernel's binary specifically meant to test its functionality. It is not meant to be a full-featured shell.
 * It processes a few commands raw without any API assistance.
 * 
*/

#include <kernel.h>
#include <keyboard.h>
#include <alloc.h>
#include <time.h>
#include <multitasking.h>
#include <acpi.h>

// The maximum command size is 255 characters (leave space for null terminator) for now
#define CMD_MAX_SIZE 256

const char* prompt = "KISh> ";

char* cmdBuffer = NULL;

uint8_t cmdBufferIndex = 0;

volatile bool enterPressed = false;         // Must be volatile so the compiler doesn't optimize it away
volatile bool exit = false;

// Handle a key press
void handler(KeyboardEvent_t event){
    if(event.keyUp || event.ascii == 0){
        return;
    }
    switch(event.ascii){
        case '\n': {
            printf("\n");
            enterPressed = true;
            break;
        }
        case '\b': {
            if(cmdBufferIndex > 0){
                cmdBufferIndex--;
                WriteChar(event.ascii);
            }
            break;
        }
        default: {
            if(cmdBufferIndex < CMD_MAX_SIZE - 1){
                cmdBuffer[cmdBufferIndex] = event.ascii;
                cmdBufferIndex++;
                WriteChar(event.ascii);
            }
            break;
        }
    }
}

void ProcessCommand(char* cmd){
    // TODO: Implement a proper command parser (this is crazy inefficient)
    if(strcmp(cmd, "clear") == 0){
        ClearScreen();
    }else if(strcmp(cmd, "meminfo") == 0){
        printf("Total memory: %d MiB\n", memSizeMiB);
        printf("Used memory: %d MiB\n", totalPages * PAGE_SIZE / 1024 / 1024);
    }else if(strcmp(cmd, "help") == 0){
        printf("Commands:\n");
        printf("clear: clears the screen\n");
        printf("meminfo: prints some memory info\n");
        printf("help: prints this help message\n");
        printf("exit: exits the shell\n");
        printf("syscall: tests the system call mechanism\n");
        printf("version: prints the kernel version\n");
        printf("pinfo: prints the process info of the shell\n");
        printf("ACPI: prints ACPI info\n");
    }else if(strcmp(cmd, "exit") == 0){
        printf("Exiting shell...\n");
        exit = true;
        return;
    }else if(strcmp(cmd, "syscall") == 0){
        uint32_t result = 0;
        do_syscall(SYS_DBG, 0, 0, 0);
        asm volatile("mov %%eax, %0" : "=r" (result));
        printf("System call returned: %d\n", result);
    }else if(strcmp(cmd, "version") == 0){
        printf("Dedication OS Version: %d.%d.%d %s\n", kernelVersion.major, kernelVersion.minor, kernelVersion.patch, kernelRelease);
    }else if(strcmp(cmd, "pinfo") == 0){
        pcb_t* currentProcess;
        do_syscall(SYS_GET_PCB, 0, 0, 0);
        asm volatile("mov %%eax, %0" : "=r" (currentProcess));
        printf("Process info:\n");
        printf("PID: %d\n", currentProcess->pid);
        printf("Name: %s\n", currentProcess->name);
        printf("Owner: %d\n", currentProcess->owner);
        printf("State: %d\n", currentProcess->state);
        printf("Priority: %d\n", currentProcess->priority);
        printf("Time slice: %d ms\n", currentProcess->timeSlice);
        printf("Stack: 0x%x\n", currentProcess->stack);
        printf("Stack base: 0x%x\n", currentProcess->stackBase);
        printf("Stack top: 0x%x\n", currentProcess->stackTop);
        printf("Heap base: 0x%x\n", currentProcess->heapBase);
        printf("Heap end: 0x%x\n", currentProcess->heapEnd);
    }else if(strcmp(cmd, "ACPI") == 0){
        printf("ACPI info:\n");
        printf("ACPI version: %d\n", acpiInfo.version);
        printf("ACPI exists: %d\n", acpiInfo.exists);
        printf("ACPI FADT: 0x%x\n", acpiInfo.fadt);
        printf("ACPI RSDP: 0x%x\n", acpiInfo.rsdp.rsdpV1);
        printf("ACPI RSDT: 0x%x\n", acpiInfo.rsdt.rsdt);
    }else if(strlen(cmd) != 0){
        printf("Unknown command: %s\n", cmd);
    }

    printf(prompt);
}

int shell(void){
    ClearScreen();
    write("Kernel-Integrated Shell (KISh)\n", STDOUT);
    write("Type 'help' for a list of commands\n", STDOUT);
    write(prompt, STDOUT);
    do_syscall(SYS_INSTALL_KBD_HANDLE, (uint32_t)handler, 0, 0);
    cmdBuffer = (char*)halloc(CMD_MAX_SIZE);
    while(!exit){
        if(enterPressed){
            cmdBuffer[cmdBufferIndex] = '\0';
            cmdBufferIndex = 0;
            enterPressed = false;
            ProcessCommand(cmdBuffer);
        }
    }
    do_syscall(SYS_REMOVE_KBD_HANDLE, (uint32_t)handler, 0, 0);
    return 0;
}