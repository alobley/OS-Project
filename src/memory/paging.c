/***********************************************************************************************************************************************************************
 * Copyright (c) 2025, xWatexx. All rights reserved.
 * This software is licensed under the MIT license. See the LICENSE file in the root directory for more details.
 * 
 * This file is part of Dedication OS.
 * This file contains the code and core components of the paging subsystem for Dedication OS. Most virtual memory management is performed here.
 ***********************************************************************************************************************************************************************/
#include <paging.h>
#include <alloc.h>
#include <system.h>
#include <console.h>
#include <acpi.h>

// The total amount of system RAM in bytes
size_t totalMemSize;

// The total number of mapped pages (including those not currently in the page directory)
size_t mappedPages;

physaddr_t lastPhysicalAddress = 0;

// Page table structure (size is one page each)
struct ALIGNED(PAGE_SIZE) Page_Table {
    page_t pages[1024];
};

// Page directory structure (size is one page)
struct ALIGNED(PAGE_SIZE) Page_Directory {
    pde_t tables[1024];
};

// Helper macros for translating a virtual address
#define PDE_INDEX(virt) ((virt >> 22) & 0x3FF)              // Index in a page directory
#define PTE_INDEX(virt) ((virt >> 12) & 0x3FF)              // Index in a page table
#define PAGE_OFFSET(virt) (virt & 0xFFF)                    // Just the offset within a page - not particularly important

// Reconstruct a virtual address from the page directory index, page table index, and offset
#define ReconstructVirtualAddress(pdIndex, ptIndex, offset) \
    ((pdIndex << 22) | (ptIndex << 12) | offset)

// Macros for interacting with the control registers to enable paging

// Invalidate a TLB entry
#define invlpg(x) asm volatile("invlpg (%0)" :: "r" (x) : "memory")

// Set the PE bit in CR0
#define EnablePaging()              \
    asm volatile (                  \
        "mov %%cr0, %%eax\n\t"      \
        "or $0x80000000, %%eax\n\t" \
        "mov %%eax, %%cr0"          \
        ::: "eax")

#define DisablePaging()             \
    asm volatile (                  \
        "mov %%cr0, %%eax\n\t"      \
        "and $0x7FFFFFFF, %%eax\n\t"\
        "mov %%eax, %%cr0"          \
        ::: "eax")

// Load CR3 with a pointer (physical address) to a page directory
#define LoadPageDirectory(dir) asm volatile("mov %0, %%cr3" :: "r"(dir))

#define PAGE_DIR_VIRTUAL ((struct Page_Directory*) PD_VIRTADDR)
#define PAGE_TABLE_VIRTUAL(pdIndex)  ((struct Page_Table*)(PT_VIRTADDR + (pdIndex * PAGE_SIZE)))

// Define a stack of free page frames for extremely easy page frame allocation (this uses a whopping 4MB of memory... wow)
ALIGNED(PAGE_SIZE) page_t _frameStack[MAX_PAGES] = {0};
page_t* frameSp = &_frameStack[0];

// Define a bitmap to keep track of available page frames
ALIGNED(PAGE_SIZE) unsigned char memoryBitmap[TOTAL_BITS / BITS_PER_BITMAP_ENTRY] = {0};

/// @brief Take a virtual address and return its actual address in physical memory (does not require alignment)
/// @param virt The virtual address to convert
/// @return The physical address of the memory
physaddr_t GetPhysicalAddress(virtaddr_t virt){
    // Get the nececarry offsets
    virtaddr_t offset = PAGE_OFFSET(virt);
    index_t pdIndex = PDE_INDEX(virt);
    index_t ptIndex = PTE_INDEX(virt);

    // Get the page value (we can skip the page directory itself but we still need the PD index to find the correct page table)
    page_t page = PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex];

    // Extract the physical address of the page
    physaddr_t addr = PAGE_ADDR_MASK(page);
    addr += offset;                                         // Add the offset within the page so aligned addresses aren't required

    return addr;
}

/// @brief Set a bit in the memory bitmap to 1
/// @param bit The index of the bit to set
void SetBit(index_t bit){
    memoryBitmap[bit / BITS_PER_BITMAP_ENTRY] |= (1 << (bit % BITS_PER_BITMAP_ENTRY));
}

