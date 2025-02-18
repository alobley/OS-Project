#include <alloc.h>

// Get the start of the kernel's heap
uintptr_t heapStart = 0;

uintptr_t heapEnd = 0;

// Define a doubly-linked list of memory blocks
typedef struct MemoryBlock {
    uint32_t magic;
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
    firstBlock->magic = MEMBLOCK_MAGIC;
    heapEnd = heapStart + PAGE_SIZE;
}

// More complex memory allocation algorithm that takes blocks and makes them exactly the correct size
void* halloc(size_t size){ 
    // Get the kernel's first heap memory block
    block_header_t* current = firstBlock;
    block_header_t* previous = NULL;
    while(current != NULL){
        if(current->magic != MEMBLOCK_MAGIC){
            // There was a memory corruption or invalid allocation
            printf("KERNEL PANIC: Invalid memory block magic number 0x%x\n", current->magic);
            STOP
        }
        if(current->free && current->size > size + HEADER_SIZE + 1 /*Make sure the size is right and there's enough space for another memory block and at least one byte*/){
            // The block is free and large enough
            block_header_t* newBlock = (block_header_t*)((uint8_t*)current + HEADER_SIZE + size);
            newBlock->size = (current->size - size) - HEADER_SIZE;
            newBlock->free = true;
            newBlock->next = current->next;
            newBlock->prev = current;
            newBlock->magic = MEMBLOCK_MAGIC;
            current->size = size;
            current->free = false;
            current->next = newBlock;
            //WriteString("Found block, split it\n");
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }else if(current->free && current->size >= size){
            // The block is free and the right size
            // If there is not enough space for another block but it's too big, just allocate the whole block
            current->free = false;
            //WriteString("Found block, allocating whole block\n");
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }
        if(current->next == NULL && current->free && current->size < size + HEADER_SIZE){
            // The current block is free and the last block, but it's not large enough
            //WriteString("Last block is too small, allocating new block\n");
            current->size = size + HEADER_SIZE;
            size_t pagesToAdd = current->size / PAGE_SIZE;
            if(current->size % PAGE_SIZE != 0){
                pagesToAdd++;
            }
            for(size_t i = 0; i < pagesToAdd; i++){
                palloc((uintptr_t)current + (i * PAGE_SIZE), PDE_FLAG_PRESENT | PDE_FLAG_RW);
            }
            if(pagesToAdd * PAGE_SIZE > current->size + HEADER_SIZE + 1){
                // The block is too large
                //WriteString("Block is too large, splitting\n");
                block_header_t* newBlock = (block_header_t*)((uint8_t*)current + HEADER_SIZE + size);
                newBlock->size = (pagesToAdd * PAGE_SIZE) - size - HEADER_SIZE;
                newBlock->free = true;
                newBlock->next = NULL;
                newBlock->magic = MEMBLOCK_MAGIC;
                newBlock->prev = current;
                current->size = size + HEADER_SIZE;
                current->next = newBlock;
            }else if(pagesToAdd * PAGE_SIZE > current->size){
                // Too big, but not enough for another block
                //WriteString("Block is too big, allocating whole block\n");
                current->size += (pagesToAdd * PAGE_SIZE) - current->size;
            }else{
                // Just right
                //WriteString("Block is just right\n");
                current->size = pagesToAdd * PAGE_SIZE;
                current->next = NULL;
            }
            totalPages += pagesToAdd;
            current->free = false;
            current->magic = MEMBLOCK_MAGIC;
            heapEnd += pagesToAdd * PAGE_SIZE;
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }
        previous = current;
        current = current->next;
    }

    current = previous;

    if(heapEnd >= totalMemSize || heapEnd + size + HEADER_SIZE >= heapEnd){
        // Out of memory
        //WriteString("Out of memory\n");
        return NULL;
    }

    // No free memory block found, allocate a new one
    //WriteString("No free memory block found, allocating new block\n");
    size_t pagesToAdd = (size + HEADER_SIZE) / PAGE_SIZE;
    if((size + HEADER_SIZE) % PAGE_SIZE != 0){
        pagesToAdd++;
    }
    for(size_t i = 0; i < pagesToAdd; i++){
        palloc(heapEnd + (i * PAGE_SIZE), PDE_FLAG_PRESENT | PDE_FLAG_RW);
    }
    totalPages += pagesToAdd;
    block_header_t* newBlock = (block_header_t*) heapEnd;
    newBlock->size = size;
    newBlock->free = false;
    newBlock->next = NULL;
    current->next = newBlock;
    heapEnd += pagesToAdd * PAGE_SIZE;
    return (void*)((uint8_t*)newBlock + HEADER_SIZE);
}


void hfree(UNUSED void* ptr){
    if(ptr == NULL){
        return;
    }

    block_header_t* block = (block_header_t*)((uintptr_t)ptr - HEADER_SIZE);
    if(block->magic != MEMBLOCK_MAGIC){
        // Invalid memory block
        printf("KERNEL PANIC: Invalid memory block magic number 0x%x\n", block->magic);
        STOP;
    }else{
        // Free the block
        block->free = true;
    }

    // Coalesce adjacent free blocks
    block_header_t* current = firstBlock;
    while(current != NULL && current->next != NULL){
        if(current->free && current->next != NULL && current->next->free){
            // Coalesce the blocks
            current->size += current->next->size + HEADER_SIZE;
            current->next->magic = 0;                                   // Invalidate the block, just in case
            current->next = current->next->next;
        }
        current = current->next;
    }

    // Free unused pages...
}