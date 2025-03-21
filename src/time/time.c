#include <time.h>
#include <alloc.h>
#include <console.h>
#include <cmos.h>
#include <acpi.h>

#define TIMER_TPS 1000

typedef struct TimerCallback {
    timer_callback_t callback;
    uint64_t interval;
    uint64_t elapsed;
    struct TimerCallback* next;
} TimerCallbackEntry;

TimerCallbackEntry* timerCallbacks = NULL;

static struct State {
    uint64_t frequency;
    uint64_t divisor;
    uint64_t ticks;
} state;

#define MAX_TIMERS 1024

uint16_t numTimers = 0;

void AddTimerCallback(timer_callback_t callback, uint64_t interval){
    if(numTimers >= MAX_TIMERS){
        // There are too many timers!
        printk("KERNEL PANIC: Too many timers!\n");
        // Log the error, kill the caller...
        return;
    }
    TimerCallbackEntry* entry = (TimerCallbackEntry*)halloc(sizeof(TimerCallbackEntry));
    if(entry == NULL){
        // Memory allocation failed
        printk("KERNEL PANIC: Memory allocation failed!\n");
        return;
    }

    // Add the latest callback to the start of the list (faster and easier than appending to the end)
    entry->callback = callback;
    entry->interval = interval;
    entry->elapsed = 0;
    entry->next = timerCallbacks;
    timerCallbacks = entry;
    numTimers++;
}

void RemoveTimerCallback(timer_callback_t callback){
    TimerCallbackEntry* entry = timerCallbacks;
    TimerCallbackEntry* prev = NULL;

    while(entry != NULL && entry->callback != *callback){
        prev = entry;
        entry = entry->next;
    }

    if(entry == NULL){
        // Timer not found
        return;
    }

    if(prev){
        prev->next = entry->next;
    } else {
        timerCallbacks = entry->next;
    }

    hfree(entry);
}

uint64_t GetTicks(){
    return state.ticks;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
// Busy-wait in order to delay (when multitasking is implemented, put the process to sleep instead, saves time and resources)
void busysleep(uint64_t ms){
    uint64_t currentTicks = GetTicks();
    uint64_t end = currentTicks + ms;
    while(currentTicks < end){
        currentTicks = GetTicks();
    }
}
#pragma GCC pop_options

void SetTimer(int hz){
    outb(PIT_COMMAND, PIT_SET);
    uint16_t divisor = PIT_HZ / hz;

    outb(PIT_CHANNEL0, divisor & PIT_MASK);
    outb(PIT_CHANNEL0, (divisor >> 8) & PIT_MASK);
}

static void TimerHandler(UNUSED struct Registers* regs){
    state.ticks++;
    volatile TimerCallbackEntry* entry = timerCallbacks;
    while(entry != NULL){
        entry->elapsed++;
        if(entry->elapsed >= entry->interval){
            entry->callback();
            entry->elapsed = 0;
        }
        entry = entry->next;
    }
    outb(PIC1, PIC_EOI);
}

void InitTimer(){
    state.frequency = PIT_HZ;
    state.divisor = state.frequency / TIMER_TPS;
    state.ticks = 0;

    SetTimer(TIMER_TPS);
    InstallIRQ(PIT_IRQ, TimerHandler);
}

// The system time
datetime_t currentTime = {0, 0, 0, 0, 0, 0};

void UpdateTime(){
    if(currentTime.second < 59){
        currentTime.second++;
    }else{
        currentTime.second = 0;
        if(currentTime.minute < 59){
            currentTime.minute++;
        }else{
            currentTime.minute = 0;
            if(currentTime.hour < 23){
                currentTime.hour++;
            }else{
                currentTime.hour = 0;
                if(currentTime.day < 31){
                    currentTime.day++;
                }else{
                    currentTime.day = 1;
                    if(currentTime.month < 12){
                        currentTime.month++;
                    }else{
                        currentTime.month = 1;
                        currentTime.year++;
                    }
                }
            }
        }
    }
}

// TODO: Improve year detection
void SetTime(){
    cli
    outb(CMOS_ADDRESS, 0x0A);
    while(inb(CMOS_DATA) & 0x80);

    outb(CMOS_ADDRESS, CMOS_RTC_SECONDS);
    currentTime.second = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, CMOS_RTC_MINUTES);
    currentTime.minute = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, CMOS_RTC_HOURS);
    currentTime.hour = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, CMOS_RTC_DAY);
    currentTime.day = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, CMOS_RTC_MONTH);
    currentTime.month = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, CMOS_RTC_YEAR);
    currentTime.year = inb(CMOS_DATA);
    uint8_t century = 0;
    if(acpiInfo.fadt->century != 0){
        outb(CMOS_ADDRESS, CMOS_MAYBE_CENTURY);
        century = inb(CMOS_DATA);
    }

    uint8_t regB = 0;
    outb(CMOS_ADDRESS, CMOS_SELECT_RTCB);
    regB = inb(CMOS_DATA);

    if(!(regB & 0x04)){
        currentTime.second = (currentTime.second & 0x0F) + ((currentTime.second / 16) * 10);
        currentTime.minute = (currentTime.minute & 0x0F) + ((currentTime.minute / 16) * 10);
        currentTime.hour = ( (currentTime.hour & 0x0F) + (((currentTime.hour & 0x70) / 16) * 10) ) | (currentTime.hour & 0x80);
        currentTime.day = (currentTime.day & 0x0F) + ((currentTime.day / 16) * 10);
        currentTime.month = (currentTime.month & 0x0F) + ((currentTime.month / 16) * 10);
        currentTime.year = (currentTime.year & 0x0F) + ((currentTime.year / 16) * 10);
        if(acpiInfo.fadt->century != 0){
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    if(!(regB & 0x02) && (currentTime.hour & 0x80)){
        currentTime.hour = ((currentTime.hour & 0x7F) + 12) % 24;
    }
    
    if(acpiInfo.fadt->century != 0){
        currentTime.year += century * 100;
    }else{
        currentTime.year += 2000;
    }

    AddTimerCallback(UpdateTime, 1000);
    sti
}