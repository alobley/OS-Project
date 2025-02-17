#ifndef TIME_H
#define TIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <interrupts.h>

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

#define PIT_MASK 0xFF

#define PIT_SET 0x36

#define PIT_HZ 1193182

#define PIT_IRQ 0

typedef struct Date_Time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

extern datetime_t currentTime;

typedef void (*timer_callback_t)(void);

void AddTimerCallback(timer_callback_t callback, uint64_t interval);
void RemoveTimerCallback(timer_callback_t* callback);
void InitTimer();
void SetTimer(int hz);
void sleep(uint64_t ms);

void SetTime();

#endif