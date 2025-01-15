#include "pixel.h"
#include <memmanage.h>

void VGA_ClearScreen(){
    uint8* fb = (uint8*)VGA_PIXEL_MODE_START;
    for(int i = 0; i < VGA_PIXEL_MODE_SIZE; i++){
        *(fb + i) = 0;
    }
}

void VGA_DrawPixel(int x, int y, color c){
    uint8* fb = (uint8*)VGA_PIXEL_MODE_START;
    *(fb + x + (y * VGA_PIXEL_MODE_WIDTH)) = c;
}

void VGA_DrawSquare(square_t* square){
    for(int i = 0; i < square->height; i++){
        for(int j = 0; j < square->width; j++){
            VGA_DrawPixel(square->x + j, square->y + i, square->c);
        }
    }
}

square_t* VGA_CreateSquare(uint16 x, uint16 y, uint16 width, uint16 height, color c){
    square_t* square = alloc(sizeof(square_t));
    square->height = height;
    square->width = width;
    square->c = c;
    square->x = x;
    square->y = y;

    return square;
}