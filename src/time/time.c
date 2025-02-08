#include <time.h>
#include <alloc.h>
#include <console.h>

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

#define MAX_TIMERS 256

uint16_t numTimers = 0;

void AddTimerCallback(timer_callback_t callback, uint64_t interval){
    if(numTimers >= MAX_TIMERS){
        // There are too many timers!
        printf("KERNEL PANIC: Too many timers!\n");
        return;
    }
    TimerCallbackEntry* entry = (TimerCallbackEntry*)ALLOC_STUB(sizeof(TimerCallbackEntry));
    if(entry == NULL){
        // Memory allocation failed
        printf("KERNEL PANIC: Memory allocation failed!\n");
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

void RemoveTimerCallback(timer_callback_t* callback){
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

    DEALLOC_STUB(entry);
}

uint64_t GetTicks(){
    return state.ticks;
}

// Busy-wait in order to delay (when multitasking is implemented, put the process to sleep instead, saves time and resources)
void delay(uint64_t ms){
    uint64_t currentTicks = GetTicks();
    uint64_t end = currentTicks + ms;
    while(currentTicks < end){
        currentTicks = GetTicks();
    }
}

void SetTimer(int hz){
    outb(PIT_COMMAND, PIT_SET);
    uint16_t divisor = PIT_HZ / hz;

    outb(PIT_CHANNEL0, divisor & PIT_MASK);
    outb(PIT_CHANNEL0, (divisor >> 8) & PIT_MASK);
}

static void TimerHandler(UNUSED struct Registers* regs){
    state.ticks++;
    TimerCallbackEntry* entry = timerCallbacks;
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