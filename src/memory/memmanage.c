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

memory_block_t* kernel_heap = NULL;

size_t numPages = 0;

uint32 KERNEL_FREE_HEAP_BEGIN;
uint32 KERNEL_FREE_HEAP_END;
size_t kernel_heap_end;

size_t memSize = 0;
uintptr_t vgaRegion = NULL;
uintptr_t heapStart = 0;
size_t totalPages = 0;

//uintptr_t next_free_physaddr = 0;
//uintptr_t next_free_virtaddr = 0;

PageDirectoryEntry ALIGNED(4096) pageDir[1024] = {0};
PageTable ALIGNED(4096) pageTables[1024] = {0};

PageDirectory* currentDir;

bool IsPagePresent(uintptr_t virtualAddr, PageDirectory* pageDir){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    if(dirEntry->present){
        PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
        if(table->entries[PTI(virtualAddr)].present){
            return true;
        }
    }

    return false;
}

// Activate a set of new pages in a page directory. Assumes a full and valid directory.
page_t* palloc(uintptr_t virtualAddr, uintptr_t physicalAddr, size_t pagesToAdd /*Add only more than one when certain there is a contiguous block of free virtual addresses*/, PageDirectory* pageDir, bool user){
    // Get the page directory entry
    bool firstPagePaged = false;
    if(pagesToAdd == 0){
        return NULL;
    }
    page_t* firstPage = NULL;
    for(size_t i = 0; i < pagesToAdd; i++){
        PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr + (i * PAGE_SIZE))];
        PageTable* table = (PageTable*)(dirEntry->address << 12);                               // palloc assumes a full, valid page directory
        if(i == 0){
            firstPage = &table->entries[PTI(virtualAddr + (i * PAGE_SIZE))];
        }
        if(table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].present && dirEntry->present){
            // Page already allocated
            continue;               // There must be a better way. If it's already allocated, it's probably there for a reason.
        }else{
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].present = 1;                                      // This page is active
            dirEntry->present = 1;
            dirEntry->user = user;
            dirEntry->readWrite = 1;
            dirEntry->writeThrough = 0;
            dirEntry->cacheDisabled = 0;
            dirEntry->accessed = 0;
            dirEntry->dirty = 0;
            dirEntry->pageSize = 0;
            dirEntry->available = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].address = (physicalAddr + (i * PAGE_SIZE)) >> 12;      // The physical memory location of the page
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].user = user;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].readWrite = 1;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].writeThrough = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].cacheDisabled = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].accessed = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].dirty = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].pageSize = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].global = 0;
            table->entries[PTI(virtualAddr + (i * PAGE_SIZE))].available = 0;
            asm volatile("invlpg (%0)" :: "r"(virtualAddr + (i * PAGE_SIZE)) : "memory");
            numPages++;
        }
    }

    if(firstPage == NULL){
        STOP;
    }

    // Tell the CPU there was a change in the page directory
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

page_t* GetPage(uintptr_t virtAddr){
    PageDirectoryEntry* dirEntry = &currentDir->entries[PDI(virtAddr)];
    if(dirEntry->present){
        PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
        if(table->entries[PTI(virtAddr)].present){
            return &table->entries[PTI(virtAddr)];
        }
    }

    return NULL;
}