/// @brief Clear a bit in the memory bitmap
/// @param bit The index of the bit to clear
void ClearBit(index_t bit){
    memoryBitmap[bit / BITS_PER_BITMAP_ENTRY] &= ~(1 << (bit % BITS_PER_BITMAP_ENTRY));
}

/// @brief Check if a bit in the bitmap has been set or not
/// @param bit The index of the bit to check
/// @return True or false, based on whether the bit is set or not
bool TestBit(index_t bit){
    return memoryBitmap[bit / BITS_PER_BITMAP_ENTRY] & (1 << (bit % BITS_PER_BITMAP_ENTRY));
}

// Flag for determining whether the stack is empty
bool stackEmpty = false;

/// @brief Clear the page frame stack - should be done upon a physpalloc. This should happen very rarely.
void FlushFrameStack(){
    memset(&_frameStack[0], 0, MAX_PAGES);
    frameSp = &_frameStack[0];
    stackEmpty = true;
}

/// @brief After a page was freed, its frame can be reused by another page allocation
/// @param physicalAddress The physical address of the freed page. Must be page-aligned.
bool PushFrame(physaddr_t physicalAddress){
    if(frameSp > &_frameStack[MAX_PAGES - 1]){
        // This should never happen (and if coded correctly, it won't ever happen), but just in case.
        printk("CRITICAL ERROR: FRAME STACK OVERFLOW! THIS IS A KERNEL BUG.\nPLEASE REPORT THIS TO THE DEVELOPER.\n");
        STOP
    }

    if(PAGE_ADDR_MASK(physicalAddress) != physicalAddress){
        return false;
    }

    if(frameSp < &_frameStack[0]){
        // The stack is empty, reset the pointer
        frameSp = &_frameStack[0];
    }

    *frameSp = physicalAddress;
    frameSp++;
    stackEmpty = false;
    return true;
}

/// @brief Pop a page frame from the stack for use. 
/// @warning DO NOT DISCARD A POPPED FRAME
/// @return The frame which can be used
physaddr_t PopFrame(){
    if(frameSp < &_frameStack[0]){
        // Out of memory!
        return INVALID_ADDRESS;
    }

    if(PAGE_ADDR_MASK(*frameSp) != *frameSp){
        printk("CRITICAL ERROR: INVALID FRAME DETECTED! THIS IS A KERNEL BUG.\n");
        printk("PLEASE REPORT TO THE DEVELOPER ASAP.\n");
        STOP
    }
    while(!TestBit(*frameSp / PAGE_SIZE)){
        // Just in case the frame was incorrectly pushed or its status changed, skip it.
        // Changes to the memory bitmap should be exceedingly rare after the kernel maps itself.
        if(frameSp <= &_frameStack[0]){
            // Out of memory!
            return INVALID_ADDRESS;
        }
        frameSp--;
    }

    physaddr_t frame = *frameSp;
    frameSp--;
    return frame;
}

/// @brief Read the memory bitmap to build the page frame stack
void MapFrameStack(){
    frameSp = &_frameStack[0];
    for(index_t i = 0; i < MAX_PAGES; i++){
        if(TestBit(i)){
            if(!PushFrame(i * PAGE_SIZE)){
                printk("ERROR: Kernel bug when mapping the frame stack!\n");
                STOP;
            }
        }
    }

    physaddr_t frame = PopFrame();
    while(frame == 0 || frame > lastPhysicalAddress - PAGE_SIZE){
        // Skip zero frames and invalid frames
        frame = PopFrame();
    }

    if(frame != 0 && frame != INVALID_ADDRESS && (frame & 0xFFF) == 0){
        // Push the frame back onto the stack
        PushFrame(frame);
    }
}

