#ifndef PCSPKR_H
#define PCSPKR_H

#include <stdint.h>
#include <stdbool.h>
#include <util.h>

void PCSP_NoSound();
void PCSP_Beep();
void PCSP_PlaySound(uint16_t frequency);
void PCSP_SpeakerInit();
void PCSP_BeepOn();

#endif