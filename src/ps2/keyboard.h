#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <util.h>
#include <system.h>

// The keyboard ports
#define PS2_READ_PORT 0x64      // Status Register
#define PS2_WRITE_PORT 0x64     // Command Register
#define PS2_DATA_PORT 0x60      // Data Port

#define EVENT_KEYUP 0x80

// PS/2 controller command bytes
#define READ_BYTE 0x20          // This reads byte 0 from internal RAM. 0x21 to 0x3F read the other bytes. Refer to documentation for instructions.
#define WRITE_BYTE 0x60         // This writes a byte to internal RAM. 0x61 to 0x7F write the other bytes. Refer to documentation for instructions.
#define DISABLE_MOUSE 0xA7      // Disables the PS/2 mouse (Second PS/2 port)
#define ENABLE_MOUSE 0xA8       // Enables the PS/2 mouse
#define TEST_MOUSE 0xA9         // Tests the PS/2 mouse
#define TEST_CONTROLLER 0xAA    // Tests the PS/2 controller
#define TEST_KEYBOARD 0xAB      // Tests the PS/2 keyboard
#define DIAGNOSTIC_DUMP 0xAC    // Dumps the diagnostic data
#define DISABLE_KEYBOARD 0xAD   // Disables the PS/2 keyboard
#define ENABLE_KEYBOARD 0xAE    // Enables the PS/2 keyboard
#define READ_INPUT_PORT 0xC0    // Reads the controller input port

#define IDK1 0xC1               // Not sure what to call this one. Copies bits 0-3 of the input port to bits 4-7 of the status port
#define IDK2 0xC2               // Not sure what to call this one. Copies bits 4-7 of the input port to bits 4-7 of the status port

#define READ_OUTPUT_PORT 0xD0   // Reads the controller output port
#define WRITE_OUTPUT_PORT 0xD1  // Writes the controller output port
#define WRITE_KBD_OUTPUT 0xD2   // Writes the keyboard output buffer
#define WRITE_MOUSE_OUTPUT 0xD3 // Writes the mouse output buffer
#define WRITE_TO_MOUSE 0xD4     // Writes to the mouse

#define RESET 0xFF              // Resets the controller or a device

// 0xF0 to 0xFF are used for pulse output lines, irrelevant.

// Keyboard scancodes for a 104-key PS/2 keyboard
enum Keys {
    NONE = 0x0,
    ESC = 0x01,
    K1 = 0x02,
    K2 = 0x03,
    K3 = 0x04,
    K5 = 0x05,
    K6 = 0x06,
    K7 = 0x07,
    K8 = 0x09,
    K9 = 0x0A,
    K0 = 0x0B,
    MINUS = 0x0C,
    PLUS = 0x0D,
    BACKSPACE = 0x0E,
    TAB = 0x0F,
    Q = 0x10,
    W = 0x11,
    E = 0x12,
    R = 0x13,
    T = 0x14,
    Y = 0x15,
    U = 0x16,
    I = 0x17,
    O = 0x18,
    P = 0x19,
    BRACKET_OPEN = 0x1A,
    BRACKET_CLOSE = 0x1B,
    ENTER = 0x1C,
    LCTRL = 0x1D,
    A = 0x1E,
    S = 0x1F,
    D = 0x20,
    F = 0x21,
    G = 0x22,
    H = 0x23,
    J = 0x24,
    K = 0x25,
    L = 0x26,
    SEMICOLON = 0x27,
    APOSTROPHE = 0x28,
    TILDE = 0x29,
    LSHIFT = 0x2A,
    BACKSLASH = 0x2B,
    Z = 0x2C,
    X = 0x2D,
    C = 0x2E,
    V = 0x2F,
    B = 0x30,
    N = 0x31,
    M = 0x32,
    COMMA = 0x33,
    PERIOD = 0x34,
    FORWARDSLASH = 0x35,
    RSHIFT = 0x36,
    KPDSTAR = 0x37,
    LALT = 0x38,
    SPACE = 0x39,
    CAPSLOCK = 0x3A,
    F1 = 0x3B,
    F2 = 0x3C,
    F3 = 0x3D,
    F4 = 0x3E,
    F5 = 0x3F,
    F6 = 0x40,
    F7 = 0x41,
    F8 = 0x42,
    F9 = 0x43,
    F10 = 0x44,
    NUMLOCK = 0x45,
    SCROLLOCK = 0x46,
    HOME = 0x47,
    UP = 0x48,
    PGUP = 0x49,
    KPDMINUS = 0x4A,
    LEFT = 0x4B,
    KPDFIVE = 0x4C,
    RIGHT = 0x4D,
    KPDPLUS = 0x4E,
    END = 0x4F,
    DOWN = 0x50,
    PGDOWN = 0x51,
    INSERT = 0x52,
    DELETE = 0x53,
    SYSRQ = 0x54,
    PAUSE = 0x55,
    WINKEY = 0x56,
    F11 = 0x57,
    F12 = 0x58
};

typedef struct KeyboardEvent {
    unsigned char scanCode;
    char ascii;
    _Bool keyUp;
} KeyboardEvent_t;
typedef void (*KeyboardCallback)(KeyboardEvent_t event);

// Struct for the OS to determine the PS/2 hardware
typedef struct PS2Info {
    bool mouseExists;
    bool keyboardExists;
    uint8_t mouseID;
    uint8_t keyboardID;
    uint8_t scanCodeSet;
} PS2Info;

int InitializeKeyboard();

bool MouseExists();

void PS2Reboot();

void InstallKeyboardCallback(KeyboardCallback callback);

#endif