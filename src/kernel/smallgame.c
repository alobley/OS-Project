#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <vga.h>
#include <keyboard.h>
#include <memmanage.h>
#include <time.h>
#include <fpu.h>
#include <pcspkr.h>

square_t* square;

bool beepOn = false;

void timer(){
    // Movement detection
    if(IsKeyPressed(W)){
        square->y -= 5;
    }
    if(IsKeyPressed(S)){
        square->y += 5;
    }
    if(IsKeyPressed(A)){
        square->x -= 5;
    }
    if(IsKeyPressed(D)){
        square->x += 5;
    }

    // Clear the screen for the next draw event
    VGA_ClearScreen();

    // If the square is drawn after the screen clears and there aren't many interruptions in drawing, screen tearing should be minimal.
    VGA_DrawSquare(square);
}

// The timer callback for playing sound - just a beep every 0.1 seconds
void SoundTimer(){
    if(!beepOn){
        PCSP_BeepOn();
        beepOn = true;
    }else{
        beepOn = false;
        PCSP_NoSound();
    }
}

// The main function for the "little game," although the game is minimal.
void LittleGame(){
    // Set VGA graphics (320x200x256 pixel mode) and set the color to 256-color rrrgggbb
    VGA_SetMode(VGA_MODE_GRAPHICS);
    VGA_SetColorRGB8();

    // Create a square that will be controlled by the player
    square = VGA_CreateSquare(10, 10, 10, 10, COLOR_WHITE);

    VGA_DrawSquare(square);

    // Add the timers for the game to the list of timers
    AddTimerCallback(timer, 17);         // The game's timer processes the game's logic ~60 times per second, making the speed manageable
    AddTimerCallback(SoundTimer, 100);   // The speaker's timer sets it to beep for 100ms then be quiet for 100ms

    // Play until the escape key is pressed
    while(!IsKeyPressed(ESC));

    // Be responsible with graphics and timers!
    RemoveTimerCallback(0);
    RemoveTimerCallback(1);
    PCSP_NoSound();
    VGA_SetMode(VGA_MODE_TEXT);
}