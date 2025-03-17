#include <paging.h>
#include <alloc.h>
#include <acpi.h>
#include <system.h>

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

// Find a valid page frame in the memory bitmap
physaddr_t FindValidFrame(){
    while(!TestBit(lastFreeBit) && lastFreeBit < TOTAL_PAGES){
        // Skip to the next free bit if this one is unavailable
        lastFreeBit++;
    }
    if(lastFreeBit >= TOTAL_PAGES){
        // Search the whole memory bitmap in a dword-by-dword manner (32 times faster than checking individual bits)
        uint32_t* bitmap = (uint32_t*)memoryBitmap;
        for(size_t i = 0; i < TOTAL_BITS / 32; i++){
            if(bitmap[i] != 0xFFFFFFFF){
                // Find the first zero bit in the dword
                for(size_t j = 0; j < 32; j++){
                    if(!(bitmap[i] & (1 << j))){
                        lastFreeBit = i * 32 + j;
                        return lastFreeBit * PAGE_SIZE;
                    }
                }
            }
        }
    }else{
        // We found a free page with the lookup
        physaddr_t frame = lastFreeBit * PAGE_SIZE;
        lastFreeBit++;
        return frame;
    }

    // Return something that isn't page aligned
    return INVALID_ADDRESS;
}

// Allocate a page at the specified virtual address
int palloc(virtaddr_t virt, uint32_t flags){
    physaddr_t frame = FindValidFrame();
    if(frame == INVALID_ADDRESS){
        printf("KERNEL PANIC: Failed to find a valid frame for page allocation\n");
        return -1;
    }

    //printf("Allocating page at 0x%x\n", frame);

    // Set the page table entry
    physaddr_t table = currentPageDir[PD_INDEX(virt)] & 0xFFFFF000;
    pde_t* pd_entry = (pde_t*)table;
    if(*pd_entry & PDE_FLAG_PRESENT){
        page_table_t* pt = (page_table_t*)((physaddr_t)pd_entry & 0xFFFFF000);
        pt->pages[PT_INDEX(virt)] = (frame & 0xFFFFF000) | flags;

        asm volatile("invlpg (%0)" :: "r" (frame) : "memory");

        totalPages++;

        ClearBit(frame / PAGE_SIZE);

        //printf("Allocated page at 0x%x\n", frame);
        //printf("Page virtual address: 0x%x\n", virt);

        return 1;
    }else{
        // Allocate a new page table...
        return -1;
    }
}

extern uintptr_t heapEnd;

// Free a page at the specified virtual address, avoiding kernel memory
// Takes the virtual address so that the page can be freed regardless of the physical address (better for getting memory near the kernel)
int user_pfree(virtaddr_t virt){
    if(virt % PAGE_SIZE != 0 || (virt >= (uintptr_t)&__kernel_start && virt <= heapEnd)){
        return STANDARD_FAILURE; // Not page-aligned or requested address was inside the kernel
    }
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

        SetBit(virt / PAGE_SIZE);

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");
    }

    return STANDARD_SUCCESS;
}

int pfree(virtaddr_t virt){
    if(virt % PAGE_SIZE != 0 || (virt >= (uintptr_t)&__kernel_start && virt <= (uintptr_t)&__kernel_end)){
        return STANDARD_FAILURE; // Not page-aligned or requested address was inside the kernel
    }
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

        SetBit(virt / PAGE_SIZE);

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");
    }

    return STANDARD_SUCCESS;
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

        ClearBit(phys / PAGE_SIZE);

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");

        totalPages++;

        return STANDARD_SUCCESS;
    }else{
        return STANDARD_FAILURE;
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
    size_t vgaPages = (1024 * 256) / PAGE_SIZE;
    if(vgaPages * PAGE_SIZE < 1024 * 256){
        vgaPages++;
    }

    // Get the first free page of memory after the kernel
    uintptr_t firstVgaPage = (((uintptr_t)&__kernel_end) + PAGE_SIZE) & 0xFFFFF000;
    uint32_t offset = 0;

    for(size_t i = 0xA0000; i < 0xA0000 + vgaPages * PAGE_SIZE; i += PAGE_SIZE){
        // Allocate the VGA pages as write-through pages that user-mode can access
        int result = physpalloc(i, firstVgaPage + offset, (PTE_FLAG_PRESENT | PTE_FLAG_RW | PTE_FLAG_WRITE_THROUGH | PTE_FLAG_CACHE_DISABLED | PTE_FLAG_GLOBAL | PTE_FLAG_USER));

        if(result == -1){
            printf("KERNEL PANIC: Failed to map VGA page at 0x%x\n", i);
            STOP
        }

        offset += PAGE_SIZE;
    }

    // Remap ACPI tables
    heapStart = firstVgaPage + offset + PAGE_SIZE;
    size_t acpiPages = 0;
    if(acpiInfo.exists){
        // Remap RSDP
        int result = physpalloc((physaddr_t)acpiInfo.rsdp.rsdpV1, heapStart, (PTE_FLAG_PRESENT | PTE_FLAG_RW));
        if(result == -1){
            printf("KERNEL PANIC: Failed to map ACPI RSDP at 0x%x\n", (physaddr_t)acpiInfo.rsdp.rsdpV1);
            STOP
        }
        acpiPages++;

        acpiInfo.rsdp.rsdpV1 = (RSDP_V1_t*)heapStart;

        // Remap RSDT
        heapStart += PAGE_SIZE;
        result = physpalloc((physaddr_t)acpiInfo.rsdt.rsdt, heapStart + PAGE_SIZE, (PTE_FLAG_PRESENT | PTE_FLAG_RW));
        if(result == -1){
            printf("KERNEL PANIC: Failed to map ACPI RSDT at 0x%x\n", (physaddr_t)acpiInfo.rsdt.rsdt);
            STOP
        }

        acpiPages++;
        acpiInfo.rsdt.rsdt = (RSDT_t*)(heapStart + PAGE_SIZE);

        // Remap FADT
        heapStart += PAGE_SIZE;
        result = physpalloc((physaddr_t)acpiInfo.fadt, heapStart + PAGE_SIZE, (PTE_FLAG_PRESENT | PTE_FLAG_RW));
        if(result == -1){
            printf("KERNEL PANIC: Failed to map ACPI FADT at 0x%x\n", (physaddr_t)acpiInfo.fadt);
            STOP
        }

        acpiPages++;
        acpiInfo.fadt = (FADT_t*)(heapStart + PAGE_SIZE);
    }

    heapStart += PAGE_SIZE;

    // Map the first page of the heap
    if(palloc(heapStart, PTE_FLAG_PRESENT | PTE_FLAG_RW) == -1){
        printf("KERNEL PANIC: Failed to allocate heap start page at 0x%x\n", heapStart);
        STOP
    }

    // Map MMIO and other things...

    // Enable paging
    asm volatile("mov %0, %%cr3" :: "r" (currentPageDir));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 = 0x80000001;
    asm volatile("mov %0, %%cr0" :: "r" (cr0));

    RemapVGA(firstVgaPage);                                                 // Move the VGA framebuffer pointer to the new location

    // Allocate one page at the end of all the other data for the beginning of the kernel's heap
    if(palloc(heapStart, PTE_FLAG_PRESENT | PTE_FLAG_RW) == -1){
        printf("KERNEL PANIC: Failed to allocate heap start page at 0x%x\n", heapStart);
        STOP

    }
}