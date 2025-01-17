#include <memmanage.h>
#include <vga.h>
#include <util.h>
#include <acpi.h>

// For paging the BIOS to another location (or not at all)
#define BIOS_START 0xF0000
#define BIOS_END 0xFFFFF
#define BIOS_SIZE (BIOS_END - BIOS_START + 1)

// NOTE:
// Virtual address translates directly into a page directory index and page table index. There's nothing else to it. That's what the virtual address means.
// The physical address is 4KiB-aligned and in the page entry itself.
extern uint32 __kernel_end;
extern uint32 __kernel_start;

size_t numPages = 0;

// For allocation of new page directories and page tables
typedef struct process_page {
    PageDirectory* dir;
    size_t size;                // These regions will include entire page directories and page tables
    struct process_page* next;
} procpage_t;

uint32 KERNEL_FREE_HEAP_BEGIN;
uint32 KERNEL_FREE_HEAP_END;
size_t kernel_heap_end;


typedef struct memory_block {
    size_t size;                 // Size of the memory block
    struct memory_block* next;   // Pointer to the next free block of memory
    bool free;                   // Block free or bot
    bool paged;                  // Is this block paged?
} memory_block_t;

#define MEMORY_BLOCK_SIZE (sizeof(memory_block_t))

memory_block_t* kernel_heap;

size_t memSize = 0;
uintptr_t vgaRegion = NULL;
uintptr_t heapStart = 0;
size_t totalPages = 0;

uintptr_t next_free_physaddr = 0;
uintptr_t next_free_virtaddr = 0;

PageDirectoryEntry ALIGNED(4096) pageDir[1024] = {0};
PageTable ALIGNED(4096) pageTables[1024] = {0};

PageDirectory* currentDir;

// Activate a set of new pages in a page directory. Assumes a full and valid directory.
page_t* palloc(uintptr_t virtualAddr, uintptr_t physicalAddr, size_t pagesToAdd /*Add only more than one when certain there is a contiguous block of free virtual addresses*/, PageDirectory* pageDir, bool user){
    // Get the page directory entry
    if (virtualAddr % 4096 != 0 || physicalAddr % 4096 != 0) {
        return NULL; // Addresses must be 4KiB aligned
    }
    page_t* firstPage = NULL;
    for(size_t i = 0; i < pagesToAdd; i++){
        PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr + (i * 4096))];
        PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);                               // palloc assumes a full, valid page directory
        if(table->entries[PTI(virtualAddr + (i * 4096))].present && dirEntry->present){
            // Page already allocated
            return NULL;
        }else{
            table->entries[PTI(virtualAddr + (i * 4096))].present = 1;                                      // This page is active
            dirEntry->present = 1;
            dirEntry->user = user;
            dirEntry->readWrite = 1;
            dirEntry->writeThrough = 0;
            dirEntry->cacheDisabled = 0;
            dirEntry->accessed = 0;
            dirEntry->dirty = 0;
            dirEntry->pageSize = 0;
            dirEntry->available = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].address = (physicalAddr + (i * 4096)) >> 12;      // The physical memory location of the page
            table->entries[PTI(virtualAddr + (i * 4096))].user = user;
            table->entries[PTI(virtualAddr + (i * 4096))].readWrite = 1;
            table->entries[PTI(virtualAddr + (i * 4096))].writeThrough = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].cacheDisabled = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].accessed = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].dirty = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].pageSize = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].global = 0;
            table->entries[PTI(virtualAddr + (i * 4096))].available = 0;
            numPages++;
            if(i == 0){
                firstPage = &table->entries[PTI(virtualAddr + (i * 4096))];
            }
        }
    }

    // Tell the CPU there was a change in the page directory
    asm volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
    return firstPage;
}

void pfree(uintptr_t virtualAddr, PageDirectory* pageDir){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
    if(table->entries[PTI(virtualAddr)].present){
        table->entries[PTI(virtualAddr)].present = 0;
        table->entries[PTI(virtualAddr)].dirty = 1;         // We assume it has been written to
        numPages--;
    }

    asm volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
}