// We need the memory map to determine what memory is available
mboot_mmap_entry_t* memoryMap;
size_t mmapLen;
uintptr_t FindUnpagedMemoryHigh(PageDirectory* pageDir){
    // Find a 4KiB-aligned region of physical memory that is not paged and above the kernel. Search the entire page directory.
    uintptr_t address = ((((uintptr_t)&__kernel_end) >> 12) << 12) + PAGE_SIZE;                           // Set it to the kernel's end address (should skip paged memory)
    for(size_t i = 0; i < 1024; i++){
        PageDirectoryEntry* dirEntry = &pageDir->entries[i];
        if(dirEntry->present){
            PageTable* table = (PageTable*)(dirEntry->address << 12);
            for(size_t j = 0; j < 1024; j++){
                if(table->entries[j].present && (table->entries[j].address << 12) == address || table->entries[j].present && (table->entries[j].address << 12) < address || table->entries[j].readWrite == 0){
                    address += PAGE_SIZE;
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
            AllocatePage(virtualAddr + (i * sizeof(pageTables) * 1024) + (j * PAGE_SIZE), dir, user);
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

PageDirectory* GetCurrentPageDir(){
    return currentDir;
}

// This is important for the kernel heap, and will also page important memory regions such as VGA memory and ACPI tables
// TODO: make this a higher-half kernel
void PageKernel(size_t totalmem, mboot_mmap_entry_t* mmap, size_t mmapLength){
    totalPages = totalmem / PAGE_SIZE;
    if(totalmem % PAGE_SIZE != 0){
        totalPages++;
    }

    // Make sure to include the kernel's page directory in the mapping
    size_t kernelPages = ((uintptr_t)&__kernel_end - (uintptr_t)&__kernel_start) / PAGE_SIZE;
    if(((uintptr_t)&__kernel_end - (uintptr_t)&__kernel_start) % PAGE_SIZE != 0){
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
    size_t kernelSize = kernelPages * PAGE_SIZE;
    

    size_t vgaPages = (1024 * 256) / PAGE_SIZE;
    if(VGA_REGION_SIZE % PAGE_SIZE != 0){
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
        firstVgaPage[i].address = (0xA0000 + (i * PAGE_SIZE)) >> 12;
    }

    // Remap the BIOS...

    size_t acpiPages = 0;
    if(acpiInfo.exists){
        // Remap the RSDP
        page_t* rsdpPage = palloc(vgaRegion + (vgaPages * PAGE_SIZE), acpiInfo.rsdpV1, 1, &pageDir[0], false);
        if(rsdpPage == NULL){
            return;
        }
        rsdpPage->readWrite = 1;
        rsdpPage->present = 1;
        rsdpPage->user = false;
        acpiPages++;

        acpiInfo.rsdpV1 = (RSDP_V1_t*)(vgaRegion + (vgaPages * PAGE_SIZE));

        // Remap the RSDT
        page_t* rsdtPage = palloc(vgaRegion + ((vgaPages +1) * PAGE_SIZE), acpiInfo.rsdt, 1, &pageDir[0], false);
        if(rsdtPage == NULL){
            return;
        }
        rsdtPage->readWrite = 1;
        rsdtPage->present = 1;
        rsdtPage->user = false;
        acpiPages++;

        acpiInfo.rsdt = (RSDT_t*)(vgaRegion + ((vgaPages + 1) * PAGE_SIZE));

        // Remap the FADT
        page_t* fadtPage = palloc(vgaRegion + ((vgaPages + 2) * PAGE_SIZE), acpiInfo.fadt, 1, &pageDir[0], false);
        if(fadtPage == NULL){
            return;
        }
        fadtPage->readWrite = 1;
        fadtPage->present = 1;
        fadtPage->user = false;
        acpiPages++;

        acpiInfo.fadt = (FADT_t*)(vgaRegion + ((vgaPages + 2) * PAGE_SIZE));
    }

    memSize = totalmem;

    heapStart = vgaRegion + vgaPages * PAGE_SIZE;
    heapStart += PAGE_SIZE * acpiPages;      // Make sure the heap starts after the ACPI tables

    // Page the memory map to just after the kernel and framebuffer
    size_t mapPages = mmapLength / PAGE_SIZE;
    if(mmapLength % PAGE_SIZE != 0){
        mapPages++;
    }
    uintptr_t startAddr = FindUnpagedMemoryHigh(&pageDir[0]);
    page_t* firstMapPage = palloc(heapStart, startAddr, mapPages, &pageDir[0], false);

    memoryMap = (mboot_mmap_entry_t*)heapStart;

    heapStart += mapPages * PAGE_SIZE;

    startAddr = FindUnpagedMemoryHigh(&pageDir[0]);
    page_t* firstHeapPage = palloc(heapStart, startAddr, 1, &pageDir[0], false);
    if(firstHeapPage == NULL){
        WriteStr("Failed to allocate heap page\n");
    }

    // Map the rest of the memory
    //size_t remainingPages = totalPages - kernelPages - vgaPages - acpiPages - mapPages - 1;
    //uint32 lowmemPages = (uintptr_t)&__kernel_start / PAGE_SIZE;

    currentDir = &pageDir[0];

    // Enable paging
    cr3((uint32)&pageDir[0]);
    uint32 currentCr0 = 0;
    get_cr0(currentCr0);
    cr0(currentCr0 | 0x80000001);

    kernel_heap = (memory_block_t*)heapStart;
    kernel_heap->size = PAGE_SIZE;
    kernel_heap->next = NULL;
    kernel_heap->free = true;
    kernel_heap->magic = MEMBLOCK_MAGIC;
    kernel_heap_end = heapStart + PAGE_SIZE;
    KERNEL_FREE_HEAP_BEGIN = heapStart;
    KERNEL_FREE_HEAP_END = memSize;
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
    // Get the kernel's first heap memory block
    memory_block_t* current = kernel_heap;
    memory_block_t* previous = NULL;
    while(current != NULL){
        if(current->magic != MEMBLOCK_MAGIC){
            // There was a memory corruption or invalid allocation
            //WriteStr("Memory block magic number invalid, fixing\n");
            // Attempt to fix the error (may deallocate further free blocks, but if this is corrupted they're inaccessible anyways)
            current->magic = MEMBLOCK_MAGIC;
            current->next = NULL;
            current->free = false;
            // Check if there are enough allocated pages at this location
            size_t pagesToAdd = (size + MEMORY_BLOCK_SIZE) / PAGE_SIZE;
            if((size + MEMORY_BLOCK_SIZE) % PAGE_SIZE != 0){
                pagesToAdd++;
            }
            current->size = pagesToAdd * PAGE_SIZE;
            for(size_t i = 0; i < pagesToAdd; i++){
                // This should skip over pages that are already allocated
                palloc((uintptr_t)current + (i * PAGE_SIZE), FindUnpagedMemoryHigh(currentDir), 1, currentDir, false);
            }

            return (void*)((uint8*)current + MEMORY_BLOCK_SIZE);
        }
        if(current->free && current->size > size + MEMORY_BLOCK_SIZE + 1 /*Make sure the size is right and there's enough space for another memory block and at least one byte*/){
            // The block is free and large enough
            memory_block_t* newBlock = (memory_block_t*)((uint8*)current + MEMORY_BLOCK_SIZE + size);
            newBlock->size = (current->size - size) - MEMORY_BLOCK_SIZE;
            newBlock->free = true;
            newBlock->next = current->next;
            newBlock->magic = MEMBLOCK_MAGIC;
            current->size = size;
            current->free = false;
            current->next = newBlock;
            //WriteStr("Found block, split it\n");
            return (void*)((uint8*)current + MEMORY_BLOCK_SIZE);
        }else if(current->free && current->size >= size){
            // The block is free and the right size
            // If there is not enough space for another block but it's too big, just allocate the whole block
            current->free = false;
            //WriteStr("Found block, allocating whole block\n");
            return (void*)((uint8*)current + MEMORY_BLOCK_SIZE);
        }
        if(current->next == NULL && current->free && current->size < size + MEMORY_BLOCK_SIZE){
            // The current block is free and the last block, but it's not large enough
            //WriteStr("Last block is too small, allocating new block\n");
            current->size = size + MEMORY_BLOCK_SIZE;
            size_t pagesToAdd = current->size / PAGE_SIZE;
            if(current->size % PAGE_SIZE != 0){
                pagesToAdd++;
            }
            for(size_t i = 0; i < pagesToAdd; i++){
                palloc((uintptr_t)current + (i * PAGE_SIZE), FindUnpagedMemoryHigh(currentDir), 1, currentDir, false);
            }
            if(pagesToAdd * PAGE_SIZE > current->size + MEMORY_BLOCK_SIZE + 1){
                // The block is too large
                //WriteStr("Block is too large, splitting\n");
                memory_block_t* newBlock = (memory_block_t*)((uint8*)current + MEMORY_BLOCK_SIZE + size);
                newBlock->size = (pagesToAdd * PAGE_SIZE) - size - MEMORY_BLOCK_SIZE;
                newBlock->free = true;
                newBlock->next = NULL;
                newBlock->magic = MEMBLOCK_MAGIC;
                current->size = size + MEMORY_BLOCK_SIZE;
                current->next = newBlock;
            }else if(pagesToAdd * PAGE_SIZE > current->size){
                // Too big, but not enough for another block
                //WriteStr("Block is too big, allocating whole block\n");
                current->size += (pagesToAdd * PAGE_SIZE) - current->size;
            }else{
                // Just right
                //WriteStr("Block is just right\n");
                current->size = pagesToAdd * PAGE_SIZE;
                current->next = NULL;
            }
            current->free = false;
            current->magic = MEMBLOCK_MAGIC;
            kernel_heap_end += pagesToAdd * PAGE_SIZE;
            return (void*)((uint8*)current + MEMORY_BLOCK_SIZE);
        }
        previous = current;
        current = current->next;
    }

    current = previous;

    if(kernel_heap_end >= KERNEL_FREE_HEAP_END || kernel_heap_end + size + MEMORY_BLOCK_SIZE >= KERNEL_FREE_HEAP_END){
        // Out of memory
        //WriteStr("Out of memory\n");
        return NULL;
    }

    // No free memory block found, allocate a new one
    //WriteStr("No free memory block found, allocating new block\n");
    size_t pagesToAdd = (size + MEMORY_BLOCK_SIZE) / PAGE_SIZE;
    if((size + MEMORY_BLOCK_SIZE) % PAGE_SIZE != 0){
        pagesToAdd++;
    }
    for(size_t i = 0; i < pagesToAdd; i++){
        palloc(kernel_heap_end + (i * PAGE_SIZE), FindUnpagedMemoryHigh(currentDir), 1, currentDir, false);
    }
    memory_block_t* newBlock = kernel_heap_end;
    newBlock->size = size;
    newBlock->free = false;
    newBlock->next = NULL;
    current->next = newBlock;
    kernel_heap_end += pagesToAdd * PAGE_SIZE;
    return (void*)((uint8*)newBlock + MEMORY_BLOCK_SIZE);
}

// Free an allocated pointer
void dealloc(void* ptr){
    if(ptr == NULL){
        return;
    }

    memory_block_t* block = (memory_block_t*)((uintptr_t)ptr - MEMORY_BLOCK_SIZE);
    if(block->magic != MEMBLOCK_MAGIC){
        // Invalid memory block
        WriteStr("Invalid memory block, adjusting heap\n");
        eax(block);
        STOP;
    }else{
        // Free the block
        block->free = true;
    }

    // Coalesce adjacent free blocks
    memory_block_t* current = kernel_heap;
    while(current != NULL && current->next != NULL){
        if(current->free && current->next != NULL && current->next->free){
            // Coalesce the blocks
            current->size += current->next->size + MEMORY_BLOCK_SIZE;
            current->next->magic = 0;                                   // Invalidate the block, just in case
            current->next = current->next->next;
        }
        current = current->next;
    }
}
