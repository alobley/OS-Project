#ifndef ALLOC_H
#define ALLOC_H

#define IDT_BASE 0x00001000

#include <types.h>
void* alloc(size_t amount);
void dealloc(void* ptr);

// Emergencies only - clear the kernel's heap entirely
//void PANIC_FREE_HEAP();

// Initialize the kernel's memory heap
void InitializeMemory(size_t memSize);

uint32 GetTotalMemory();

#endif