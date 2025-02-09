#include <alloc.h>

// Get the start of the kernel's heap
uintptr_t heapStart = 0;

uintptr_t heapEnd = 0;

// Define a doubly-linked list of memory blocks
typedef struct MemoryBlock {
    size_t size;
    bool free;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

block_header_t* firstBlock = NULL;

void InitializeAllocator(void){
    firstBlock = (block_header_t*)heapStart;
    firstBlock->size = PAGE_SIZE - HEADER_SIZE;
    firstBlock->free = true;
    firstBlock->next = NULL;
    firstBlock->prev = NULL;
    heapEnd = heapStart + PAGE_SIZE;

    printf("Initialized heap at 0x%x\n", heapStart);
    STOP
}

// First-fit allocation (TODO: add block splitting for smaller and more efficient allocations)
void* halloc(size_t size){
    block_header_t* current = firstBlock;

    while(current->next != NULL){
        if(current->free && current->size >= size){
            printf("Found a free block of size %u\n", current->size);
            current->free = false;
            return (void*)(current + HEADER_SIZE);
        }

        current = current->next;
    }

    if(!current->free){
        // If the last block is not free, allocate new pages and set a block pointer to it (no splitting paged areas for now)
        size_t pagesToAlloc = (size + HEADER_SIZE) / PAGE_SIZE;
        if((size + HEADER_SIZE) % PAGE_SIZE != 0){
            pagesToAlloc++;
        }
        for(size_t i = 0; i < pagesToAlloc; i++){
            if(palloc(heapEnd + (i * PAGE_SIZE), PTE_FLAG_PRESENT | PTE_FLAG_RW) == -1){
                return NULL;
            }
        }
        block_header_t* newBlock = (block_header_t*)heapEnd;
        newBlock->size = (pagesToAlloc * PAGE_SIZE) - HEADER_SIZE;
        newBlock->free = true;
        newBlock->next = NULL;
        newBlock->prev = current;
        current->next = newBlock;
        heapEnd += pagesToAlloc * PAGE_SIZE;
        printf("Allocated new block of size %u\n", newBlock->size);
        return (void*)(newBlock + HEADER_SIZE);
    }

    if(current->size <= size){
        size_t missingSize = size - current->size;
        size_t pagesToAlloc = missingSize / PAGE_SIZE;
        if(missingSize % PAGE_SIZE != 0){
            pagesToAlloc++;
        }
        for(size_t i = 0; i < pagesToAlloc; i++){
            palloc(heapEnd + (i * PAGE_SIZE), PTE_FLAG_PRESENT | PTE_FLAG_RW);
        }
        current->size += pagesToAlloc * PAGE_SIZE;
        heapEnd += pagesToAlloc * PAGE_SIZE;

        printf("Extended block to size %u\n", current->size);
        return (void*)(current + HEADER_SIZE);
    }

    // There was an allocation failure
    return NULL;
}

void hfree(UNUSED void* ptr){
    return;
}