uintptr_t FindUnpagedMemoryHigh(PageDirectory* pageDir){
    // Find a 4KiB-aligned region of physical memory that is not paged and above the kernel. Search the entire page directory.
    uintptr_t address = ((((uintptr_t)&__kernel_end) >> 12) << 12) + 4096;                           // Set it to the kernel's end address (should skip paged memory)
    for(size_t i = 0; i < 1024; i++){
        PageDirectoryEntry* dirEntry = &pageDir->entries[i];
        if(dirEntry->present){
            PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
            for(size_t j = 0; j < 1024; j++){
                if(table->entries[j].present && (table->entries[j].address << 12) == address){
                    address += 4096;
                }else{
                    return address;
                }
            }
        }
    }

    // No free memory above the kernel found in the page directory
    return 0;
}

// Allocate a new page
void AllocatePage(uintptr_t virtualAddr, PageDirectory* pageDir, bool user){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
    if(!dirEntry->present){
        dirEntry->address = (uint32)(&pageTables[PTI(virtualAddr)]) >> 12;
        dirEntry->present = 0;
        dirEntry->readWrite = 1;
        dirEntry->user = user;
        dirEntry->writeThrough = 0;
        dirEntry->cacheDisabled = 0;
        dirEntry->accessed = 0;
        dirEntry->dirty = 0;
        dirEntry->pageSize = 0;
        dirEntry->available = 0;
    }
    if(table->entries[PTI(virtualAddr)].present){
        // The page is already allocated in this directory (need new directory to remap)
        return;
    }else{
        // Allocate a new page
        table->entries[PTI(virtualAddr)].address = 0;
        table->entries[PTI(virtualAddr)].present = 0;
        table->entries[PTI(virtualAddr)].readWrite = 1;
        table->entries[PTI(virtualAddr)].user = user;
        table->entries[PTI(virtualAddr)].writeThrough = 0;
        table->entries[PTI(virtualAddr)].cacheDisabled = 0;
        table->entries[PTI(virtualAddr)].accessed = 0;
        table->entries[PTI(virtualAddr)].dirty = 0;
        table->entries[PTI(virtualAddr)].pageSize = 0;
        table->entries[PTI(virtualAddr)].global = 0;
        table->entries[PTI(virtualAddr)].available = 0;
    }
}

// Allocate a new page directory
PageDirectory* AllocatePageDirectory(size_t virtualAddr, void* start, bool user){
    PageDirectory* dir = start;

    for(size_t i = 0; i < 1024; i++){
        // Allocate the page tables
        dir->entries[i].address = (uint32)(&pageTables[i]) >> 12;
        dir->entries[i].present = 0;
        dir->entries[i].readWrite = 1;
        dir->entries[i].user = user;
        dir->entries[i].writeThrough = 0;
        dir->entries[i].cacheDisabled = 0;
        dir->entries[i].accessed = 0;
        dir->entries[i].dirty = 0;
        dir->entries[i].pageSize = 0;
        dir->entries[i].available = 0;
        for(int j = 0; j < 1024; j++){
            // Allocate the pages from the first given physical address and the first given virtual address
            AllocatePage(virtualAddr + (i * sizeof(pageTables) * 1024) + (j * 4096), dir, user);
        }
    }

    return dir;
}

uintptr_t GetVgaRegion(){
    return vgaRegion;
}

size_t GetPages(){
    return numPages;
}

size_t GetTotalMemory(){
    return memSize;
}

// We need the memory map to determine what memory is available
mboot_mmap_entry_t* memoryMap;
size_t mmapLen;

