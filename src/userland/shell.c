#include <kernel.h>
#include <console.h>
#include <keyboard.h>
#include <alloc.h>

// The maximum command size is 255 characters (leave space for null terminator) for now
#define CMD_MAX_SIZE 256

const char* prompt = "KISh> ";

char* cmdBuffer = NULL;

uint8_t cmdBufferIndex = 0;

volatile bool enterPressed = false;         // Must be volatile so the compiler doesn't optimize it away

// Handle a key press
void handler(KeyboardEvent_t event){
    if(event.keyUp){
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
    if(strcmp(cmd, "clear") == 0){
        ClearScreen();
        return;
    }

    if(strcmp(cmd, "help") == 0){
        printf("Help screen:\n");
        printf("This is a stub!\n");
        return;
    }

    printf("Invalid Command!\n");
}

int shell(void){
    ClearScreen();
    printf("KISh Shell\n");
    printf("Type 'help' for a list of commands\n");
    printf(prompt);
    InstallKeyboardCallback(handler);
    cmdBuffer = (char*)halloc(CMD_MAX_SIZE);
    for(;;){
        if(enterPressed){
            cmdBuffer[cmdBufferIndex] = '\0';
            ProcessCommand(cmdBuffer);
            cmdBufferIndex = 0;
            enterPressed = false;
            printf(prompt);
        }
    }
    return 0;
}