#include "keyboard.h"
#include "io.h"
#include <isr.h>
#include <idt.h>
#include <irq.h>
#include <vga.h>
#include <time.h>
#include <util.h>

#define KEYBOARD_ISR 0x21
#define KEYBOARD_IRQ 1

static bool keysDown[256];
static uint8 lastKeyPressed = 0;

void WaitForKeyPress(){
    uint8 currentKey = 0;
    while(currentKey == 0){
        currentKey = GetKey();
    }
}

void WaitForRelease(uint8 ScanCode){
    while(IsKeyPressed(ScanCode));
}

bool shiftPressed = false;


void kb_handler(){
    uint8 scanCode;
    if(inb(KBD_STATUS_PORT) & 0x01){
        // If there is anything to retrieve, retrieve it
        scanCode = inb(KBD_DATA_PORT);
    }else{
        return;
    }


    if(scanCode & EVENT_KEYUP){
        // On key release
        if((scanCode ^ EVENT_KEYUP) == LSHIFT || (scanCode ^ EVENT_KEYUP) == RSHIFT){
            // Detect a shift release
            shiftPressed = false;
        }
        keysDown[scanCode ^ EVENT_KEYUP] = false;
        return;
    }

    if(scanCode == LSHIFT || scanCode == RSHIFT){
        shiftPressed = true;
    }
    
    if(keyASCII[scanCode] != 0){
        if(shiftPressed){
            keysDown[scanCode] = true;
            lastKeyPressed = ASCIIUpper[scanCode];
        }else{
            keysDown[scanCode] = true;
            lastKeyPressed = keyASCII[scanCode];
        }
    }else{
        keysDown[scanCode] = true;
    }


    outb(PIC_EOI, PIC_EOI);
}

// Find if a key was pressed or not
bool IsKeyPressed(uint8 scanCode){
    return keysDown[scanCode];
}

// Get the last key pressed
uint8 GetKey(){
    uint8 key = lastKeyPressed;
    lastKeyPressed = 0;
    return key;
}

void InitializeKeyboard(){
    while (inb(KBD_STATUS_PORT) & 0x01) {
        // Flush the keyboard's buffer
        inb(KBD_DATA_PORT);
    }

    // Install the keyboard interrupt
    InstallISR(KEYBOARD_ISR, kb_handler);
    InstallIRQ(KEYBOARD_IRQ, kb_handler);
}