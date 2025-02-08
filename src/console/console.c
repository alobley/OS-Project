#include <console.h>
#include <stdbool.h>

/* TODO:
 * - Impmement an STDOUT, which can then be accessed by terminal applications? (Should the kernel do that instead?)
 * - Implement a way to have better text customization (more colors, better way to change colors)
*/

int16_t cursor_x = 0;
int16_t cursor_y = 0;

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

// Scroll the screen up one line
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
void ClearScreen(void){
    for(uint16_t i = 0; i < VGA_BYTES; i++){
        // Change this out for SIMD instructions?
        *(uint16_t*)(VGA_ADDRESS + (i * 2)) = (VGA_WHITE_ON_BLACK << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

HOT void WriteChar(char c){
    if(c == '\n'){
        cursor_x = 0;
        cursor_y++;
    } else {
        *(uint16_t*)(VGA_ADDRESS + ((cursor_x + (cursor_y * VGA_WIDTH)) * 2)) = (VGA_WHITE_ON_BLACK << 8) | c;
        cursor_x++;
    }

    if(cursor_x >= VGA_WIDTH){
        cursor_x = 0;
        cursor_y++;
    }

    if(cursor_y >= VGA_HEIGHT){
        cursor_y = 0;
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
        printf("Error: Base must be 16 or less. Value was greater than 16.\n");
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

HOT void printf(const char* fmt, ...){
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