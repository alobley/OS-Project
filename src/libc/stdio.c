#include "stdio.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <system.h>
#include <stdbool.h>

void printfloat(double num, int precision){
    char buffer[65];                    // Maximum of 64 characters for a double

    // Check if the number is negative
    if(num < 0){
        num = -num;
        write(STDOUT_FILENO, "-", 1);
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
    write(STDOUT_FILENO, buffer, i);
    write(STDOUT_FILENO, ".", 1);

    if(precision > 0){
        for(int k = 0; k < precision; k++){
            fraction *= 10;
            char c = (int)fraction + '0';
            write(STDOUT_FILENO, &c, 1);
            fraction -= (int)fraction;
        }
    }else{
        write(STDOUT_FILENO, "0", 1);
    }
}

void printnum(uint64_t num, uint8_t base, bool s){
    static char* digits = "0123456789ABCDEF";
    char buffer[65];                                // Allow support for a 64-bit binary number with a null terminator
    size_t i = 0;

    if(base > 16){
        write(STDOUT_FILENO, "Error: Base must be 16 or less. Value was greater than 16.\n", 59);
        return;
    }

    // Check if the number is negative
    if(s && base == 10 && (int64_t) num < 0){
        if((int64_t) num < 0){
            num = -num;
            write(STDOUT_FILENO, "-", 1);
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

    write(STDOUT_FILENO, buffer, i);
}

void printf(const char* format, ...){
    char c[2] = {'\0', '\0'};
    va_list args;
    va_start(args, format);

    for(size_t i = 0; format[i] != 0; i++){
        if(format[i] == '%'){
            i++;
            switch(format[i]){
                case 'c': {
                    c[0] = va_arg(args, int);
                    write(STDOUT_FILENO, c, 1);
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    write(STDOUT_FILENO, str, strlen(str));
                    break;
                }
                case 'd': {
                    int x = va_arg(args, int);
                    printnum((int64_t)x, 10, true);
                    break;
                }
                case 'x': {
                    uint32_t x = va_arg(args, uint32_t);
                    printnum((uint64_t)x, 16, false);
                    break;
                }
                case 'b': {
                    uint32_t x = va_arg(args, uint32_t);
                    printnum((uint64_t)x, 2, false);
                    break;
                }
                case 'o': {
                    uint32_t x = va_arg(args, uint32_t);
                    printnum((uint64_t)x, 8, false);
                    break;
                }
                case 'u': {
                    uint32_t x = va_arg(args, uint32_t);
                    printnum((uint64_t)x, 10, false);
                    break;
                }
                case 'f': {
                    double x = va_arg(args, double);
                    printfloat(x, 6);
                    break;
                }
                case 'l': {
                    extra_l:
                    i++;
                    switch(format[i]){
                        case 'd':
                            printnum(va_arg(args, int64_t), 10, true);
                            break;
                        case 'x':
                            printnum(va_arg(args, int64_t), 16, false);
                            break;
                        case 'b':
                            printnum(va_arg(args, int64_t), 2, false);
                            break;
                        case 'o':
                            printnum(va_arg(args, int64_t), 8, false);
                            break;
                        case 'u':
                            printnum(va_arg(args, uint64_t), 10, false);
                            break;
                        case 'l':
                            // Yes I know this isn't best practice, but it's simple and it works very well without recursion
                            goto extra_l;
                        case 'f':
                            printfloat(va_arg(args, double), 6);
                            break;
                        default:
                            write(STDOUT_FILENO, "%", 1);
                            write(STDOUT_FILENO, "l", 1);
                            write(STDOUT_FILENO, &format[i], 1);
                            break;
                    }
                    break;
                }
                default: {
                    write(STDOUT_FILENO, "Unknown format specifier: ", 26);
                    write(STDOUT_FILENO, &format[i], 1);
                    break;
                }
            }
        } else {
            write(STDOUT_FILENO, &format[i], 1);
        }
    }
}