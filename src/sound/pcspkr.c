#include "pcspkr.h"
#include <time.h>

#define TIMER_SELECT 0x43
#define TIMER_DATA 0x42
#define TIMER2 0xB6
#define SPEAKER_PORT 0x61

static bool playing = false;

void PCSP_SpeakerInit(){
    outb(SPEAKER_PORT, inb(SPEAKER_PORT) | 3);
}

void PCSP_NoSound(){
    outb(SPEAKER_PORT, inb(SPEAKER_PORT) & 0xFC);
    playing = false;
}


void PCSP_PlaySound(uint16 d){
    outb(TIMER_SELECT, TIMER2);
    outb(TIMER_DATA, (uint8)(d & 0xFF));
    outb(TIMER_DATA, (uint8)((d >> 8) & 0xFF));

    if(!playing){
        playing = true;
        outb(SPEAKER_PORT, inb(SPEAKER_PORT) | 0x3);
    }
}

#define BEEP_TONE 4096

void PCSP_Beep(){
    PCSP_PlaySound(BEEP_TONE);
    delay(10);
    PCSP_NoSound();
}

void PCSP_BeepOn(){
    PCSP_PlaySound(BEEP_TONE);
}