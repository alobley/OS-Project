#include <memmanage.h>
#include <vga.h>
#include <util.h>


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

typedef struct memory_region {
    size_t size;
    void* start;
    struct memory_region* next;
} memregion_t;

size_t memSize = 0;
void* vgaRegion = NULL;
uintptr_t heapStart = 0;
size_t totalPages = 0;

PageDirectoryEntry ALIGNED(4096) pageDir[1024] = {0};
PageTable ALIGNED(4096) pageTables[1024] = {0};

PageDirectory* currentDir;

// Activate a set of new pages in a page directory.
page_t* palloc(uintptr_t virtualAddr, uintptr_t physicalAddr, size_t pagesToAdd, PageDirectory* pageDir, bool user){
    // Get the page directory entry
    if (virtualAddr % 4096 != 0 || physicalAddr % 4096 != 0) {
        return NULL; // Addresses must be 4KiB aligned
    }
    page_t* firstPage = NULL;
    for(size_t i = 0; i < pagesToAdd; i++){
        PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr + (i * 4096))];
        PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
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
    return firstPage;
}

void pfree(uintptr_t virtualAddr, PageDirectory* pageDir){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
    if(table->entries[PTI(virtualAddr)].present){
        table->entries[PTI(virtualAddr)].present = 0;
        table->entries[PTI(virtualAddr)].address = 0;
        numPages--;
    }
}

// Allocate a new page
void AllocatePage(uintptr_t virtualAddr, PageDirectory* pageDir, bool user){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
    if(!dirEntry->present){
        dirEntry->address = (uint32)(&pageTables[PTI(virtualAddr)]) >> 12;
        dirEntry->present = 1;
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

void* GetVgaRegion(){
    return (void*)vgaRegion;
}

size_t GetPages(){
    return numPages;
}

size_t GetTotalMemory(){
    return memSize;
}

// This is important for the kernel heap, and will also page important memory regions such as VGA memory and ACPI tables
// TODO: make this a higher-half kernel
void PageKernel(size_t totalmem){
    totalPages = totalmem / 4096;
    if(totalmem % 4096 != 0){
        totalPages++;
    }

    // Make sure to include the kernel's page directory in the mapping
    size_t kernelPages = (&__kernel_end - &__kernel_start) / 4096;
    if((&__kernel_end - &__kernel_start) % 4096 != 0){
        kernelPages++;
    }

    // Get the page directory and assign its virtual address to its physical address
    AllocatePageDirectory(&pageDir[0], &pageDir[0], false);

    //palloc(0, 0, totalPages, &pageDir[0], false);
    //return;

    // Set the kernel's pages to active (virtual address will change later but for now identity map it)
    page_t* firstKernelPage = palloc(&__kernel_start, &__kernel_start, kernelPages, &pageDir[0], false);
    if(firstKernelPage == NULL){
        return;
    }

    // Map VGA memory to right after the kernel memory
    size_t vgaSize = VGA_PIXEL_MODE_SIZE + VGA_TEXT_MODE_SIZE;
    size_t kernelSize = kernelPages * 4096;
    
    size_t vgaPages = vgaSize / 4096;
    if(vgaSize % 4096 != 0){
        vgaPages++;
    }
    
    vgaRegion = (uint32)&__kernel_start + kernelSize;
    page_t* firstVgaPage = palloc(vgaRegion, 0xA0000, vgaPages, &pageDir[0], false);

    for(size_t i = 0; i < vgaPages; i++){
        // The framebuffer is an MMIO region, so it must be write-through
        firstVgaPage[i].writeThrough = 1;
        firstVgaPage[i].cacheDisabled = 1;
        firstVgaPage[i].readWrite = 1;
        firstVgaPage[i].present = 1;
        firstVgaPage[i].user = false;
    }

    // Remap the BIOS...
    // Remap the ACPI tables...

    memSize = totalmem;

    heapStart = vgaRegion + vgaSize;

    // Map the rest of the memory
    size_t remainingPages = totalPages - kernelPages - vgaPages;
    uint32 lowmemPages = (uint32)&__kernel_start / 4096;

    // Enable paging
    cr3((uint32)&pageDir[0]);
    uint32 currentCr0 = 0;
    get_cr0(currentCr0);
    cr0(currentCr0 | 0x80000001);
}

memregion_t* kheapRegion = NULL;
memregion_t* userHeaps = NULL;


void* alloc(size_t size){
    // Search for an unpaged region of memory
}

void dealloc(void* ptr){
    // Find the region of memory that contains the pointer
    // Free the memory
}