/// @brief Map the memory bitmap based on the multiboot memory map
/// @param memoryMap The array of multiboot memory map entries
/// @return Success or error code
void MapBitmap(mmap_entry_t* memoryMap, size_t memmapLength){
    // Clear the bitmap
    memset(memoryBitmap, 0, sizeof(memoryBitmap));

    // Get the actual end of usable RAM based on the memory map (the provided numbers by GRUB are usually not accurate)
    physaddr_t highestUsableAddr = 0;
    for (size_t j = 0; j < memmapLength; j++) {
        if (memoryMap[j].type == MEMTYPE_USABLE) {
            physaddr_t top = memoryMap[j].base_addr + memoryMap[j].length;
            if (top > highestUsableAddr) {
                highestUsableAddr = top;
            }
        }
    }

    lastPhysicalAddress = highestUsableAddr;

    // Get the total number of pages in the system
    size_t pages = highestUsableAddr / PAGE_SIZE;
    for(size_t i = 0; i < pages; i++){
        bool usable = false;
        for(size_t j = 0; j < memmapLength; j++){
            mmap_entry_t* entry = &memoryMap[j];
            if(entry->type == MEMTYPE_USABLE && i >= entry->base_addr / PAGE_SIZE && i < (entry->base_addr + entry->length) / PAGE_SIZE){
                usable = true;
                break;
            }
        }
        if(usable){
            // Set the bit in the bitmap and push its frame onto the page frame stack
            SetBit(i);
        }
    }

    MapFrameStack();
}

/// @brief Get a physical address that is page-aligned and contains enough memory to contain a single page table and then add it to the page directory.
/// @warning ENSURE that either the current PDE at that address is saved freed. This function overwrites its PDE and does not push its frame.
/// @return Success or error code
page_result_t AllocatePageTable(virtaddr_t addr){
    index_t pdIndex = PDE_INDEX(addr);
    index_t ptIndex = PTE_INDEX(addr);

    physaddr_t pageTable = PopFrame();
    if(pageTable == INVALID_ADDRESS){
        // The frame can be safely discarded here as it was invalid anyway
        return NO_MORE_MEMORY;
    }

    ClearBit(pageTable / PAGE_SIZE);

    // Page permissions and flags can be on a per-page basis, so this should be fine.
    PAGE_DIR_VIRTUAL->tables[pdIndex] = PAGE_ADDR_MASK(pageTable) | DEFAULT_USER_PDE_FLAGS;

    memset(PAGE_TABLE_VIRTUAL(pdIndex), 0, PAGE_SIZE); // Clear the page table
    return PAGE_OK;
}

/// @brief Release an allocated page table
/// @param table An actual pointer to the virtual address of the table (this should be easy to get)
/// @return Success or error code
page_result_t FreePageTable(struct Page_Table* table){
    index_t pdIndex = PDE_INDEX((virtaddr_t)table);
    index_t ptIndex = PTE_INDEX((virtaddr_t)table);

    if(!(PAGE_DIR_VIRTUAL->tables[pdIndex] & PDE_PRESENT)){
        // No page table for this virtual address
        return PAGE_NOT_PRESENT;
    }

    if(!(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] & PDE_PRESENT)){
        // No page table for this virtual address
        return PAGE_NOT_PRESENT;
    }

    // Free the page table
    physaddr_t frame = PAGE_ADDR_MASK(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex]);
    SetBit(frame / PAGE_SIZE);
    PushFrame(frame);

    // Remove the page table from the directory
    PAGE_DIR_VIRTUAL->tables[pdIndex] = 0;
    mappedPages--;
    return PAGE_OK;
}

/// @brief Allocate a page with a given virtual address
/// @param address The address of the new page (must be aligned)
/// @return Success or error code
page_result_t palloc(virtaddr_t address, unsigned int flags){
    if(PAGE_ADDR_MASK(address) != address){
        // Unaligned address!
        return ADDRESS_NOT_ALIGNED;
    }
    index_t pdIndex = PDE_INDEX(address);
    index_t ptIndex = PTE_INDEX(address);

    if(!(PAGE_DIR_VIRTUAL->tables[pdIndex] & PDE_PRESENT)){
        // No page table for this virtual address, allocate one
        if(AllocatePageTable(address) != PAGE_OK){
            // Most likely out of memory
            return NO_MORE_MEMORY;
        }
    }

    if(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] & PAGE_PRESENT){
        // This page is already mapped!
        return PAGE_ALREADY_MAPPED;
    }

    // Get the frame to allocate
    physaddr_t frame = PopFrame();
    if(frame == INVALID_ADDRESS){
        // If there was an invalid address, it's likely there's no more memory
        return NO_MORE_MEMORY;
    }
    ClearBit(frame / PAGE_SIZE);
    // If we make it here, the page shall be allocated
    PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] = PAGE_ADDR_MASK(frame) | flags;
    invlpg(PAGE_ADDR_MASK(address));
    mappedPages++;
    return PAGE_OK;
}

