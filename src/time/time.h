#ifndef TIME_H
#define TIME_H

#include <types.h>

// Milliseconds
#define TIMER_TPS 1000

// Define a type for timer functions that will be called
typedef void (*TimerCallback)(void);

uint64 GetTicks();
void InitializePIT();
void delay(uint64 ms);
void AddTimerCallback(TimerCallback callback, uint32 interval);
void RemoveTimerCallback(TimerCallback callback);

#endif