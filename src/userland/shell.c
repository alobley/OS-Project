#include <alloc.h>
#include <string.h>
#include <vfs.h>
#include <hash.h>
#include <console.h>
#include <acpi.h>
#include <keyboard.h>

size_t dirSize = 1024;
hash_table_t* cmdTable;

// The maximum command size is 255 characters (leave space for null terminator) for now
#define CMD_MAX_SIZE 256

const char* prompt = "> ";

char* cmdBuffer = NULL;

uint8_t cmdBufferIndex = 0;

volatile bool enterPressed = false;         // Must be volatile so the compiler doesn't optimize it away
volatile bool done = false;

char* currentWorkingDir = NULL;

void PrintPrompt(){
    printk("[");
    // Get only the last part of the working directory
    if(currentWorkingDir == NULL){
        printk("ERROR");
    }else{
        char* lastPart = strrchr(currentWorkingDir, '/');
        if(strcmp(currentWorkingDir, VFS_ROOT) == 0){
            lastPart = currentWorkingDir;
        }else{
            lastPart++;
        }

        printk(lastPart);
    }
    printk("]");
    printk(prompt);
}

// Handle a key press
void handler(KeyboardEvent_t event){
    if(event.keyUp || event.ascii == 0){
        return;
    }
    switch(event.ascii){
        case '\n': {
            printk("\n");
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
    ClearScreen();
    //write(STDOUT_FILENO, ANSI_ESCAPE, strlen(ANSI_ESCAPE));
}

void help(UNUSED char* cmd){
    printk("Commands:\n");
    printk("clear: clears the screen\n");
    printk("help: prints this help message\n");
    printk("exit: exits the shell\n");
    printk("syscall: tests the system call mechanism\n");
    printk("time: prints the current time\n");
    printk("settz: sets the timezone\n");
    printk("pwd: prints the current working directory\n");
    printk("ls: lists the files in the current directory\n");
    printk("cd: changes the current directory\n");
    printk("shutdown: shuts down the system\n");
    printk("sysinfo: prints system information\n");
}

void exitShell(UNUSED char* cmd){
    printk("Exiting shell...\n");
    done = true;
}

void syscall(UNUSED char* cmd){
    int result = 0;
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);
    getresult(result);
    printk("System call returned: 0x%x\n", result);
}

void time(UNUSED char* cmd){
    uint8_t hour = currentTime.hour;
    if(hour > 12){
        hour -= 12;
    }
    printk("Date: %d/%d/%d\n", currentTime.month, currentTime.day, currentTime.year);
    printk("Time: %d:%d:%d\n", hour, currentTime.minute, currentTime.second);
}

void settz(char* cmd){
    // Assumes the clock is originally set to UTC
    char* tz = cmd + 6;
    if(strlen(tz) == 0 || strlen(cmd) < 6){
        printk("Usage: settz <timezone>\n");
        return;
    }
    printk("Setting timezone to %s\n", tz);
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
        printk("Only EST and UTC are supported.\n");
    }
}

void pwd(UNUSED char* cmd){
    // Allocate a buffer for the directory
    printk("%s\n", currentWorkingDir);
}

void ls(UNUSED char* cmd){
    vfs_node_t* current = VfsFindNode(currentWorkingDir);
    if(current == NULL){
        printk("Error: current directory does not exist\n");
        return;
    }
    if(!(current->flags & NODE_FLAG_DIRECTORY)){
        printk("Error: current directory is not a directory\n");
        return;
    }
    if(strcmp(current->name, "/") != 0){
        // printk .. for the previous directory
        printk("..\n");
    }
    vfs_node_t* child = current->firstChild;
    for(size_t i = 0; i < current->size; i++){
        if(child != NULL){
            printk("%s\n", child->name);
            child = child->next;
        }
    }
}

void cd(char* cmd){
    char* dir = cmd + 3;
    if(strlen(dir) == 0 || strlen(cmd) < 3){
        printk("Usage: cd <directory>\n");
        return;
    }
    
    int result = 0;
    do_syscall(SYS_CHDIR, (uint32_t)dir, strlen(dir), 0, 0, 0);
    getresult(result);
    if(result != SYSCALL_SUCCESS){
        printk("Error: directory %s does not exist\n", dir);
    }

    memset(currentWorkingDir, 0, strlen(currentWorkingDir));
    do_syscall(SYS_GETCWD, (uint32_t)currentWorkingDir, dirSize, 0, 0, 0);
}