// This is important for the kernel heap, and will also page important memory regions such as VGA memory and ACPI tables
// TODO: make this a higher-half kernel
void PageKernel(size_t totalmem, mboot_mmap_entry_t* mmap, size_t mmapLength){
    totalPages = totalmem / 4096;
    if(totalmem % 4096 != 0){
        totalPages++;
    }

    // Make sure to include the kernel's page directory in the mapping
    size_t kernelPages = ((uintptr_t)&__kernel_end - (uintptr_t)&__kernel_start) / 4096;
    if(((uintptr_t)&__kernel_end - (uintptr_t)&__kernel_start) % 4096 != 0){
        kernelPages++;
    }

    // Get the page directory and assign its virtual address to its physical address
    AllocatePageDirectory(&pageDir[0], &pageDir[0], false);

    //palloc(0, 0, totalPages, &pageDir[0], false);
    //return;

    // Set the kernel's pages to active (virtual address will change later but for now identity map it)
    page_t* firstKernelPage = palloc((uintptr_t)&__kernel_start, (uintptr_t)&__kernel_start, kernelPages, &pageDir[0], false);
    if(firstKernelPage == NULL){
        return;
    }

    // Map VGA memory to right after the kernel memory
    //size_t vgaSize = VGA_PIXEL_MODE_SIZE + VGA_TEXT_MODE_SIZE;
    size_t kernelSize = kernelPages * 4096;
    

    size_t vgaPages = (1024 * 256) / 4096;
    if(VGA_REGION_SIZE % 4096 != 0){
        vgaPages++;
    }
    
    vgaRegion = FindUnpagedMemoryHigh(&pageDir[0]);
    page_t* firstVgaPage = palloc(vgaRegion, 0xA0000, vgaPages, &pageDir[0], false);

    for(size_t i = 0; i < vgaPages; i++){
        // The framebuffer is an MMIO region, so it must be write-through
        firstVgaPage[i].writeThrough = 1;
        firstVgaPage[i].cacheDisabled = 1;
        firstVgaPage[i].readWrite = 1;
        firstVgaPage[i].present = 1;
        firstVgaPage[i].global = 1;
        firstVgaPage[i].user = false;
    }

    // Remap the BIOS...
    // Remap the ACPI tables...

    memSize = totalmem;

    heapStart = vgaRegion + vgaPages * 4096;

    next_free_physaddr = heapStart;
    next_free_virtaddr = (firstVgaPage[vgaPages - 1].address << 12) + 4096;

    // Page the memory map to just after the kernel and framebuffer
    size_t mapPages = mmapLength / 4096;
    if(mmapLength % 4096 != 0){
        mapPages++;
    }
    uintptr_t startAddr = FindUnpagedMemoryHigh(&pageDir[0]);
    page_t* firstMapPage = palloc(heapStart, startAddr, mapPages, &pageDir[0], false);

    memoryMap = (mboot_mmap_entry_t*)heapStart;

    heapStart += mapPages * 4096;

    startAddr = FindUnpagedMemoryHigh(&pageDir[0]);
    page_t* firstHeapPage = palloc(heapStart, startAddr, 1, &pageDir[0], false);

    kernel_heap = (memory_block_t*)heapStart;
    kernel_heap->size = 4096;
    kernel_heap->next = NULL;
    kernel_heap->free = true;
    kernel_heap->paged = true;
    kernel_heap_end = heapStart + 4096;
    KERNEL_FREE_HEAP_BEGIN = heapStart;
    KERNEL_FREE_HEAP_END = memSize;

    // Map the rest of the memory
    size_t remainingPages = totalPages - kernelPages - vgaPages;
    uint32 lowmemPages = (uintptr_t)&__kernel_start / 4096;

    currentDir = &pageDir[0];

    // Enable paging
    cr3((uint32)&pageDir[0]);
    uint32 currentCr0 = 0;
    get_cr0(currentCr0);
    cr0(currentCr0 | 0x80000001);
}


void PanicFree(){
    // Free all memory
    memory_block_t* current = kernel_heap;
    while(current != NULL){
        memory_block_t* next = current->next;
        dealloc((void*)((uint8*)current + MEMORY_BLOCK_SIZE));
        current = next;
    }
}

