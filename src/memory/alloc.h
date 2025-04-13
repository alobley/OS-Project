#ifndef ALLOC_H
#define ALLOC_H

#include <paging.h>
#include <stddef.h>

#define MEMBLOCK_MAGIC 0xDEADBEEF

extern uintptr_t heapStart;
extern uintptr_t heapEnd;

// Allocate a block of memory on the heap of the specified size
void* halloc(size_t size);

// Deallocate a block of memory on the heap
void hfree(void* ptr);

// Reallocate a block of memory
void* rehalloc(void* ptr, size_t newSize);

void InitializeAllocator(void);

#endif