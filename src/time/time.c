#include "time.h"
#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <util.h>
#include <memmanage.h>
#include <io.h>
#include <text.h>

// PIT channel 0 - used for timing
#define PIT_A 0x40

// PIT channel 1
#define PIT_B 0x41

// PIT channel 2 (may not exist)
#define PIT_C 0x42

// The PIT control register value
#define PIT_CONTROL 0x43

// This is used to mask values wider than a certain bit width (16 bit to 8 bit)
#define PIT_MASK 0xFF

// The value that must be sent to the PIT to change its values
#define PIT_SET 0x36

// The total refresh rate of the PIT
#define PIT_HZ 1193181

// The maximum amount of timers the kernel can handle
#define MAX_TIMERS 100

// These macros define the divisor of the frequency
#define DIV_OF_FREQ(_f) (PIT_HZ / (_f))
#define FREQ_OF_DIV(_d) (PIT_HZ / (_d))
#define REAL_FREQ_OF_FREQ(_f) (FREQ_OF_DIV(DIV_OF_FREQ((_f))))

// This is an entry that contains data for other timers which will be executed when a timer interrupt is called
typedef struct TimerCallbackEntry{
    TimerCallback callback;             // The function that this timer calls
    uint64 interval;                    // Interval in milliseconds
    uint64 elapsed;                     // Elapsed time in milliseconds
    struct TimerCallbackEntry *next;    // The next timer in the linked list
} PACKED TimerCallbackEntry;

// This is a linked list of timer entries that will also be called when there is a timer interrupt
TimerCallbackEntry *timerCallbacks = NULL;

// This struct holds the timer's data
static struct{
    uint64 freq;
    uint64 divisor;
    uint64 ticks;
} state;

uint16 numcallbacks = 0;

// Note that new timers can't be passed anything and don't return anything. They're just for code execution at a certain interval.
// Add a timer callback to the linked list of timer callbacks
void AddTimerCallback(TimerCallback callback, uint32 interval){
    if(numcallbacks > MAX_TIMERS){
        // This will need to be expanded when I can run applications
        // If the linked list is full, we must return without adding the timer
        return;
    }

    // Search through the timers to make sure this specific timer is not already in the list
    TimerCallbackEntry* entry = timerCallbacks;
    while(entry != NULL){
        if(entry->callback == callback){
            // If the timer is already in the list, return without adding it
            return;
        }
        entry = entry->next;
    }

    // Create a new linked list entry and allocate the nececcary memory for it
    TimerCallbackEntry* newEntry = (TimerCallbackEntry* )alloc(sizeof(TimerCallbackEntry));

    if(newEntry == NULL){
        // This too will have to be expanded
        // If there is not enough memory to allocate the new entry, return
        printk("No Memory!\n");
        return;
    }

    // Set the values of the new timer entry
    newEntry->callback = callback;
    newEntry->interval = interval;
    newEntry->elapsed = 0;
    newEntry->next = NULL;

    if(timerCallbacks == NULL){
        // If the list of timer callbacks is empty, make this new callback the first entry
        timerCallbacks = newEntry;
    }else{
        // Otherwise, iterate through the linked list and append the new entry
        TimerCallbackEntry *entry = timerCallbacks;
        while(entry->next != NULL){
            entry = entry->next;
        }
        entry->next = newEntry;
    }

    numcallbacks++;
}

// Remove a given timer callback from the linked list of callbacks
void RemoveTimerCallback(TimerCallback callback){
    // Set pointers to the linked list
    TimerCallbackEntry* entry = timerCallbacks;
    TimerCallbackEntry* previous = NULL;

    // Iterate through the linked list to find the timer we're looking for
    while(entry != NULL && entry->callback != callback){
        previous = entry;
        entry = entry->next;
    }

    if(entry == NULL){
        // If we reached the end of the linked list without finding the timer, it does not exist and we can safely return
        return;
    }

    if(previous){
        // If this is not the first entry in the list, link the previous entry to the next one
        previous->next = entry->next;
    }else{
        // Otherwise, we can change the poiner to timerCallbacks
        timerCallbacks = entry->next;
    }

    // Finally, we can safely remove the timer and free it from memory
    dealloc(entry);
}


// Force the compiler not to optimize this function
#pragma GCC push_options
#pragma GCC optimize("O0")
// Stop execution for a specified number of milliseconds
void delay(uint64 ms){
    // Get the current time elapsed since the ISR was created
    uint64 currentTicks = GetTicks();

    // Get the total time that will be elapsed when done
    uint64 targetTicks = currentTicks + ms;

    while(currentTicks < targetTicks){
        // Wait until the amount of ticks passed reaches the target
        currentTicks = state.ticks;
    }
    // Wait completed, execution will continue
}
#pragma GCC pop_options

// Set the refresh rate of the PIT
static void SetTimer(int hz){
    // Enable writing a new value to the PIT register
    outb(PIT_CONTROL, PIT_SET);

    // Get the divisor of the PIT frequency (it will be divided by this value because it is 16-bit hardware, so the actual frequency
    // can't be directly modified)
    uint16 d = (uint16)(PIT_HZ / hz);

    // Set the divisor of the frequency to the A channel in the PIT
    outb(PIT_A, d & PIT_MASK);
    outb(PIT_A, d >> 8);
}

// Mainly for functions outside this file to get the amount of ticks that have passed since the IRQ was installed
uint64 GetTicks(){
    return state.ticks;
}


// This function is called at every PIT interrupt
static void timer_handler(struct Registers *regs){
    // Get a pointer to the linked list of loaded callbacks
    TimerCallbackEntry* entry = timerCallbacks;

    while(entry != NULL){
        // As long as there is a valid entry in the list
        // Increase the amount of ms elapsed for this entry
        entry->elapsed += 1;

        if(entry->elapsed == entry->interval){
            // If the amount of time set has passed, call the entry and reset its elapsed time
            entry->callback();
            entry->elapsed = 0;
        }

        // Go to the next entry
        entry = entry->next;
    }

    // Increment the system time
    state.ticks++;

    // Inform the PIC that the interrupt has ended
    outb(PIC_EOI, PIC_EOI);
}

// Note that the PIT is set to 1000hz (1ms between interrupts)

// Get the true frequency in hz of the PIT
const uint64 freq = REAL_FREQ_OF_FREQ(TIMER_TPS);

// Enable the PIT and install its ISRs
void InitializePIT(){
    // Put the frequency into the struct
    state.freq = freq;

    // Set the divisor
    state.divisor = DIV_OF_FREQ(freq);

    // Set the ticks to 0 (since we are initializing the PIT here)
    state.ticks = 0;

    // Set the frequency of the PIT
    SetTimer(TIMER_TPS);

    // Install the timer IRQ to the PIC
    InstallIRQ(TIMER_IRQ, timer_handler);
}
