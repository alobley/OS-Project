#include <console.h>
#include <stdbool.h>

/* TODO:
 * - Impmement an STDOUT, which can then be accessed by terminal applications? (Should the kernel do that instead?)
 * - Implement a way to have better text customization (more colors, better way to change colors)
 * - Improve the VGA driver to support more features (e.g. mode X)
 * - Allow console history and scrolling up/down through it
 * 
 * NOTE: The VGA driver will always be hardcoded into the kernel
*/

volatile uintptr_t VGA_ADDRESS = 0xB8000;

int16_t cursor_x = 0;
int16_t cursor_y = 0;

void RemapVGA(uintptr_t addr){
    VGA_ADDRESS = addr + (0xB8000 - 0xA0000);
}

uintptr_t GetVGAAddress(void){
    return VGA_ADDRESS;
}

// Move the VGA cursor's position on the screen
HOT void MoveCursor(uint16_t x, uint16_t y){
    uint16_t offset = x + (y * VGA_WIDTH);
    outb(VGA_CRTC_INDEX, 0x0E);

    outb(VGA_CRTC_DATA, (uint8_t) (offset >> 8) & 0xFF);

    outb(VGA_CRTC_INDEX, 0x0F);

    outb(VGA_CRTC_DATA, (uint8_t) offset & 0xFF);

    // Make sure the cursor is visible
    *(uint16_t*)((VGA_ADDRESS + (offset * 2)) + 1) = VGA_WHITE_ON_BLACK;
}

// Scroll the screen up one line (software-based implementation)
// Implement hardware-based with mode X?
HOT void Scroll(void){
    for(uint16_t i = 0; i < VGA_SIZE - VGA_WIDTH; i++){
        *(uint16_t*)(VGA_ADDRESS + (i * 2)) = *(uint16_t*)(VGA_ADDRESS + ((i + VGA_WIDTH) * 2));
    }

    for(uint16_t i = VGA_SIZE - VGA_WIDTH; i < VGA_SIZE; i++){
        *(uint16_t*)(VGA_ADDRESS + (i * 2)) = (VGA_WHITE_ON_BLACK << 8) | ' ';
    }

    cursor_y--;
    MoveCursor(cursor_x, cursor_y);
}

