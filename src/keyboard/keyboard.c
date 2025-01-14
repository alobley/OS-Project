#include "keyboard.h"
#include "io.h"
#include <isr.h>
#include <idt.h>
#include <irq.h>
#include <vga.h>
#include <time.h>
#include <util.h>
#include <acpi.h>

#define KEYBOARD_ISR 0x21
#define KEYBOARD_IRQ 1

// The bits of the keyboard status register when read
#define OutputStatus(value) (value << 0)
#define InputStatus(value) (value << 1)
#define StatusSystemFlag(value) (value << 2)
#define CommandData(value) (value << 3)
// Two unknown bits
#define TimeoutError(value) (value << 6)
#define ParityError(value) (value << 7)

// The bits of the keyboard controller configuration byte (commands 0x20 and 0x60)
#define FirstPortInterrupt(value) (value << 0)
#define SecondPortInterrupt(value) (value << 1)
#define ControllerSystemFlag(value) (value << 2)
#define Zero(value) (value << 3)
#define FirstPortClock(value) (value << 4)
#define SecondPortClock(value) (value << 5)
#define Translation(value) (value << 6)
#define Zero2(value) (value << 7)

// The bits of the PS/2 controller output port (commands 0xD0 and 0xD1). Can be both read and written.
#define SystemReset(value) (value << 0)
#define A20Gate(value) (value << 1)
#define SecondPortClock(value) (value << 2)
#define SecondPortData(value) (value << 3)
#define FirstPortOutputBufferFull(value) (value << 4)
#define SecondPortOutputBufferFull(value) (value<< 5)
#define FirstPortClock(value) (value << 6)
#define FirstPortData(value) (value << 7)

// Keyboard command bytes
#define SET_LEDS 0xED
#define ECHO 0xEE
#define SCAN_CODE_SET 0xF0      // This lets you get/set the scan code set. Bit 0 = get, bit 1 = set 1, bit 2 = set 2, bit 3 = set 3
#define IDENTIFY 0xF2
#define SET_TYPEMATIC 0xF3
#define ENABLE 0xF4
#define DISABLE 0xF5
#define DEFAULTS 0xF6           // Set default keyboard parameters
#define RESEND 0xFE
#define RESET 0xFF

static bool keysDown[256];
static uint8 lastKeyPressed = 0;

// Function to wait until the input buffer is empty
void wait_for_input_buffer() {
    while (inb(PS2_READ_PORT) & 0x02);
}

// Function to wait until the output buffer is full
void wait_for_output_buffer() {
    while (!(inb(PS2_READ_PORT) & 0x01));
}