// Allocate memory and return a pointer to it
void* alloc(size_t size) {
    memory_block_t* current = kernel_heap;
    memory_block_t* prev = NULL;

    while (current != NULL) {
        if (current->free && current->size >= size && current->paged) {
            // Suitable free block found
            if (current->size >= size + (MEMORY_BLOCK_SIZE * 2) + 1 /*Ensure there's enough space for another memory block at the end and at least one byte of data*/) {
                // Split the block if it's big enough
                memory_block_t* new_block = (memory_block_t*)((uint8*)current + MEMORY_BLOCK_SIZE + size);
                new_block->size = current->size - size - MEMORY_BLOCK_SIZE;
                new_block->next = current->next;
                new_block->free = true;

                current->size = size;
                current->next = new_block;
            }
            current->free = false;
            kernel_heap_end += size + MEMORY_BLOCK_SIZE;
            return (void*)((uint8*)current + MEMORY_BLOCK_SIZE);
        }
        prev = current;
        current = current->next;
    }

    // No suitable block found, expand heap
    if (kernel_heap_end + size + MEMORY_BLOCK_SIZE < KERNEL_FREE_HEAP_END) {
        uint32 pagesToAdd = size / 4096;
        if (size % 4096 != 0) {
            pagesToAdd++;
        }
        for(int i = 0; i < pagesToAdd; i++){
            uintptr_t newStart = FindUnpagedMemoryHigh(currentDir);
            if(newStart == 0){
                // No memory available or error
                return NULL;
            }
            page_t* newPage = palloc(kernel_heap_end + (i * 4096), newStart, 1, currentDir, false);
            if(newPage == NULL){
                // No memory available or error
                return NULL;
            }
        }
        // If the last block is free, find it and expand it
        if(prev != NULL && prev->free){
            prev->size += size;
            kernel_heap_end += size;
            return (void*)((uint8*)prev + MEMORY_BLOCK_SIZE);
        }else{
            memory_block_t* new_block = (memory_block_t*)kernel_heap_end;
            kernel_heap_end += size + MEMORY_BLOCK_SIZE;

            new_block->size = size;
            new_block->next = NULL;
            new_block->free = false;
            new_block->paged = true;

            if (prev != NULL) {
                prev->next = new_block;
                prev = new_block;                   // Make prev the new block for consistency
            } else {
                // Edge case, will likely never happen
                kernel_heap = new_block;
                prev = new_block;                   // Prev is used for allocation, so it must be set (yes I know it should be current)
            }
        }

        if(size < pagesToAdd * 4096 && (pagesToAdd * 4096) - size > MEMORY_BLOCK_SIZE /*Ensure there's at least enough space for a memory block and one byte*/){
            // The block does not fill the entire page, so split it
            memory_block_t* new_block2 = (memory_block_t*)(kernel_heap_end + MEMORY_BLOCK_SIZE + size);
            new_block2->size = (pagesToAdd * 4096) - size - MEMORY_BLOCK_SIZE;
            new_block2->next = NULL;
            new_block2->free = true;
            new_block2->paged = true;
            prev->next = new_block2;
        }else if(size < pagesToAdd * 4096){
            // The block does not fill the entire page, but there's not enough space for a new block
            // Just make the block bigger
            prev->size += (pagesToAdd * 4096) - size;
        }

        return (void*)((uint8*)prev + MEMORY_BLOCK_SIZE);
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

    // Not sure if removing pages in this way is a good idea, but even without this code there's faults
    if(block->size % 4096 != 0){
        // The block is not page-aligned, best not to release it
        // (resize and unpage?)
        return;
    }else{
        // The block is page-aligned
        size_t pages = block->size / 4096;
        for(size_t i = 0; i < pages; i++){
            pfree((uintptr_t)block + (i * 4096), currentDir);
        }
    }
}
