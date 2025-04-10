#include <system.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <util.h>
#include <stdio.h>
#include <users.h>

static char workingdir[256] = {0};
static char cmdBuffer[256] = {0};
const char* prompt = "> ";

void PrintPrompt(){
    printf("[");
    // Get only the last part of the working directory
    if(workingdir == NULL){
        printf("ERROR");
    }else{
        char* lastPart = strrchr(workingdir, '/');
        if(strcmp(workingdir, "/") == 0){
            lastPart = workingdir;
        }else{
            lastPart++;
        }
        printf(lastPart);
    }
    printf("]");
    printf(prompt);
}

void ProcessCommand(char* cmdBuffer){
    // TODO: implement malloc and hash tables
    if(strcmp(cmdBuffer, "clear") == 0){
        write(STDOUT_FILENO, ANSI_ESCAPE, strlen(ANSI_ESCAPE));
    }else if(strcmp(cmdBuffer, "help") == 0){
        printf("Commands:\n");
        printf("clear: clears the screen\n");
        printf("help: prints this help message\n");
        printf("exit: exits the shell\n");
        printf("syscall: tests the system call mechanism\n");
        printf("pinfo: prints the process info of the shell\n");
        printf("time: prints the current time\n");
        printf("pwd: prints the current working directory\n");
        printf("ls: lists the files in the current directory\n");
        printf("cd: changes the current directory\n");
        printf("shutdown: shuts down the system\n");
        printf("sysinfo: prints system information\n");
    }else if(strcmp(cmdBuffer, "exit") == 0){
        exit(0);
    }else if(strcmp(cmdBuffer, "syscall") == 0){
        sys_debug();
    }else if(strcmp(cmdBuffer, "pinfo") == 0){
        pid_t currentPid = getpid();
        printf("Process ID: %d\n", currentPid);
        printf("Owner: ");
        if(currentPid == ROOT_UID){
            printf("root\n");
        }else{
            printf("user\n");
        }
    }else if(strcmp(cmdBuffer, "time") == 0){
        datetime_t time;
        gettime(&time);
        printf("%d/%d/%d %d:%d:%d\n", time.month, time.day, time.year, time.hour, time.minute, time.second);
    }else if(strcmp(cmdBuffer, "pwd") == 0){
        printf("%s\n", workingdir);
    }else if(strcmp(cmdBuffer, "ls") == 0){
        size_t i = 0;
        struct Node_Data node = {0};
        while(stat(NULL, i, &node) != FILE_NOT_FOUND){
            printf("%s\n", node.name);
            i++;
        }
    }else if(strncmp(cmdBuffer, "cd", 2) == 0){
        char* dir = cmdBuffer + 3;
        if(strlen(dir) == 0 || strlen(cmdBuffer) < 3){
            printf("Usage: cd <directory>\n");
            PrintPrompt();
            return;
        }
        
        if(chdir(dir) == STANDARD_FAILURE){
            printf("Error: directory %s does not exist\n", dir);
        }

        memset(workingdir, 0, sizeof(workingdir));
        getcwd(workingdir, sizeof(workingdir));
    }else if(strcmp(cmdBuffer, "shutdown") == 0){
        printf("Shutting down...\n");
        shutdown();
    }else if(strcmp(cmdBuffer, "sysinfo") == 0){
        printf("System information:\n");
        struct sysinfo info;
        sysinfo(&info);
        printf("Kernel version: %d.%d.%d\n", info.kernelVersion.major, info.kernelVersion.minor, info.kernelVersion.patch);
        printf("Kernel release: %s\n", info.kernelRelease);
        printf("CPU ID: %s\n", info.cpuID);
        printf("Uptime: %llu seconds\n", info.uptime);
        printf("Total memory: %d MiB\n", info.totalMemory / 1024 / 1024);
        printf("Used memory: %d MiB\n", info.usedMemory / 1024 / 1024);
        printf("Free memory: %d MiB\n", info.freeMemory / 1024 / 1024);
        printf("ACPI supported: ");
        if(info.acpiSupported){
            printf("yes\n");
        }else{
            printf("no\n");
    }
    }else{
        if(*cmdBuffer == 0){
            PrintPrompt();
            return;
        }

        int result = exec(cmdBuffer, NULL, NULL, 0);

        if(result == SYSCALL_TASKING_FAILURE){
            printf("Error: failed to load and execute file.\n");
        }else if(result == SYSCALL_FAULT_DETECTED){
            printf("Segmentation Fault\n");
        }else{
            printf("Program exited with code %d\n", result);
        }
    }
    // Print the prompt again
    PrintPrompt();
}

NORET void _start(){
    write(STDOUT_FILENO, ANSI_ESCAPE, sizeof(ANSI_ESCAPE));
    printf("Dedication OS shell (version 0.1)\n");
    printf("Type 'help' for a list of commands\n");
    getcwd(workingdir, sizeof(workingdir));
    PrintPrompt();
    while(1){
        memset(cmdBuffer, 0, sizeof(cmdBuffer));
        if(read(STDIN_FILENO, cmdBuffer, sizeof(cmdBuffer)) != FILE_READ_SUCCESS){
            printf("Error: failed to read from stdin\n");
            return;
        }
        size_t bufferLen = strlen(cmdBuffer);
        cmdBuffer[bufferLen - 1] = '\0'; // Remove the newline character
        ProcessCommand(cmdBuffer);
    }
    exit(0);
}