/// @brief Remap a page from an old virtual address to a new virtual address
/// @param old The old (page-aligned) virtual address
/// @param new The new (page-aligned) virtual address
/// @return Success or error code
page_result_t RemapPage(virtaddr_t old, virtaddr_t new){
    old = PAGE_ADDR_MASK(old);
    if(!(PAGE_DIR_VIRTUAL(PDE_INDEX(old)) & PDE_PRESENT) || !(PAGE_TABLE_VIRTUAL(PDE_INDEX(old))->pages[PTE_INDEX(old)] & PAGE_PRESENT)){
        // No page present at that virtual address, nothing to do
        return PAGE_NOT_PRESENT;
    }

    page_t page = PAGE_TABLE_VIRTUAL(PDE_INDEX(old))->pages[PTE_INDEX(old)];
    physaddr_t frame = PAGE_ADDR_MASK(page);
    unsigned int flags = page & 0xFFF;

    cli                                             // Be absolutely certain the frame won't be retaken while moving it
    pfree(old);
    page_result_t result = physpalloc(PAGE_ADDR_MASK(new), frame, flags);
    sti

    return result;
}

/// @brief Allocate a page based on a given physical address
/// @param virt The virtual address of the page to allocate
/// @param phys The physical address of the page to allocate
/// @return Success or error code
page_result_t physpalloc(virtaddr_t virt, physaddr_t phys, unsigned int flags){
    if(PAGE_ADDR_MASK(virt) != virt || PAGE_ADDR_MASK(phys) != phys){
        return ADDRESS_NOT_ALIGNED;
    }
    index_t pdIndex = PDE_INDEX(virt);
    index_t ptIndex = PTE_INDEX(virt);

    if(!(PAGE_DIR_VIRTUAL->tables[pdIndex] & PDE_PRESENT)){
        if(AllocatePageTable(virt) != PAGE_OK){
            // Most likely out of memory
            return NO_MORE_MEMORY;
        }
    }

    if(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] & PAGE_PRESENT){
        return PAGE_ALREADY_MAPPED;
    }

    if(TestBit(PAGE_ADDR_MASK(phys) / PAGE_SIZE)){
        // Sometimes usable memory will be physically allocated, so only clear the bit if it needs to be cleared.
        ClearBit(PAGE_ADDR_MASK(phys) / PAGE_SIZE);

        // Reload the frame stack (this function likely won't often be called, and since it's a stack, a complete reload is neccessary)
        // It's also likely that (after the kernel is paged) most physically allocated regions won't be located in usable RAM.
        FlushFrameStack();
        MapFrameStack();
    }

    // Map the address
    PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] = PAGE_ADDR_MASK(phys) | flags | PAGE_NOREMAP;
    invlpg(PAGE_ADDR_MASK(virt));
    mappedPages++;
    return PAGE_OK;
}

/// @brief Release a page of virtual memory
/// @param address The virtual address of the page frame to release
/// @return Success or error code
page_result_t pfree(virtaddr_t address){
    if(PAGE_ADDR_MASK(address) != address){
        // Unaligned address
        return ADDRESS_NOT_ALIGNED;
    }
    index_t pdIndex = PDE_INDEX(address);
    index_t ptIndex = PTE_INDEX(address);

    if((!(PAGE_DIR_VIRTUAL->tables[pdIndex] & PDE_PRESENT)) || (!(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] & PDE_PRESENT))){
        // Nothing to be done
        return PAGE_NOT_PRESENT;
    }

    physaddr_t frame = PAGE_ADDR_MASK(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex]);

    if(!(PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] & PAGE_NOREMAP)){
        // Page is remappable, set its bit and push its frame
        SetBit(frame / PAGE_SIZE);
        if(!PushFrame(frame)){
            // Alignment error in the kernel, caller likely not at fault
            return GENERIC_ERROR;
        }
    }

    // Need to do something for shared pages...

    // Remove the page
    PAGE_TABLE_VIRTUAL(pdIndex)->pages[ptIndex] = 0;
    invlpg(PAGE_ADDR_MASK(address));
    mappedPages--;
    return PAGE_OK;
}

