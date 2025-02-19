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
#include <time.h>
#include <string.h>
#include <vfs.h>

// The maximum command size is 255 characters (leave space for null terminator) for now
#define CMD_MAX_SIZE 256

const char* prompt = "> ";

char* cmdBuffer = NULL;

uint8_t cmdBufferIndex = 0;

volatile bool enterPressed = false;         // Must be volatile so the compiler doesn't optimize it away
volatile bool exit = false;

pcb_t* shellPCB = NULL;

void PrintPrompt(){
    WriteChar('[');
    WriteString(shellPCB->workingDirectory);
    WriteChar(']');
    printf(prompt);
}

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
        printf("time: prints the current time\n");
        printf("settz: sets the timezone\n");
        printf("pwd: prints the current working directory\n");
        printf("ls: lists the files in the current directory\n");
        printf("cd: changes the current directory\n");
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
        do_syscall(SYS_GET_PCB, 0, 0, 0);
        asm volatile("mov %%eax, %0" : "=r" (shellPCB));
        printf("Process info:\n");
        printf("PID: %d\n", shellPCB->pid);
        printf("Name: %s\n", shellPCB->name);
        printf("Owner: %d ", shellPCB->owner);
        if(shellPCB->owner == ROOT_UID){
            printf("(root)\n");
        }else{
            printf("(user)\n");
        }
        printf("State: %d\n", shellPCB->state);
        printf("Priority: %d\n", shellPCB->priority);
        printf("Time slice: %d ms\n", shellPCB->timeSlice);
        printf("Stack: 0x%x\n", shellPCB->stack);
        printf("Stack base: 0x%x\n", shellPCB->stackBase);
        printf("Stack top: 0x%x\n", shellPCB->stackTop);
        printf("Heap base: 0x%x\n", shellPCB->heapBase);
        printf("Heap end: 0x%x\n", shellPCB->heapEnd);
        printf("Current working directory: %s\n", shellPCB->workingDirectory);
    }else if(strcmp(cmd, "ACPI") == 0){
        printf("ACPI info:\n");
        printf("ACPI version: %d\n", acpiInfo.version);
        printf("ACPI exists: %d\n", acpiInfo.exists);
        printf("ACPI FADT: 0x%x\n", acpiInfo.fadt);
        printf("ACPI RSDP: 0x%x\n", acpiInfo.rsdp.rsdpV1);
        printf("ACPI RSDT: 0x%x\n", acpiInfo.rsdt.rsdt);
    }else if(strcmp(cmd, "time") == 0){
        uint8_t hour = currentTime.hour;
        if(hour > 12){
            hour -= 12;
        }
        printf("Date: %d/%d/%d\n", currentTime.month, currentTime.day, currentTime.year);
        printf("Time: %d:%d:%d\n", hour, currentTime.minute, currentTime.second);
    }else if(strncmp(cmd, "settz", 5) == 0){
        // Assumes the clock is originally set to UTC
        char* tz = cmd + 6;
        if(strlen(tz) == 0 || strlen(cmd) < 6){
            printf("Usage: settz <timezone>\n");
            goto end;
        }
        printf("Setting timezone to %s\n", tz);
        if(strcmp(tz, "UTC") == 0){
            SetTime();
        }else if(strcmp(tz, "EST") == 0){
            // EST is UTC-5, calculate the offset
            if(currentTime.hour >= 5){
                currentTime.hour -= 5;
            }else{
                currentTime.hour += 19;
                if(currentTime.day > 1){
                    currentTime.day--;
                }else{
                    currentTime.day = 31;
                    if(currentTime.month > 1){
                        currentTime.month--;
                    }else{
                        currentTime.month = 12;
                        currentTime.year--;
                    }
                }
            }
        }else{
            printf("Only EST and UTC are supported\n");
        }
    }else if(strcmp(cmd, "pwd") == 0){
        printf("%s\n", shellPCB->workingDirectory);
    }else if(strcmp(cmd, "ls") == 0){
        vfs_node_t* current = VfsFindNode(shellPCB->workingDirectory);
        if(current == NULL){
            printf("Error: current directory does not exist\n");
            goto end;
        }
        if(!current->isDirectory){
            printf("Error: current directory is not a directory\n");
            goto end;
        }
        vfs_node_t* child = current->pointer.firstChild;
        for(size_t i = 0; i < current->size; i++){
            if(child != NULL){
                printf("%s\n", child->name);
                child = child->next;
            }
        }
    }else if(strncmp(cmd, "cd", 2) == 0){
        char* dir = cmd + 3;
        if(strlen(dir) == 0 || strlen(cmd) < 3){
            printf("Usage: cd <directory>\n");
            goto end;
        }
        vfs_node_t* current = VfsFindNode(shellPCB->workingDirectory);
        if(current == NULL){
            printf("Error: current directory does not exist\n");
            goto end;
        }
        if(!current->isDirectory){
            printf("Error: current directory is not a directory\n");
            goto end;
        }
        if(strcmp(dir, "..") == 0 && current->parent != NULL){
            //shellPCB->workingDirectory = current->parent->name;
            shellPCB->workingDirectory = GetFullPath(current->parent);
            goto end;
        }else if(strcmp(dir, ".") == 0){
            goto end;
        }
        vfs_node_t* newDir = VfsFindNode(dir);
        if(newDir != NULL && newDir->isDirectory){
            shellPCB->workingDirectory = GetFullPath(newDir);
            goto end;
        }
        printf("Error: directory %s does not exist\n", dir);
    }else if(strlen(cmd) != 0){
        printf("Unknown command: %s\n", cmd);
    }

    end:
    PrintPrompt();
}

int shell(void){
    ClearScreen();
    printf("Kernel-Integrated Shell (KISh)\n");
    printf("Type 'help' for a list of commands\n");
    do_syscall(SYS_GET_PCB, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (shellPCB));
    if(shellPCB == NULL){
        printf("Error finding process information!\n");
        return 1;
    }
    PrintPrompt();
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