#include <paging.h>
#include <alloc.h>
#include <acpi.h>
#include <system.h>


// This isn't used yet, must learn how to implement higher-half kernel
#define KERNEL_VIRTADDR 0xC0000000

#define TOTAL_PAGES (MAX_MEMORY_SIZE / PAGE_SIZE)    // 1048576 pages
#define TOTAL_BITS (TOTAL_PAGES / 8)                 // 131072 bytes

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
    if(lastFreeBit >= totalMemSize / PAGE_SIZE){
        // Search the whole memory bitmap in a dword-by-dword manner (32 times faster than checking individual bits)
        uint32_t* bitmap = (uint32_t*)memoryBitmap;
        for(size_t i = 0; i < TOTAL_BITS / 32; i++){
            if(bitmap[i] != 0xFFFFFFFF){
                // Find the first zero bit in the dword
                for(size_t j = 0; j < 32; j++){
                    if(!(bitmap[i] & (1 << j))){
                        lastFreeBit = i * 32 + j + 1;
                        ClearBit(lastFreeBit);
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
PAGE_RESULT palloc(virtaddr_t virt, uint32_t flags){
    physaddr_t frame = FindValidFrame();
    if(frame == INVALID_ADDRESS){
        printk("KERNEL ERROR: Failed to find a valid frame for page allocation\n");
        return INVALID_FRAME; // No valid frame found
    }

    //printk("Allocating page at 0x%x\n", frame);

    // Get the page directory index and page table index
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    //printk("Virtual address: 0x%x\n", virt);
    //printk("PD Index: %d, PT Index: %d\n", pd_idx, pt_idx);

    // Check if page directory entry is present
    if(currentPageDir[pd_idx] & PDE_FLAG_PRESENT){
        // Get the page table physical address from the page directory entry
        physaddr_t table_phys = currentPageDir[pd_idx] & 0xFFFFF000;
        //printk("Table physical address: 0x%x\n", table_phys);

        if(flags & PTE_FLAG_USER){
            currentPageDir[pd_idx] |= PDE_FLAG_USER;
        }
        
        // Access the page table
        page_table_t* pt = (page_table_t*)table_phys;

        if(pt->pages[pt_idx] & PTE_FLAG_PRESENT){
            // Page already allocated!
            printk("Page present at virtual address 0x%x\n", virt);
            return PAGE_NOT_AQUIRED;
        }
        
        // Set the page table entry
        pt->pages[pt_idx] = (frame & 0xFFFFF000) | flags;

        asm volatile("invlpg (%0)" :: "r" (virt) : "memory");

        totalPages++;

        ClearBit(frame / PAGE_SIZE);

        //printk("Allocated page at 0x%x\n", frame);
        //printk("Page virtual address: 0x%x\n", virt);

        return PAGE_AQUIRED;
    }else{
        // Allocate a new page table...
        //printk("Page directory entry not present for virtual address 0x%x\n", virt);
        return PAGE_NOT_AQUIRED;
    }
}

extern uintptr_t heapEnd;

// Free a page at the specified virtual address, avoiding kernel memory
// Takes the virtual address so that the page can be freed regardless of the physical address (better for getting memory near the kernel)
PAGE_RESULT user_pfree(virtaddr_t virt){
    if(virt % PAGE_SIZE != 0 || (virt >= (uintptr_t)&__kernel_start && virt <= heapEnd)){
        return INVALID_FRAME; // Not page-aligned or requested address was inside the kernel
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

    return PAGE_FREED;
}

PAGE_RESULT pfree(virtaddr_t virt){
    if(virt % PAGE_SIZE != 0 || (virt >= (uintptr_t)&__kernel_start && virt <= (uintptr_t)&__kernel_end)){
        return INVALID_FRAME; // Not page-aligned or requested address was inside the kernel
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

    return PAGE_FREED;
}

// Allocate a page at the specified physical and virtual address
PAGE_RESULT physpalloc(physaddr_t phys, virtaddr_t virt, uint32_t flags) {
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
        int result = physpalloc(i, i, PTE_FLAG_PRESENT | PTE_FLAG_RW);

        if(result == -1){
            printk("KERNEL PANIC: Failed to map kernel page at 0x%x\n", i);
            STOP
        }
    }

    // Identity map the VGA framebuffer to its physical address
    size_t vgaPages = (1024 * 256) / PAGE_SIZE;
    if(vgaPages * PAGE_SIZE < 1024 * 256){
        vgaPages++;
    }

    // Get the first free page of memory after the kernel (Add a page of padding)
    uintptr_t firstVgaPage = (((uintptr_t)&__kernel_end) + PAGE_SIZE) & 0xFFFFF000;
    uint32_t offset = 0;

    for(size_t i = 0xA0000; i < 0xA0000 + vgaPages * PAGE_SIZE; i += PAGE_SIZE){
        // Allocate the VGA pages as write-through pages that user-mode can access
        int result = physpalloc(i, firstVgaPage + offset, (PTE_FLAG_PRESENT | PTE_FLAG_RW | PTE_FLAG_WRITE_THROUGH | PTE_FLAG_CACHE_DISABLED | PTE_FLAG_GLOBAL | PTE_FLAG_USER));

        if(result == -1){
            printk("KERNEL PANIC: Failed to map VGA page at 0x%x\n", i);
            STOP
        }

        offset += PAGE_SIZE;
    }

    // Remap ACPI tables
    heapStart = firstVgaPage + offset + PAGE_SIZE;
    virtaddr_t acpiStart = heapStart;
    size_t acpiPages = 0;
    if(acpiInfo.exists){
        // Remap RSDP
        physaddr_t rsdpAddr = (physaddr_t)acpiInfo.rsdp.rsdpV1;
        // Align the rsdp address to page boundaries and get the offset from alignment
        physaddr_t rsdpAligned = (rsdpAddr & 0xFFFFF000);
        uint32_t rsdpOffset = rsdpAddr - rsdpAligned;
        
        virtaddr_t rsdpVirt = acpiStart + acpiPages * PAGE_SIZE;
        for(size_t i = 0; i < sizeof(RSDP_V2_t); i += PAGE_SIZE){
            int result = physpalloc(rsdpAligned + i, rsdpVirt + i, PTE_FLAG_PRESENT | PTE_FLAG_RW | PTE_FLAG_USER);
            if(result == -1){
                printk("KERNEL PANIC: Failed to map RSDP page at 0x%x\n", rsdpAligned + i);
                STOP
            }
            acpiPages++;
        }
        // Update the RSDP pointer with the virtual address + offset
        acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)(rsdpVirt + rsdpOffset);

        // Remap the RSDT
        physaddr_t rsdtAddr = (physaddr_t)acpiInfo.rsdt.rsdt;
        physaddr_t rsdtAligned = (rsdtAddr & 0xFFFFF000);
        uint32_t rsdtOffset = rsdtAddr - rsdtAligned;
        
        virtaddr_t rsdtVirt = acpiStart + acpiPages * PAGE_SIZE;
        for(size_t i = 0; i < acpiInfo.rsdt.rsdt->header.length; i += PAGE_SIZE){
            int result = physpalloc(rsdtAligned + i, rsdtVirt + i, PTE_FLAG_PRESENT | PTE_FLAG_RW | PTE_FLAG_USER);
            if(result == -1){
                printk("KERNEL PANIC: Failed to map RSDT page at 0x%x\n", rsdtAligned + i);
                STOP
            }
            acpiPages++;
        }
        // Update the RSDT pointer with the virtual address + offset
        acpiInfo.rsdt.rsdt = (RSDT_t*)(rsdtVirt + rsdtOffset);

        // Remap the FADT
        physaddr_t fadtAddr = (physaddr_t)acpiInfo.fadt;
        physaddr_t fadtAligned = (fadtAddr & 0xFFFFF000);
        uint32_t fadtOffset = fadtAddr - fadtAligned;
        
        virtaddr_t fadtVirt = acpiStart + acpiPages * PAGE_SIZE;
        for(size_t i = 0; i < acpiInfo.fadt->header.length; i += PAGE_SIZE){
            int result = physpalloc(fadtAligned + i, fadtVirt + i, PTE_FLAG_PRESENT | PTE_FLAG_RW | PTE_FLAG_USER);
            if(result == -1){
                printk("KERNEL PANIC: Failed to map FADT page at 0x%x\n", fadtAligned + i);
                STOP
            }
            acpiPages++;
        }
        // Update the FADT pointer with the virtual address + offset
        acpiInfo.fadt = (FADT_t*)(fadtVirt + fadtOffset);
    }

    heapStart += acpiPages * PAGE_SIZE;

    // Add a page of padding to prevent underflows into the framebuffer
    heapStart += PAGE_SIZE;

    printk("Allocating heap pages...\n");

    // Map the first page of the heap
    if(palloc(heapStart, PTE_FLAG_PRESENT | PTE_FLAG_RW) == -1){
        printk("KERNEL PANIC: Failed to allocate heap start page at 0x%x\n", heapStart);
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
}