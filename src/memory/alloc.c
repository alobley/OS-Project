#include "alloc.h"
#include <vga.h>
#include <util.h>
#include <io.h>

extern uint32 __kernel_end;

uint32 KERNEL_FREE_HEAP_BEGIN;
uint32 KERNEL_FREE_HEAP_END;


typedef struct memory_block {
    size_t size;                 // Size of the memory block
    struct memory_block* next;   // Pointer to the next free block of memory
    bool free;                   // Block free or bot
} memory_block_t;

#define MEMORY_BLOCK_SIZE (sizeof(memory_block_t))

memory_block_t* kernel_heap;

uint32 kernel_heap_end;

uint32 totalMem = 0;

uint32 GetTotalMemory(){
    return totalMem;
}

uint32 PAGE_TABLE_AREA_BEGIN;
uint32 PAGE_TABLE_AREA_END;

void InitializeMemory(size_t memSize){
    totalMem = memSize;

    // Reserve space for the page tables
    PAGE_TABLE_AREA_BEGIN = kernel_heap_end;
    if(PAGE_TABLE_AREA_BEGIN % 4096 != 0){
        PAGE_TABLE_AREA_BEGIN += 4096 - (PAGE_TABLE_AREA_BEGIN % 4096);
    }
    PAGE_TABLE_AREA_END = PAGE_TABLE_AREA_BEGIN + (1024 * 4096); 

    // TODO: 
    // - Overhaul memory management and paging to be more efficient (and for paging, understand it properly)
    // - Implement a kernel heap and the ability to allocate heap memory for userland programs
    // - With both of those, it's important to also provide applications access to the VGA framebuffer somehow

    KERNEL_FREE_HEAP_END = memSize;
    kernel_heap_end = PAGE_TABLE_AREA_END;

    KERNEL_FREE_HEAP_BEGIN = __kernel_end;
    kernel_heap = (memory_block_t *)KERNEL_FREE_HEAP_BEGIN;

    kernel_heap->size = 0;
    kernel_heap->next = NULL;
    kernel_heap->free = true;
}

// Allocate memory and return a pointer to it
void* alloc(size_t size){
    memory_block_t* current = kernel_heap;
    memory_block_t* prev = NULL;

    while(current != NULL){
        if(current->free && current->size >= size){
            // Suitable free block found
            if(current->size >= size + MEMORY_BLOCK_SIZE + 1){
                // Split the block if it's big enough
                memory_block_t* new_block = (memory_block_t* )((uint8* )current + MEMORY_BLOCK_SIZE + size);
                new_block->size = current->size - size - MEMORY_BLOCK_SIZE;
                new_block->next = current->next;
                new_block->free = true;

                current->size = size;
                current->next = new_block;
            }
            current->free = false;
            return (void *)((uint8 *)current + MEMORY_BLOCK_SIZE);
        }
        prev = current;
        current = current->next;
    }

    // No suitable block found, expand heap
    if(kernel_heap_end + size + MEMORY_BLOCK_SIZE < KERNEL_FREE_HEAP_END){
        memory_block_t* new_block = (memory_block_t* )kernel_heap_end;
        new_block->size = size;
        new_block->next = NULL;
        new_block->free = false;

        if(prev != NULL){
            prev->next = new_block;
        }else{
            kernel_heap = new_block;
        }

        kernel_heap_end += size + MEMORY_BLOCK_SIZE;
        return (void* )((uint8* )new_block + MEMORY_BLOCK_SIZE);
    }

    // Out of memory
    return NULL;
}

// Free an allocated pointer
void dealloc(void* ptr){
    if(ptr == NULL){
        // Data does not exist
        return;
    }

    memory_block_t* block = (memory_block_t* )((uint8* )ptr - MEMORY_BLOCK_SIZE);
    block->free = true;

    // Coalesce adjacent free blocks to prevent memory fragmentation
    memory_block_t* current = kernel_heap;
    while(current != NULL){
        if(current->free && current->next != NULL && current->next->free){
            current->size += current->next->size + MEMORY_BLOCK_SIZE;
            current->next = current->next->next;
        }
        current = current->next;
    }
}