#include <alloc.h>
#include <kernel.h>
#include <system.h>
#include <multitasking.h>
#include <console.h>
#include <system.h>

// Get the start of the kernel's heap
uintptr_t heapStart = 0;

uintptr_t heapEnd = 0;

uintptr_t heapTerm = 0;

// Define a doubly-linked list of memory blocks
typedef struct MemoryBlock {
    uint32_t magic;
    size_t size;
    bool free;
    uint32_t padding; // Padding to ensure proper alignment
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

block_header_t* firstBlock = NULL;

// Although targeting a single-CPU environment, multiple processes might request kernel resources at the same time. Allocation is quick so wait times likely won't be long.
spinlock_t heapLock = SPINLOCK_INIT;

void InitializeAllocator(void){
    firstBlock = (block_header_t*)heapStart;
    firstBlock->size = PAGE_SIZE - HEADER_SIZE;
    firstBlock->free = true;
    firstBlock->next = NULL;
    firstBlock->prev = NULL;
    firstBlock->magic = MEMBLOCK_MAGIC;
    heapEnd = heapStart + PAGE_SIZE;
    heapTerm = USER_MEM_START;          // The end of the heap is the start of user memory (If I ever go higfher-half, this will need to be changed)

    for(int i = 0; i < 500; i++){
        // Helps detect bad RAM
        nop
    }

    if(firstBlock->magic != MEMBLOCK_MAGIC){
        printk("KERNEL PANIC: Kernel could not write memory to 0x%x.\n", firstBlock);
        printk("Physical address of offending memory: 0x%x\n", GetPhysicalAddress((virtaddr_t)firstBlock));
        printk("Tried to write: 0x%x\n", MEMBLOCK_MAGIC);
        printk("Value found: 0x%x\n", firstBlock->magic);
        STOP
    }
}

// More complex memory allocation algorithm that takes blocks and makes them exactly the correct size
MALLOC void* halloc(size_t size){
    if(size == 0){
        return NULL;
    }

    while(SpinLock(&heapLock, GetCurrentProcess()) == SEM_LOCKED){
        // Spin
    }

    block_header_t* current = firstBlock;
    block_header_t* previous = NULL;
    while(current != NULL){
        if(current->magic != MEMBLOCK_MAGIC){
            printk("KERNEL PANIC: Invalid memory block magic number 0x%x\n", current->magic);
            STOP
        }
        if(current->free && current->size > size + HEADER_SIZE + 1){
            // A suitable block was found, split it
            block_header_t* newBlock = (block_header_t*)((uint8_t*)current + HEADER_SIZE + size);
            newBlock->size = (current->size - size) - HEADER_SIZE;
            newBlock->free = true;
            newBlock->next = current->next;
            newBlock->prev = current;
            newBlock->magic = MEMBLOCK_MAGIC;
            newBlock->padding = 0; // Ensure proper alignment
            current->size = size;
            current->free = false;
            current->next = newBlock;
            
            // Update the next block's prev pointer if it exists
            if (newBlock->next != NULL) {
                newBlock->next->prev = newBlock;
            }
            
            SpinUnlock(&heapLock);
            //printk("Splitting block\n");
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }else if(current->free && current->size >= size){
            // A suitable block of perfect size was found
            current->free = false;
            SpinUnlock(&heapLock);
            //printk("Perfect block found\n");
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }
        if(current->next == NULL && current->free && current->size < size + HEADER_SIZE){
            // We reached the end of the heap and now must extend it
            // Calculate total needed size including potential split header
            size_t totalNeeded = size + HEADER_SIZE;
            if(totalNeeded + HEADER_SIZE + 1 <= PAGE_SIZE) {
                totalNeeded += HEADER_SIZE + 1; // Account for potential split
            }

            //printk("Extending heap by %u bytes\n", totalNeeded);
            
            // Calculate pages needed with proper rounding
            size_t pagesToAdd = (totalNeeded + (PAGE_SIZE - 1)) / PAGE_SIZE;
            
            // Check if we would exceed memory limits
            if(heapEnd + (pagesToAdd * PAGE_SIZE) > heapTerm || 
               heapEnd + (pagesToAdd * PAGE_SIZE) < heapEnd) {
                SpinUnlock(&heapLock);
                return NULL;
            }
            
            // Allocate pages
            for(size_t i = 0; i < pagesToAdd; i++){
                if(palloc(heapEnd + (i * PAGE_SIZE), PDE_PRESENT | PDE_RW) != PAGE_OK){
                    // Deallocate any pages we've already allocated
                    for(size_t j = 0; j < i; j++) {
                        pfree(heapEnd + (j * PAGE_SIZE));
                    }
                    SpinUnlock(&heapLock);
                    printk("PAGING ERROR\n");
                    STOP
                }
            }

            size_t totalSpace = pagesToAdd * PAGE_SIZE;
            
            if(totalSpace >= size + (2 * HEADER_SIZE) + 1){
                block_header_t* newBlock = (block_header_t*)((uint8_t*)current + HEADER_SIZE + size);
                newBlock->size = totalSpace - size - (2 * HEADER_SIZE);
                newBlock->free = true;
                newBlock->next = NULL;
                newBlock->magic = MEMBLOCK_MAGIC;
                newBlock->prev = current;
                newBlock->padding = 0;
                current->size = size;
                current->next = newBlock;
                //printk("Split block magic number: 0x%x\n", newBlock->magic);
            }else{
                current->size = totalSpace - HEADER_SIZE;
                current->next = NULL;
            }
            
            current->free = false;
            current->magic = MEMBLOCK_MAGIC;
            heapEnd += pagesToAdd * PAGE_SIZE;
            SpinUnlock(&heapLock);
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }
        previous = current;
        current = current->next;
    }

    //printk("Extending heap by %u bytes and creating new block\n", size);

    // We've reached the end of the list and didn't find a suitable block
    if(heapEnd >= heapTerm || heapEnd + size + HEADER_SIZE <= heapEnd){
        SpinUnlock(&heapLock);
        return NULL;
    }

    size_t totalNeeded = size + HEADER_SIZE;
    if(totalNeeded + HEADER_SIZE + 1 <= PAGE_SIZE) {
        totalNeeded += HEADER_SIZE + 1;
    }
    
    size_t pagesToAdd = (totalNeeded + (PAGE_SIZE - 1)) / PAGE_SIZE;
    
    // Check if we would exceed memory limits
    if(heapEnd + (pagesToAdd * PAGE_SIZE) > heapTerm || 
       heapEnd + (pagesToAdd * PAGE_SIZE) < heapEnd) {
        SpinUnlock(&heapLock);
        return NULL;
    }
    
    for(size_t i = 0; i < pagesToAdd; i++){
        if(palloc(heapEnd + (i * PAGE_SIZE), PDE_PRESENT | PDE_RW) != PAGE_OK){
            // Deallocate any pages we've already allocated
            for(size_t j = 0; j < i; j++) {
                pfree(heapEnd + (j * PAGE_SIZE));
            }
            SpinUnlock(&heapLock);
            printk("PAGING ERROR\n");
            STOP
        }
    }
    
    block_header_t* newBlock = (block_header_t*) heapEnd;
    newBlock->size = size;
    newBlock->free = false;
    newBlock->next = NULL;
    newBlock->magic = MEMBLOCK_MAGIC;
    newBlock->padding = 0;
    
    // Fix: properly link the new block to the existing list
    if (previous != NULL) {
        previous->next = newBlock;
        newBlock->prev = previous;
    } else if (firstBlock == NULL) {
        // This would be the first block in the heap
        firstBlock = newBlock;
        newBlock->prev = NULL;
    } else {
        // Shouldn't reach here, but just in case
        newBlock->prev = NULL;
    }
    
    size_t totalSize = pagesToAdd * PAGE_SIZE;
    
    // If we have enough space to split, create another block
    if (totalSize > size + HEADER_SIZE + HEADER_SIZE + 1) {
        block_header_t* splitBlock = (block_header_t*)((uint8_t*)newBlock + HEADER_SIZE + size);
        splitBlock->size = totalSize - size - (2 * HEADER_SIZE);
        splitBlock->free = true;
        splitBlock->next = NULL;
        splitBlock->prev = newBlock;
        splitBlock->magic = MEMBLOCK_MAGIC;
        newBlock->next = splitBlock;
    }
    
    heapEnd += pagesToAdd * PAGE_SIZE;
    SpinUnlock(&heapLock);
    //printk("Memory allocated at 0x%x ", newBlock + HEADER_SIZE);
    return (void*)((uint8_t*)newBlock + HEADER_SIZE);
}

void hfree(void* ptr){
    if(ptr == NULL){
        return;
    }

    //printk("Freeing memory at 0x%x ", ptr);

    while(SpinLock(&heapLock, GetCurrentProcess()) == SEM_LOCKED){
        // Spin
    }

    block_header_t* block = (block_header_t*)((uintptr_t)ptr - HEADER_SIZE);
    if(block->magic != MEMBLOCK_MAGIC){
        // Invalid memory block
        printk("KERNEL PANIC: Invalid memory block magic number: 0x%x\n", block->magic);
        printk("Invalid ptr: 0x%x, header address: 0x%x\n", ptr, block);
        STOP;
    }

    // Mark the block as free
    block->free = true;

    // Coalesce free blocks
    block_header_t* current = firstBlock;
    while(current != NULL){
        if(current->magic != MEMBLOCK_MAGIC){
            // Invalid memory block
            printk("KERNEL PANIC: Invalid memory block magic number during coalescing: 0x%x\n", current->magic);
            STOP;
        }
        if(current->free && current->next != NULL && current->next->free){
            // Merge the two blocks
            current->size += current->next->size + HEADER_SIZE;
            current->next = current->next->next;
            if(current->next != NULL){
                current->next->prev = current;
            }
        }
        current = current->next;
    }

    // Handle special case - if there's only one block and it's free
    if (firstBlock->free && firstBlock->next == NULL) {
        // Check if heap is larger than one page
        if (heapEnd > heapStart + PAGE_SIZE) {
            // Calculate pages to free
            size_t heapSizeInPages = (heapEnd - heapStart) / PAGE_SIZE;
            
            // Keep first page, free the rest
            for (size_t i = 1; i < heapSizeInPages; i++) {
                pfree(heapStart + (i * PAGE_SIZE));
            }
            
            // Reset heap to initial state
            heapEnd = heapStart + PAGE_SIZE;
            firstBlock->size = PAGE_SIZE - HEADER_SIZE;
        }
    } 
    // Try to shrink the heap if the last block is free
    else {
        current = firstBlock;
        // Find the last block
        while (current->next != NULL) {
            current = current->next;
        }
        
        if (current->free) {
            // Calculate the address of the end of the used portion
            uintptr_t usedEnd = (uintptr_t)current;
            
            // Align to page boundary (round up)
            uintptr_t usedEndAligned = ((usedEnd + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
            
            // Free pages from usedEndAligned to heapEnd
            if (usedEndAligned < heapEnd) {
                size_t pagesToFree = (heapEnd - usedEndAligned) / PAGE_SIZE;
                
                for (size_t i = 0; i < pagesToFree; i++) {
                    pfree(usedEndAligned + (i * PAGE_SIZE));
                }
                
                // Update heap end
                heapEnd = usedEndAligned;
                
                // Adjust the size of the last block if needed
                if ((uintptr_t)current + current->size + HEADER_SIZE > heapEnd) {
                    current->size = heapEnd - ((uintptr_t)current + HEADER_SIZE);
                }
            }
        }
    }

    SpinUnlock(&heapLock);
}

// Reallocate some memory to a new size (expects the new area added to be initialized to 0)
MALLOC void* rehalloc(void* ptr, size_t newSize){
    if(ptr == NULL){
        char* newPtr = halloc(newSize);
        if(newPtr == NULL){
            return NULL;
        }
        memset(newPtr, 0, newSize);
        return newPtr;
    }
    block_header_t* block = (block_header_t*)((uintptr_t)ptr - HEADER_SIZE);
    if(block->magic != MEMBLOCK_MAGIC){
        printk("KERNEL PANIC: Invalid memory block magic number: 0x%x\n", block->magic);
        
        STOP
    }

    if(block->size >= newSize){
        // The new size is too small
        return ptr;
    }

    void* newPtr = halloc(newSize);
    if(newPtr == NULL){
        // Failed to allocate new memory
        return NULL;
    }
    memset(newPtr, 0, newSize);
    memcpy(newPtr, ptr, block->size);
    hfree(ptr);
    return newPtr;
}