void WaitForKeyPress(){
    uint8 currentKey = 0;
    while(currentKey == 0){
        currentKey = GetKey();
    }
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void WaitForRelease(uint8 ScanCode){
    while(IsKeyPressed(ScanCode));      // Busy wait so that the CPU isn't halted. Important for multitasking.
}
#pragma GCC pop_options

bool shiftPressed = false;

#pragma GCC push_options
#pragma GCC optimize("O0")
void kb_handler(){
    uint8 scanCode;
    // If there is anything to retrieve, retrieve it and inform the PIC that the interrupt has been handled.
    scanCode = inb(PS2_DATA_PORT);
    outb(PIC_EOI, PIC_EOI);


    // At some point, replace the following code with calling keyboard handlers from applications (just make sure they return to user mode first).
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
        // When shift is pressed
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
}
#pragma GCC pop_options

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

bool mouseExists = false;

bool MouseExists(){
    return mouseExists;
}


PS2Info ps2Info;

void InitializeKeyboard(){
    if(!PS2ControllerExists()){
        printk("PS/2 Controller not found. This computer can't be used.\n");
        return;
    }
    // If it doesn't, return. These kinds of systems will not be supported until USB is implemented.

    // USB translation will need to be disabled eventually. No USB drivers for now.

    // Disable PS/2 devices
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, DISABLE_KEYBOARD);
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, DISABLE_MOUSE);        // This will be ignored in single-port devices.

    while (inb(PS2_READ_PORT) & 0x01) {
        // Flush the output buffer
        inb(PS2_DATA_PORT);
    }

    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, READ_BYTE);

    wait_for_output_buffer();
    uint8 config = inb(PS2_DATA_PORT);

    // Disable the first port interrupt, translation, and clock. Apparently clock = 0 enables it.
    config = config & (FirstPortInterrupt(0) || Translation(0) || FirstPortClock(0));

    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, WRITE_BYTE);
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, config);

    // Perform self-test
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, TEST_CONTROLLER);
    wait_for_output_buffer();
    while (inb(PS2_READ_PORT) & 0x01 == 0);

    wait_for_output_buffer();
    if(inb(PS2_DATA_PORT) != 0x55){
        // Self-test failed, something should be done to inform the kernel.
        printk("PS/2 Controller self-test failed.\n");
        return;
    }

    // Just in case the controller was reset, set the configuration byte again
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, WRITE_BYTE);
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, config);

    // Determine if there are two channels
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, ENABLE_MOUSE);
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, READ_BYTE);

    wait_for_output_buffer();
    config = inb(PS2_DATA_PORT);
    if(config & SecondPortOutputBufferFull(1) == 0){
        // There are two channels
        wait_for_input_buffer();
        outb(PS2_WRITE_PORT, DISABLE_MOUSE);
        config = config & (SecondPortInterrupt(0) || SecondPortClock(0));
        wait_for_input_buffer();
        outb(PS2_WRITE_PORT, WRITE_BYTE);
        wait_for_input_buffer();
        outb(PS2_DATA_PORT, config);
        mouseExists = true;
    }

    // Test the keyboard
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, TEST_KEYBOARD);
    wait_for_output_buffer();
    while (inb(PS2_READ_PORT) & 0x01 == 0);
    wait_for_output_buffer();
    uint8 result = inb(PS2_DATA_PORT);
    //printk("0x%x\n", result);

    if(result != 0x55 && result != 0x00){
        // Self-test failed, something should be done to inform the kernel.
        printk("PS/2 keyboard self-test failed.\n");
        return;
    }

    if(mouseExists){
        // Test the mouse
        wait_for_input_buffer();
        outb(PS2_WRITE_PORT, TEST_MOUSE);
        wait_for_output_buffer();
        if(inb(PS2_DATA_PORT) != 0x55){
            // Self-test failed, something should be done to inform the kernel.
            printk("PS/2 mouse self-test failed.\n");
            return;
        }
    }

    // Enable the keyboard and mouse (if it exists)
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, ENABLE_KEYBOARD);
    config = config | FirstPortInterrupt(1);
    if(mouseExists){
        wait_for_input_buffer();
        outb(PS2_WRITE_PORT, ENABLE_MOUSE);
        config = config | SecondPortInterrupt(1);
    }
    wait_for_input_buffer();
    outb(PS2_WRITE_PORT, WRITE_BYTE);
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, config);

    // Reset mouse and keyboard
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, RESET);

    wait_for_output_buffer();
    uint8 result1 = inb(PS2_DATA_PORT);

    wait_for_output_buffer();
    uint8 result2 = inb(PS2_DATA_PORT);

    if(result1 == 0xFC || result1 == 0){
        // Reset failed
        printk("PS/2 keyboard reset failed.\n");
        return;
    }else{
        // The reset was successful, the next bytes should be the device types
        uint8 id1 = inb(PS2_DATA_PORT);
        uint8 id2 = inb(PS2_DATA_PORT);

        // Determine which keyboard type it is...
    }

    if(mouseExists){
        wait_for_input_buffer();
        outb(PS2_WRITE_PORT, WRITE_TO_MOUSE);
        wait_for_input_buffer();
        outb(PS2_DATA_PORT, RESET);

        wait_for_output_buffer();
        result1 = inb(PS2_DATA_PORT);
        wait_for_output_buffer();
        result2 = inb(PS2_DATA_PORT);
        if(result1 == 0xFC || result1 == 0){
            // Reset failed
            printk("PS/2 keyboard reset failed.\n");
            return;
        }else{
            // The reset was successful, the next bytes should be the device types
            uint8 id1 = inb(PS2_DATA_PORT);
            uint8 id2 = inb(PS2_DATA_PORT);

            // Determine which mouse type it is...
        }
    }

    // Get the default scan code set
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, SCAN_CODE_SET);
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, 0x00);     // Get the scan code set
    wait_for_output_buffer();
    if(inb(PS2_DATA_PORT) == 0xFA){
        wait_for_output_buffer();
        uint8 scanCodeSet = inb(PS2_DATA_PORT);
    }

    // Set the scan code set
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, SCAN_CODE_SET);
    wait_for_input_buffer();
    outb(PS2_DATA_PORT, 0x01);     // Set to scan code set 1 (because that's what the driver supports)
    wait_for_output_buffer();
    uint8 ack = inb(PS2_DATA_PORT);
    if (ack != 0xFA) {
        printk("Failed to set scan code set.\n");
        return;
    }

    // Install the keyboard interrupt
    InstallISR(KEYBOARD_ISR, kb_handler);
    InstallIRQ(KEYBOARD_IRQ, kb_handler);
}