void shutdown_system(UNUSED char* cmd){
    AcpiShutdown();
    printk("Shutdown failed. It is safe to turn off your computer.\n");
    while(1);
}

void systeminfo(UNUSED char* cmd){
    printk("System information:\n");
    struct uname info;
    do_syscall(SYS_UNAME, (uint32_t)&info, 0, 0, 0, 0);
    printk("Kernel version: %d.%d.%d\n", info.kernelVersion.major, info.kernelVersion.minor, info.kernelVersion.patch);
    printk("Kernel release: %s\n", info.kernelRelease);
    printk("CPU ID: %s\n", info.cpuOEM);
    printk("Uptime: %llu seconds\n", info.uptime);
    printk("Total memory: %d MiB\n", info.totalMemory / (1024 * 1024));
    printk("Used memory: %d.%d MiB\n", info.usedMemory / (1024 * 1024), (info.usedMemory / 1024) % 1024);
    printk("Free memory: %d.%d MiB\n", info.freeMemory / (1024 * 1024), (info.freeMemory / 1024) % 1024);
    printk("ACPI supported: ");
    if(info.acpiSupported){
        printk("yes\n");
    }else{
        printk("no\n");
    }
}

void ProcessCommand(char* cmd){
    if(strlen(cmd) == 0){
        PrintPrompt();
        cmdBufferIndex = 0;
        memset(cmdBuffer, 0, CMD_MAX_SIZE);
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
        // Search for a program to execute
        if(*cmd == 0){
            PrintPrompt();
            cmdBufferIndex = 0;
            memset(cmdBuffer, 0, CMD_MAX_SIZE);
            return;
        }

        int result = SYSCALL_TASKING_FAILURE;

        if(result == SYSCALL_TASKING_FAILURE){
            printk("Error: failed to load and execute file.\n");
        }else if(result == SYSCALL_FAULT_DETECTED){
            printk("Segmentation Fault\n");
        }else{
            printk("Program exited with code %d\n", result);
        }
    }
    cmdBufferIndex = 0;
    memset(cmdBuffer, 0, CMD_MAX_SIZE);
    if(!done){
        PrintPrompt();
    }
}

// Slowly but surely, this shell is leaning away from directly calling kernel functions and moving towards using system calls.
// Most recent removal:
// - Program loading and execution
int shell(void){
    do_syscall(SYS_WRITE, STDOUT_FILENO, (uint32_t)ANSI_ESCAPE, strlen(ANSI_ESCAPE), 0, 0);

    InstallKeyboardCallback(handler);

    // Create a table with a default size of 30
    cmdTable = CreateTable(30);

    // Add all the commands
    HashInsert(cmdTable, "clear", clear);
    HashInsert(cmdTable, "help", help);
    HashInsert(cmdTable, "exit", exitShell);
    HashInsert(cmdTable, "syscall", syscall);
    HashInsert(cmdTable, "time", time);
    HashInsert(cmdTable, "settz", settz);
    HashInsert(cmdTable, "pwd", pwd);
    HashInsert(cmdTable, "ls", ls);
    HashInsert(cmdTable, "cd", cd);
    HashInsert(cmdTable, "shutdown", shutdown_system);
    HashInsert(cmdTable, "sysinfo", systeminfo);

    // Test the write system call by using it to printk the welcome message
    printk("Kernel-Integrated Shell (KISh)\n");
    printk("Type 'help' for a list of commands\n");


    currentWorkingDir = (char*)halloc(dirSize);
    if(currentWorkingDir == NULL){
        printk("Error: failed to allocate memory for current working directory\n");
        return STANDARD_FAILURE;
    }
    memset(currentWorkingDir, 0, dirSize);
    do_syscall(SYS_GETCWD, (uint32_t)currentWorkingDir, dirSize, 0, 0, 0);

    PrintPrompt();
    cmdBuffer = (char*)halloc(CMD_MAX_SIZE);
    if(cmdBuffer == NULL){
        printk("Error: failed to allocate memory for command buffer\n");
        return STANDARD_FAILURE;
    }
    memset(cmdBuffer, 0, CMD_MAX_SIZE);

    while(!done){
        if(enterPressed){
            cmdBuffer[cmdBufferIndex] = '\0';
            enterPressed = false;
            ProcessCommand(cmdBuffer);
        }
    }
    
    hfree(cmdBuffer);
    ClearTable(cmdTable);
    return STANDARD_SUCCESS;
}