// Clear the screen
HOT void ClearScreen(void){
    for(uint16_t i = 0; i < VGA_BYTES; i++){
        // Change this out for SIMD instructions?
        *(uint16_t*)(VGA_ADDRESS + (i * 2)) = (VGA_WHITE_ON_BLACK << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

HOT inline void WriteChar(char c){
    if(c == '\n'){
        cursor_x = 0;
        cursor_y++;
    } else if(c == '\b'){
        if(cursor_x > 0){
            cursor_x--;
            *(uint16_t*)(VGA_ADDRESS + ((cursor_x + (cursor_y * VGA_WIDTH)) * 2)) = (VGA_WHITE_ON_BLACK << 8) | ' ';
        }
    } else {
        *(uint16_t*)(VGA_ADDRESS + ((cursor_x + (cursor_y * VGA_WIDTH)) * 2)) = (VGA_WHITE_ON_BLACK << 8) | c;
        cursor_x++;
    }

    if(cursor_x >= VGA_WIDTH){
        cursor_x = 0;
        cursor_y++;
    }

    if(cursor_y >= VGA_HEIGHT){
        Scroll();
    }

    MoveCursor(cursor_x, cursor_y);
}

HOT void WriteString(const char* str){
    while(*str){
        WriteChar(*str++);
    }
}

HOT void WriteStringSize(const char* str, size_t size){
    for(size_t i = 0; i < size; i++){
        WriteChar(str[i]);
    }
}

HOT void PrintNum(uint64_t num, uint8_t base, bool s){
    static char* digits = "0123456789ABCDEF";
    char buffer[65];                                // Allow support for a 64-bit binary number with a null terminator
    size_t i = 0;

    if(base > 16){
        printk("Error: Base must be 16 or less. Value was greater than 16.\n");
    }

    // Check if the number is negative
    if(s && base == 10 && (int64_t) num < 0){
        if((int64_t) num < 0){
            num = -num;
            WriteChar('-');
        }
    }

    // Convert the number to a string
    do {
        buffer[i++] = digits[num % base];
        num /= base;
    } while(num > 0);

    // Null-terminate the string
    buffer[i] = '\0';

    // Reverse the buffer
    for(size_t j = 0; j < i / 2; j++){
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    WriteString(buffer);
}

HOT void printkloat(double num, int precision){
    char buffer[65];                    // Maximum of 64 characters for a double

    // Check if the number is negative
    if(num < 0){
        num = -num;
        WriteChar('-');
    }

    uint64_t whole = (uint64_t)num;
    double fraction = num - whole;

    int i = 0;
    if(whole == 0){
        buffer[i++] = '0';
    } else {
        while(whole > 0){
            buffer[i++] = (whole % 10) + '0';
            whole /= 10;
        }
    }

    // Reverse the integer string
    for(size_t j = 0; j < (size_t)i / 2; j++){
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    // Print the integer part
    WriteString(buffer);
    WriteChar('.');

    if(precision > 0){
        for(int k = 0; k < precision; k++){
            fraction *= 10;
            WriteChar((int)fraction + '0');
            fraction -= (int)fraction;
        }
    }else{
        WriteChar('0');
    }
}

HOT void printk(const char* fmt, ...){
    char c[2] = {'\0', '\0'};
    va_list args;
    va_start(args, fmt);

    for(size_t i = 0; fmt[i] != 0; i++){
        if(fmt[i] == '%'){
            i++;
            switch(fmt[i]){
                case 'c': {
                    c[0] = va_arg(args, int);
                    WriteString(c);
                    break;
                }
                case 's': {
                    WriteString(va_arg(args, const char*));
                    break;
                }
                case 'd': {
                    int x = va_arg(args, int);
                    PrintNum((int64_t)x, 10, true);
                    break;
                }
                case 'x': {
                    uint32_t x = va_arg(args, uint32_t);
                    PrintNum((uint64_t)x, 16, false);
                    break;
                }
                case 'b': {
                    uint32_t x = va_arg(args, uint32_t);
                    PrintNum((uint64_t)x, 2, false);
                    break;
                }
                case 'o': {
                    uint32_t x = va_arg(args, uint32_t);
                    PrintNum((uint64_t)x, 8, false);
                    break;
                }
                case 'u': {
                    uint32_t x = va_arg(args, uint32_t);
                    PrintNum((uint64_t)x, 10, false);
                    break;
                }
                case 'f': {
                    double x = va_arg(args, double);
                    printkloat(x, 6);
                    break;
                }
                case 'l': {
                    extra_l:
                    i++;
                    switch(fmt[i]){
                        case 'd':
                            PrintNum(va_arg(args, int64_t), 10, true);
                            break;
                        case 'x':
                            PrintNum(va_arg(args, int64_t), 16, false);
                            break;
                        case 'b':
                            PrintNum(va_arg(args, int64_t), 2, false);
                            break;
                        case 'o':
                            PrintNum(va_arg(args, int64_t), 8, false);
                            break;
                        case 'u':
                            PrintNum(va_arg(args, uint64_t), 10, false);
                            break;
                        case 'l':
                            // Yes I know this isn't best practice, but it's simple and it works very well without recursion
                            goto extra_l;
                        case 'f':
                            printkloat(va_arg(args, double), 6);
                            break;
                        default:
                            WriteChar('%');
                            WriteChar('l');
                            WriteChar(fmt[i]);
                            break;
                    }
                    break;
                }
                default: {
                    WriteString("Unknown format specifier: ");
                    WriteChar(fmt[i]);
                    break;
                }
            }
        } else {
            WriteChar(fmt[i]);
        }
    }
}