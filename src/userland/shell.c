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
    }else if(strcmp(cmd, "exit") == 0){
        printf("Exiting shell...\n");
        exit = true;
        return;
    }else if(strcmp(cmd, "syscall") == 0){
        uint32_t result = 0;
        do_syscall(SYS_DBG, 0, 0, 0);
        asm volatile("mov %%eax, %0" : "=r" (result));
        printf("System call returned: %d\n", result);
    }else{
        printf("Unknown command: %s\n", cmd);
    }

    printf(prompt);
}

int shell(void){
    ClearScreen();
    printf("Kernel-Integrated Shell (KISh)\n");
    printf("Type 'help' for a list of commands\n");
    printf(prompt);
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
    RemoveKeyboardCallback(handler);
    return 0;
}