/// @brief Used for allocating pages to user programs. This checks the kernel's bounds and ensures the kernel isn't overwritten or unpaged.
/// @param address The virtual address of the page to allocate
/// @param flags The flags of the page to allocate
/// @return Success or error code
page_result_t user_palloc(virtaddr_t address, unsigned int flags){
    if(address < USER_MEM_START || address > USER_MEM_END){
        // Simple bounds check to prevent user programs from unmapping the kernel
        return PROTECTION_VIOLATION;
    }
    return palloc(address, flags);
}

/// @brief Specifically meant for programs to call, this checks the kernel's bounds and ensures the kernel isn't overwritten or unpaged.
/// @param address The address of the page to free
/// @return success or error code
page_result_t user_pfree(virtaddr_t address){
    if(address < USER_MEM_START || address > USER_MEM_END){
        // Simple bounds check to prevent user programs from unmapping the kernel
        return PROTECTION_VIOLATION;
    }
    return pfree(address);
}

/// @brief Handle a page fault within the system
/// @param errCode The error code of the fault
/// @param cr2 The value of CR2
/// @return Whether the fault was handled or not
page_result_t HandlePageFault(uint32_t errCode, uint32_t cr2){
    // This is currently a stub and not entirely needed (I have a simple handler elsewhere).
    // Do something about a page fault, such as search the swap partition for a page and return whether there was a success.
    return FAULT_FATAL;
}

// This pragma makes it a lot faster but I'm worried it won't work correctly if I use this.
//#pragma GCC push_options
//#pragma GCC optimize ("O3,unroll-loops")

/// @brief Check all of the page tables and page directories currently active for sanity. Looks for duplicate non-shared physical frames.
/// @warning This function will take a lot of time to complete (around one second). It checks every possible page frame for every possible page. 
/// @warning Best to only use for debugging.
/// @return True if the sanity check passed, false if it failed
bool SanityCheck(){
    // Painstakingly check every single page frame in the page directory and page tables with every possible page frame and search for the same frame mapped
    // to multiple addresses
    cli
    bool one = false;
    virtaddr_t firstVirt = 0;
    for(index_t i = 0; i < 1024; i++){
        for(index_t j = 0; j < MAX_PAGES; j++){
            if(PDE_ADDR_MASK(PAGE_DIR_VIRTUAL->tables[i]) == j * PAGE_SIZE && (!(PAGE_DIR_VIRTUAL->tables[i] & PDE_PRESENT)) && PDE_ADDR_MASK(PAGE_DIR_VIRTUAL->tables[i]) != 0){
                if(one){
                    // Sanity check failed! One frame has two or more virtual addresses!
                    //printk("Offending frame: 0x%x\n", j * PAGE_SIZE);
                    //printk("Offending virtual address: 0x%x\n", PAGE_TABLE_VIRTUAL(i));
                    //printk("First virtual address: 0x%x\n", firstVirt);
                    return false;
                }else{
                    firstVirt = (virtaddr_t)PAGE_TABLE_VIRTUAL(i);
                    one = true;
                }
            }

            if(!(PAGE_DIR_VIRTUAL->tables[i] & PDE_PRESENT)){
                // This page directory entry is not present, skip it
                continue;
            }
            // Now do it for all the pages in the page table this directory entry points to (skip non-allocated tables)
            for(index_t k = 0; k < 1024; k++){
                if(!(PAGE_TABLE_VIRTUAL(i)->pages[k] & PAGE_PRESENT) || PAGE_TABLE_VIRTUAL(i)->pages[k] & PAGE_SHARED){
                    // This page is not present or is shared, skip it
                    continue;
                }
                if(PAGE_ADDR_MASK(PAGE_TABLE_VIRTUAL(i)->pages[k]) == j * PAGE_SIZE && PAGE_TABLE_VIRTUAL(i)->pages[k] & PAGE_PRESENT){
                    if(one){
                        // Sanity check failed! One frame has two or more virtual addresses!
                        //printk("Offending frame: 0x%x\n", j * PAGE_SIZE);
                        //printk("Second virtual address: 0x%x\n", (i << 22) | (k << 12));
                        //printk("First virtual address: 0x%x\n", firstVirt);
                        return false;
                    }else{
                        firstVirt = (i << 22) | (k << 12);
                        one = true;
                    }
                }
            }
            one = false;
        }
    }

    return true;
}
//#pragma GCC pop_options

