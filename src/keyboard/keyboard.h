#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <types.h>

// The keyboard ports
#define KBD_STATUS_PORT 0x64
#define KBD_DATA_PORT 0x60

#define EVENT_KEYUP 0x80

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

static char keyASCII[104] = {
    0,
    0,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 
    0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, '5', 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char ASCIIUpper[104] = {
    0,
    0,
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Get the last key pressed
uint8 GetKey();

void WaitForKeyPress();

bool IsKeyPressed(uint8 scanCode);

void InitializeKeyboard();

#endif