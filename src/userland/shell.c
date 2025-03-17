#include <kernel.h>
#include <keyboard.h>
#include <alloc.h>
#include <time.h>
#include <multitasking.h>
#include <acpi.h>
#include <time.h>
#include <string.h>
#include <vfs.h>
#include <hash.h>
#include <system.h>

hash_table_t* cmdTable;

// The maximum command size is 255 characters (leave space for null terminator) for now
#define CMD_MAX_SIZE 256

const char* prompt = "> ";

char* cmdBuffer = NULL;

uint8_t cmdBufferIndex = 0;

volatile bool enterPressed = false;         // Must be volatile so the compiler doesn't optimize it away
volatile bool done = false;

pcb_t* shellPCB = NULL;

void PrintPrompt(){
    WriteChar('[');
    // Get only the last part of the working directory
    if(shellPCB == NULL || shellPCB->workingDirectory == NULL){
        WriteString("ERROR");
    }else{
        char* lastPart = strrchr(shellPCB->workingDirectory, '/');
        if(strlen(shellPCB->workingDirectory) == strlen(VFS_ROOT)){
            lastPart = shellPCB->workingDirectory;
        }else{
            lastPart++;
        }
        WriteString(lastPart);
    }
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

void clear(UNUSED char* cmd){
    write(STDOUT_FILENO, ANSI_ESCAPE, strlen(ANSI_ESCAPE));
}

void help(UNUSED char* cmd){
    printf("Commands:\n");
    printf("clear: clears the screen\n");
    printf("help: prints this help message\n");
    printf("exit: exits the shell\n");
    printf("syscall: tests the system call mechanism\n");
    printf("pinfo: prints the process info of the shell\n");
    printf("time: prints the current time\n");
    printf("settz: sets the timezone\n");
    printf("pwd: prints the current working directory\n");
    printf("ls: lists the files in the current directory\n");
    printf("cd: changes the current directory\n");
    printf("shutdown: shuts down the system\n");
    printf("sysinfo: prints system information\n");
}

void exitShell(UNUSED char* cmd){
    printf("Exiting shell...\n");
    done = true;
}

void syscall(UNUSED char* cmd){
    int result = 0;
    result = sys_debug();
    printf("System call returned: 0x%x\n", result);
}

void pinfo(UNUSED char* cmd){
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
    printf("Stack base: 0x%x\n", shellPCB->stackBase);
    printf("Stack top: 0x%x\n", shellPCB->stackTop);
    printf("Heap base: 0x%x\n", shellPCB->heapBase);
    printf("Heap end: 0x%x\n", shellPCB->heapEnd);
    printf("Current working directory: %s\n", shellPCB->workingDirectory);
}

void time(UNUSED char* cmd){
    uint8_t hour = currentTime.hour;
    if(hour > 12){
        hour -= 12;
    }
    printf("Date: %d/%d/%d\n", currentTime.month, currentTime.day, currentTime.year);
    printf("Time: %d:%d:%d\n", hour, currentTime.minute, currentTime.second);
}

void settz(char* cmd){
    // Assumes the clock is originally set to UTC
    char* tz = cmd + 6;
    if(strlen(tz) == 0 || strlen(cmd) < 6){
        printf("Usage: settz <timezone>\n");
        return;
    }
    printf("Setting timezone to %s\n", tz);
    if(strcmp(tz, "UTC") == 0){
        SetTime();
    }else if(strcmp(tz, "EST") == 0){
        // EST is UTC-4 during Spring to Fall and it is UTC-5 during Fall to Spring, calculate the offset
        // TODO: make this more advanced or (hopefully) wait for daylight savings to be abolished
        if(currentTime.hour >= 4){
            currentTime.hour -= 4;
        }else{
            currentTime.hour += 20;
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
}

void pwd(UNUSED char* cmd){
    // Allocate a buffer for the directory
    size_t dirSize = 1024;
    char* dir = (char*)halloc(dirSize);
    if(dir == NULL){
        printf("Error: failed to allocate memory for directory\n");
        return;
    }
    if(getcwd(dir, dirSize) != STANDARD_FAILURE){
        printf("%s\n", dir);
    }else{
        printf("Error: failed to get current working directory\n");
    }
    hfree(dir);
}

void ls(UNUSED char* cmd){
    vfs_node_t* current = VfsFindNode(shellPCB->workingDirectory);
    if(current == NULL){
        printf("Error: current directory does not exist\n");
        return;
    }
    if(!current->isDirectory){
        printf("Error: current directory is not a directory\n");
        return;
    }
    if(strcmp(current->name, "/") != 0){
        // Print .. for the previous directory
        printf("..\n");
    }
    vfs_node_t* child = current->firstChild;
    for(size_t i = 0; i < current->size; i++){
        if(child != NULL){
            printf("%s\n", child->name);
            child = child->next;
        }
    }
}

void cd(char* cmd){
    char* dir = cmd + 3;
    if(strlen(dir) == 0 || strlen(cmd) < 3){
        printf("Usage: cd <directory>\n");
        return;
    }
    
    if(chdir(dir) == STANDARD_FAILURE){
        printf("Error: directory %s does not exist\n", dir);
    }
}

void shutdown(UNUSED char* cmd){
    AcpiShutdown();
}

void sysinfo(UNUSED char* cmd){
    printf("System information:\n");
    printf("Kernel version: %d.%d.%d %s\n", kernelVersion.major, kernelVersion.minor, kernelVersion.patch, kernelRelease);
    printf("Total memory: %d MiB\n", memSizeMiB);
    printf("Used memory: %d MiB\n", totalPages * PAGE_SIZE / 1024 / 1024);
    printf("ACPI supported: ");
    if(acpiInfo.exists){
        printf("yes\n");
    }else{
        printf("no\n");
    }

    uint32_t eax = 0;
    uint32_t others[4] = {0};
    cpuid(eax, others[0], others[1], others[2]);
    // Combine the manufacturer string into a single string
    printf("CPUID: 0x%x\n", eax);
    printf("CPU Manufacturer: %s\n", others);
}

void ProcessCommand(char* cmd){
    if(strlen(cmd) == 0){
        PrintPrompt();
        return;
    }

    // Split the command into the command and the arguments
    char* args = strchr(cmd, ' ');
    if(args != NULL){
        *args = '\0';
        args++;
    }

    hash_entry_t* entry = hash(cmdTable, cmd);
    if(entry != NULL){
        if(args != NULL){
            args--;
            *args = ' ';
        }
        entry->func(cmd);
    }else{
        printf("Command not found: %s\n", cmd);
    }
    if(!done){
        PrintPrompt();
    }
}

int shell(void){
    ClearScreen();

    // Create a table with a default size of 30
    cmdTable = CreateTable(30);

    // Add all the commands
    HashInsert(cmdTable, "clear", clear);
    HashInsert(cmdTable, "help", help);
    HashInsert(cmdTable, "exit", exitShell);
    HashInsert(cmdTable, "syscall", syscall);
    HashInsert(cmdTable, "pinfo", pinfo);
    HashInsert(cmdTable, "time", time);
    HashInsert(cmdTable, "settz", settz);
    HashInsert(cmdTable, "pwd", pwd);
    HashInsert(cmdTable, "ls", ls);
    HashInsert(cmdTable, "cd", cd);
    HashInsert(cmdTable, "shutdown", shutdown);
    HashInsert(cmdTable, "sysinfo", sysinfo);

    printf("Kernel-Integrated Shell (KISh)\n");
    printf("Type 'help' for a list of commands\n");

    shellPCB = GetCurrentProcess();
    if(shellPCB == NULL){
        printf("Error: shell PCB does not exist!\n");
        return STANDARD_FAILURE;
    }

    PrintPrompt();
    cmdBuffer = (char*)halloc(CMD_MAX_SIZE);
    if(cmdBuffer == NULL){
        printf("Error: failed to allocate memory for command buffer\n");
        return STANDARD_FAILURE;
    }

    install_keyboard_handler(handler);
    while(!done){
        if(enterPressed){
            cmdBuffer[cmdBufferIndex] = '\0';
            cmdBufferIndex = 0;
            enterPressed = false;
            ProcessCommand(cmdBuffer);
        }
    }

    remove_keyboard_handler(handler);
    hfree(cmdBuffer);
    ClearTable(cmdTable);
    return STANDARD_SUCCESS;
}