/// @brief Copy the page tables of another process to a new process. Ignores the kernel.
/// @param pde An array of 1024 PDEs to be mapped
/// @return There will almost certainly be a page fault on error
void ReplacePageTables(pde_t* pdes){
    for(index_t i = PDE_INDEX(USER_MEM_START); i < PDE_INDEX(USER_MEM_END); i++){
        PAGE_DIR_VIRTUAL->tables[i] = pdes[i];
    }
    FlushTLB();
}

/// @brief Clear the user memory page tables to prepare for mapping a new process
/// @warning DOES NOT DEALLOCATE THE PAGES! THE KERNEL MUST DO IT THROUGH PFREE!
void ClearPageTables(){
    for(index_t i = PDE_INDEX(USER_MEM_START); i < PDE_INDEX(USER_MEM_END); i++){
        PAGE_DIR_VIRTUAL->tables[i] = 0;
    }
    FlushTLB();
}

/// @brief Create the initial page directory and map the kernel to it as well as some other initialization
/// @param systemMem The total amount of system RAM in bytes
/// @return Success or error code
page_result_t PageKernel(size_t systemMem){
    // Uses recursive paging (which is great and simple)
    totalMemSize = systemMem;

    // Get the page fram of the page directory (this can be address 0x00000000)
    struct Page_Directory* pageDir = (struct Page_Directory*)PopFrame();
    if(pageDir == PAGE_NULL){
        printk("The page directory was NULL!\n");
        return GENERIC_ERROR;
    }
    ClearBit((physaddr_t)pageDir / PAGE_SIZE);
    memset(pageDir, 0, sizeof(struct Page_Directory));
    mappedPages++;

    // Map the page table to the last page directory entry for recursive paging
    pageDir->tables[1023] = PDE_ADDR_MASK((physaddr_t)pageDir) | DEFAULT_KERNEL_PDE_FLAGS;

    struct Page_Table* pageTable = NULL;

    // Recursive paging is as simple as that. Now we must page the kernel.
    for(physaddr_t i = (physaddr_t)kernel_start; i < (physaddr_t)kernel_end; i += PAGE_SIZE){
        // Get the page table index
        index_t pdIndex = PDE_INDEX(i);
        index_t ptIndex = PTE_INDEX(i);

        if(!(pageDir->tables[pdIndex] & PDE_PRESENT)){
            pageTable = (struct Page_Table*)PopFrame();
            if(pageTable == PAGE_NULL){
                // The frame can be safely discarded here as it was invalid anyway
                return NO_MORE_MEMORY;
            }
            ClearBit((physaddr_t)pageTable / PAGE_SIZE);
            memset(pageTable, 0, sizeof(struct Page_Table));
            pageDir->tables[pdIndex] = PDE_ADDR_MASK((physaddr_t)pageTable) | DEFAULT_KERNEL_PDE_FLAGS;
        }

        pageTable = (struct Page_Table*)(PDE_ADDR_MASK(pageDir->tables[pdIndex]));
        
        // Paging is not enabled yet, so painstakingly map the kernel to the page directory
        pageTable->pages[ptIndex] = PAGE_ADDR_MASK(i) | DEFAULT_KERNEL_PAGE_FLAGS | PAGE_NOREMAP;
        ClearBit(i / PAGE_SIZE);
        mappedPages++;
    }

    // Reload the frame stack
    FlushFrameStack();
    MapFrameStack();

    // Get the size of the RSDT and FADT tables so I don't have to map them before enabling paging
    size_t rsdtSize = acpiInfo.rsdt.rsdt->header.length;
    size_t fadtSize = acpiInfo.fadt->header.length;

    // The kernel should now be mapped, and now it's time to enable paging and use the regular paging functions to page the rest of physical RAM.
    LoadPageDirectory(pageDir);
    EnablePaging();

    // Paging is now enabled! The regular paging functions can now be used.

    // Get the next free address after the kernel with a page of padding
    virtaddr_t next_free_addr = ALIGNUP((virtaddr_t)kernel_end, PAGE_SIZE) + PAGE_SIZE;

    // Remap the VGA framebuffer
    size_t vgaPages = ALIGNUP(1024 * 256, PAGE_SIZE) / PAGE_SIZE;
    virtaddr_t newVgaAddr = next_free_addr;

    for(physaddr_t i = 0xA0000; i < 0xA0000 + (vgaPages * PAGE_SIZE); i += PAGE_SIZE){
        if(physpalloc(next_free_addr, i, DEFAULT_KERNEL_PAGE_FLAGS | PAGE_SHARED) != PAGE_OK){
            DisablePaging();
            printk("Error paging VGA memory!\n");
            STOP;
        }
        next_free_addr += PAGE_SIZE;
    }

    // Add a page of padding to keep the VGA framebuffer isolated
    next_free_addr += PAGE_SIZE;

    RemapVGA(newVgaAddr);

    printk("Remapping ACPI...\n");

    // Remap the ACPI tables
    if(acpiInfo.exists){
        // Remap the RSDP
        physaddr_t rsdpAddr = (physaddr_t)acpiInfo.rsdp.rsdpV1;
        physaddr_t rsdpAligned = PAGE_ADDR_MASK(rsdpAddr);
        size_t rsdpOffset = rsdpAddr - rsdpAligned;
        virtaddr_t rsdpVirt = next_free_addr;

        for(size_t i = 0; i < sizeof(RSDP_V2_t); i += PAGE_SIZE){
            if(physpalloc(next_free_addr, i + rsdpAligned, DEFAULT_KERNEL_PAGE_FLAGS | PAGE_SHARED) != PAGE_OK){
                return GENERIC_ERROR;
            }
            next_free_addr += PAGE_SIZE;
        }

        acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)(rsdpVirt + rsdpOffset);

        // Remap the RSDT
        physaddr_t rsdtAddr = (physaddr_t)acpiInfo.rsdt.rsdt;
        physaddr_t rsdtAligned = PAGE_ADDR_MASK(rsdtAddr);
        size_t rsdtOffset = rsdtAddr - rsdtAligned;
        virtaddr_t rsdtVirt = next_free_addr;

        for(size_t i = 0; i < rsdtSize; i += PAGE_SIZE){
            if(physpalloc(next_free_addr, i + rsdtAligned, DEFAULT_KERNEL_PAGE_FLAGS | PAGE_SHARED) != PAGE_OK){
                return GENERIC_ERROR;
            }
            next_free_addr += PAGE_SIZE;
        }

        acpiInfo.rsdt.rsdt = (RSDT_t*)(rsdtVirt + rsdtOffset);

        // Remap the FADT
        physaddr_t fadtAddr = (physaddr_t)acpiInfo.fadt;
        physaddr_t fadtAligned = PAGE_ADDR_MASK(fadtAddr);
        size_t fadtOffset = fadtAddr - fadtAligned;
        virtaddr_t fadtVirt = next_free_addr;

        for(size_t i = 0; i < fadtSize; i += PAGE_SIZE){
            if(physpalloc(next_free_addr, i + fadtAligned, DEFAULT_KERNEL_PAGE_FLAGS | PAGE_SHARED) != PAGE_OK){
                return GENERIC_ERROR;
            }
            next_free_addr += PAGE_SIZE;
        }

        acpiInfo.fadt = (FADT_t*)(fadtVirt + fadtOffset);
    }

    // NOTE: Will likely need to remap more ACPI tables in the future. For now, this should be fine.

    printk("ACPI remapped!\n");

    // Add a page of padding between the ACPI tables and the heap
    next_free_addr += PAGE_SIZE;

    printk("Paging the heap...\n");
    // Map the heap
    heapStart = next_free_addr;
    heapEnd = USER_MEM_START;                   // Currently using lower half kernel. I'll need to remember to change this to USER_MEM_END when I go higher-half.

    if(palloc(heapStart, DEFAULT_KERNEL_PAGE_FLAGS) != PAGE_OK){
        return GENERIC_ERROR;
    }

    printk("Sanity ckecking the page tables...\n");
    if(!SanityCheck()){
        printk("CRITICAL ERROR: PAGE TABLE SANITY CHECK FAILED!\n");
        STOP
    }

    printk("Sanity check passed!\n");

    return PAGE_OK;
}