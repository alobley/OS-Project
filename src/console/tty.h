#ifndef TTY_H
#define TTY_H

#include <devices.h>
#include <vfs.h>
#include <multitasking.h>
#include <kernel.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void InitializeTTY(void);                                   // Initialize the TTYs
tty_t* GetActiveTTY(void);                                  // Get the active tty
int SetActiveTTYByNum(short ttyNum);                        // Set the active tty by using its number
int SetActiveTTYByPtr(tty_t* tty);                          // Set the active tty by using its pointer
int SetActiveTTYByName(const char* path);                   // Search through /dev for the tty
int TTYWrite(tty_t* tty, const char* text, size_t size);    // Write some text to the tty
int TTYRead(tty_t* tty, char* buffer, size_t size);         // Read some text from the tty
int TTYClear(tty_t* tty);                                   // Clear the tty
void UpdateTTY();
//int TTYScroll(tty_t* tty, bool up);                       // Scroll the tty up or down

#endif