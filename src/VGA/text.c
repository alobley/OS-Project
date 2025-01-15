#include "text.h"
#include <memmanage.h>
#include <io.h>
#include <util.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>



void MoveCursor(uint16 x, uint16 y){
    uint16 offset = x + (y * VGA_TEXT_MODE_WIDTH);
    outb(VGA_CRTC_INDEX, 0x0E);

    outb(VGA_CRTC_DATA, (uint8)((offset >> 8) & 0xFF));

    outb(VGA_CRTC_INDEX, 0x0F);

    outb(VGA_CRTC_DATA, (uint8)(offset & 0xFF));

    *(uint16 *)((VGA_TEXT_MODE_START + (offset * 2)) + 1) = 0x0F;
}

int16 currentX = 0;
int16 currentY = 0;

void ClearTerminal(){
    for(int i = 0; i < (80 * 25)*2; i++){
        *((uint16* )(VGA_TEXT_MODE_START + i)) = 0;
    }
    currentX = 0;
    currentY = 0;
}

void NewLine(){
    currentY++;
    if(currentX + (currentY * 80) > 2000){
        ClearTerminal();
    }
    currentX = 0;
}

void WriteStr(const char* str){
    uint16* fb = (uint16* )VGA_TEXT_MODE_START;
    int i = 0;

    while(str[i] != '\0'){
        if(str[i] == '\n'){
            NewLine();
            i++;
        }else if(str[i] == '\b'){
            currentX -= 1;
            if(currentX < 0){
                currentX = 0;
            }
            fb[currentX + (currentY * 80)] = (0x0F << 8) | ' ';
            if(currentX + (currentY * 80) > 2000){
                ClearTerminal();
                WriteStr(str);
            }
            i++;
        }else{
            fb[currentX + (currentY * 80)] = (0x0F << 8) | str[i];
            i++;
            currentX += 1;
            if(currentX > 80){
                currentX = 0;
                currentY += 1;
            }
            if(currentX + (currentY * 80) > 2000){
                ClearTerminal();
                WriteStr(str);
            }
        }
    }

    MoveCursor(currentX, currentY);
}

void WriteStrSize(const char* str, size_t size){
    uint16* fb = (uint16* )VGA_TEXT_MODE_START;
    size_t i = 0;

    while(i != size){
        if(str[i] == '\n'){
            NewLine();
            i++;
        }else if(str[i] == '\b'){
            currentX -= 1;
            if(currentX < 0){
                currentX = 0;
            }
            fb[currentX + (currentY * 80)] = (0x0F << 8) | ' ';
            if(currentX + (currentY * 80) > 2000){
                ClearTerminal();
                WriteStr(str);
            }
            i++;
        }else{
            fb[currentX + (currentY * 80)] = (0x0F << 8) | str[i];
            i++;
            currentX += 1;
            if(currentX > 80){
                currentX = 0;
                currentY += 1;
            }
            if(currentX + (currentY * 80) > 2000){
                ClearTerminal();
                WriteStrSize(str, size);
            }
        }
    }

    MoveCursor(currentX, currentY);
}

void reverse(char* str, int length){
    int start = 0;
    int end = length - 1;

    while(start < end){
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* ConvertSigned(int64 number){
    // Allocate the maximum digit size (including negative sign and null terminator)
    char* buffer = (char* )alloc(22);
    memset(buffer, 0, 22);
    int i = 0;
    bool isNegative = false;

    if(number == 0){
        buffer[i] = '0';
        buffer[i + 1] = '\0';
        return buffer;
    }

    if(number < 0){
        number = -number;
        isNegative = true;
    }

    while(number != 0){
        int rem = number % 10;
        buffer[i++] = rem + '0';
        number /= 10;
    }

    if(isNegative){
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    reverse(buffer, i);

    return buffer;
}

char* ConvertUnSigned(uint64 number, uint8 base){
    // Allocate the maximum digit size (including negative sign and null terminator)
    char* buffer = (char* )alloc(22);
    memset(buffer, 0, 22);
    int i = 0;

    if(number == 0){
        buffer[i] = '0';
        buffer[i + 1] = '\0';
        return buffer;
    }

    while(number != 0){
        int rem = number % base;
        buffer[i] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        number = number / base;
        i++;
    }
    buffer[i] = '\0';

    reverse(buffer, i);

    return buffer;
}

int16 GetX(){
    return currentX;
}

void printk(const char *str, ...){
    if(*str == '\0'){
        return;
    }
    
    va_list args;
    va_start(args, str);

    int index = 0;

    // Allocate 2KB of the kernel's heap to the output buffer, which is the total size of VGA text mode. Strings can not be larger than this.
    char* output = (char* )alloc(2000);

    int i = 0;

    while(str[i] != '\0'){
        if(str[i] == '%'){
            i++;
            if(str[i] == 'd'){
                int32 argVal = va_arg(args, int32);
                char* strnum = ConvertSigned((int32)argVal);
                int len = strlen(strnum);
                for(int i = 0; i < len; i++){
                    *(output + index + i) = *(strnum + i);
                }
                dealloc(strnum);
                index += len;
            }else if(str[i] == 'u'){
                uint32 argVal = va_arg(args, uint32);
                char* strnum = ConvertUnSigned(argVal, 10);
                int len = strlen(strnum);
                for(int i = 0; i < len; i++){
                    *(output + index + i) = *(strnum + i);
                }
                dealloc(strnum);
                index += len;
            }else if(str[i] == 'x'){
                uint32 argVal = va_arg(args, uint32);
                char* strnum = ConvertUnSigned(argVal, 16);
                int len = strlen(strnum);
                for(int i = 0; i < len; i++){
                    *(output + index + i) = *(strnum + i);
                }
                dealloc(strnum);
                index += len;
            }else if(str[i] == 's'){
                const char* argStr = va_arg(args, const char*);
                int len = strlen(argStr);
                for(int j = 0; j < len; j++){
                    *(output + index + j) = argStr[j];
                }
            }else if(str[i] == 'c'){
                int chr = va_arg(args, int);
                *(output + index) = (char)chr;
            }else if(str[i] == 'l' && str[i + 1] == 'l' && str[i+2] == 'u'){
                uint32 argVal = va_arg(args, uint32);
                char* strnum = ConvertUnSigned(argVal, 10);
                int len = strlen(strnum);
                for(int i = 0; i < len; i++){
                    *(output + index + i) = *(strnum + i);
                }
                i += 2;
                dealloc(strnum);
                index += len;
            }else{
                *(output + index) = '%';
                index++;
            }
            i++;
        }
        *(output + index) = str[i];
        i++;
        index++;
    }

    // Add a null terminator
    *(output + index) = '\0';

    WriteStr(output);

    dealloc(output);

    va_end(args);
}