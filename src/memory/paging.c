#include <paging.h>

#define TOTAL_PAGES (0xFFFFFFFF / PAGE_SIZE)    // 1048576 pages
#define TOTAL_BITS (TOTAL_PAGES / 8)            // 131072 bytes

size_t totalMemSize = 0;

size_t totalPages = 0;

size_t lastFreeBit = 0;

// Define a memory bitmap of 4KiB pages - include every page up to 4GiB
// This defines usable memory for the OS to page, but the page allocation doesn't change it
char memoryBitmap[TOTAL_BITS] ALIGNED(4096) = {0};

// Set a bit in the memory bitmap
void SetBit(uint32_t bit){
    memoryBitmap[bit / 8] |= (1 << (bit % 8));
}

// Clear a bit in the memory bitmap
void ClearBit(uint32_t bit){
    memoryBitmap[bit / 8] &= ~(1 << (bit % 8));
}

// Check if a bit is set in the memory bitmap
bool TestBit(uint32_t bit){
    return memoryBitmap[bit / 8] & (1 << (bit % 8));
}

// Map the bitmap using the memory map. This allows the OS to know which pages are usable
void MapBitmap(uint32_t memSize, mmap_entry_t* mmap, size_t mmapLength /* In total entries */){
    size_t pages = memSize / 4096;
    for(size_t i = 0; i < pages; i++){
        // Check if the page is usable using the memory map
        bool usable = false;
        for(size_t j = 0; j < mmapLength; j++){
            mmap_entry_t* entry = &mmap[j];
            if(entry->type == 1 && i >= entry->base_addr / PAGE_SIZE && i < (entry->base_addr + entry->length) / 4096){
                usable = true;
                break;
            }
        }

        if(usable){
            SetBit(i);
        }else{
            ClearBit(i);
        }
    }
}

// Get the page directory index from a virtual address
#define PD_INDEX(addr) ((addr) >> 22)

// Get the page table index from a virtual address
#define PT_INDEX(addr) (((addr) >> 12) & 0x3FF)

// Extract the physical address from a frame
#define GET_PHYSICAL_ADDR(frame) (frame & 0xFFFFF000)

// Align the frame to a page boundary
#define SET_FRAME(physaddr) (physaddr & 0xFFFFF000)

pde_t kernelPageDirectory[1024] ALIGNED(4096);
page_table_t kernelPageTables[1024] ALIGNED(4096);

pde_t* currentPageDir = &kernelPageDirectory[0];
page_table_t* currentPageTables = &kernelPageTables[0];

// Find a valid page frame in the memory bitmap (TODO: Implement a better algorithm, this one is crazy bad)
physaddr_t FindValidFrame(){
    while(!TestBit(lastFreeBit) && lastFreeBit < TOTAL_PAGES){
        // Skip to the next free bit if this one is unavailable
        lastFreeBit++;
    }
    if(lastFreeBit >= TOTAL_PAGES){
        // Search the whole memory bitmap for a free page if we reach the end
        for(size_t i = 0; i < totalMemSize / PAGE_SIZE; i++){
            virtaddr_t addr = i * PAGE_SIZE;
            if(TestBit(i)){
                uint32_t pd_idx = PD_INDEX(addr);
                uint32_t pt_idx = PT_INDEX(addr);
                
                // Check if page directory entry is present
                if(currentPageDir[pd_idx] & PDE_FLAG_PRESENT){
                    physaddr_t table = currentPageDir[pd_idx] & 0xFFFFF000;
                    page_table_t* pt = (page_table_t*)table;
                    if(pt->pages[pt_idx] & PTE_FLAG_PRESENT){
                        continue;
                    }else{
                        return i * PAGE_SIZE;
                    }
                }
            }
        }
    }else{
        // We found a free page with the lookup
        lastFreeBit++;
        return lastFreeBit * PAGE_SIZE;
    }

    // Return something that isn't page aligned
    return INVALID_ADDRESS;
}

// Allocate a page at the specified virtual address
int palloc(virtaddr_t virt, uint32_t flags){
    physaddr_t frame = FindValidFrame();
    if(frame == INVALID_ADDRESS){
        return -1;
    }

    // Set the page table entry
    physaddr_t table = currentPageDir[PD_INDEX(virt)] & 0xFFFFF000;
    if(table & PDE_FLAG_PRESENT){
        page_table_t* pt = (page_table_t*)table;
        pt->pages[PT_INDEX(virt)] = (frame & 0xFFFFF000) | flags;

        asm volatile("invlpg (%0)" :: "r" (frame) : "memory");

        totalPages++;

        return 1;
    }else{
        // Allocate a new page table...
        return -1;
    }
}

void pfree(virtaddr_t virt){
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    // Check if page directory entry is present
    if(currentPageDir[pd_idx] & PDE_FLAG_PRESENT){
        physaddr_t table = currentPageDir[pd_idx] & 0xFFFFF000;
        page_table_t* pt = (page_table_t*)table;
        if(pt->pages[pt_idx] & PTE_FLAG_PRESENT){
            pt->pages[pt_idx] = 0;
            totalPages--;
        }

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");
    }
}

// Allocate a page at the specified physical and virtual address
int physpalloc(physaddr_t phys, virtaddr_t virt, uint32_t flags) {
    uint32_t pdi = PD_INDEX(virt);
    uint32_t pti = PT_INDEX(virt);

    // Check if page directory entry is present
    if(currentPageDir[pdi] & PDE_FLAG_PRESENT) {
        physaddr_t table = currentPageDir[pdi] & 0xFFFFF000;
        page_table_t* pt = (page_table_t*)table;
        pt->pages[pti] = (phys & 0xFFFFF000) | flags;

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");

        totalPages++;

        return 1;
    }else{
        // Allocate a new page table...
        return -1;
    }
}

void ConstructPageDirectory(pde_t* pageDirectory, page_table_t* pageTables){
    memset(pageDirectory, 0, sizeof(pde_t) * 1024);
    memset(pageTables, 0, sizeof(page_table_t) * 1024);

    for(int i = 0; i < 1024; i++){
        pageDirectory[i] = (uint32_t)(((physaddr_t)&pageTables[i]) | (PDE_FLAG_PRESENT | PDE_FLAG_RW));
    }
}

void PageKernel(size_t memSize){
    printf("Paging memory...\n");
    totalMemSize = memSize;
    ConstructPageDirectory(currentPageDir, currentPageTables);
    // Identity map the kernel to its physical address (The kernel's start address should already be aligned)
    for(size_t i = (uint32_t)&__kernel_start; i < ((uint32_t)&__kernel_end) + PAGE_SIZE; i += PAGE_SIZE){
        int result = physpalloc(i, i, 0U | (PTE_FLAG_PRESENT | PTE_FLAG_RW));

        if(result == -1){
            printf("KERNEL PANIC: Failed to map kernel page at 0x%x\n", i);
            STOP
        }
    }

    // Identity map the VGA framebuffer to its physical address
    for(size_t i = 0xA0000; i < 0xC0000; i += PAGE_SIZE){
        int result = physpalloc(i, i, 0U | (PTE_FLAG_PRESENT | PTE_FLAG_RW));

        if(result == -1){
            printf("KERNEL PANIC: Failed to map VGA framebuffer page at 0x%x\n", i);
            STOP
        }
    }

    // Enable paging
    asm volatile("mov %0, %%cr3" :: "r" (currentPageDir));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 = 0x80000001;
    asm volatile("mov %0, %%cr0" :: "r" (cr0));
}