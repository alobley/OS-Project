#include "paging.h"
#include <vga.h>

#pragma GCC push_options
#pragma GCC optimize("O0")

// The page directory for the OS
PageDirectoryEntry ALIGNED(4096) pageDir[1024];

extern uintptr_t __kernel_start;


#define GetPhysicalAddress(address) (address & 0xFFFFF000)         // Get the physical address from a page table entry pointer
#define AddressToEntryPointer(address) (address >> 12)             // Convert a physical address to a page table entry pointer

#define PDI(value) ((value) >> 22)
#define PTI(value) (((value) >> 12) & 0x3FF)

extern uint32 PAGE_TABLE_AREA_BEGIN;
extern uint32 PAGE_TABLE_AREA_END;

uint32 current_page_table_addr = 0;

uint32 allPages = 0;
uint32 mappedPages = 0;

uint32 GetPages(){
    return mappedPages;
}

PageTable* AllocatePageTable(){
    PageTable* table = (PageTable*)current_page_table_addr;
    current_page_table_addr += 4096;

    memset(table, 0, sizeof(PageTable));
    return table;
}

uint32 CurrentDirIndex = 0;
uint32 CurrentTableIndex = 0;

// Activate a set of new pages in a page directory.
page_t* palloc(uintptr_t virtualAddr, uintptr_t physicalAddr, size_t pagesToAdd, PageDirectory* pageDir, bool user){
    // Get the page directory entry
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
            table->entries[PTI(virtualAddr + (i * 4096))].address = (physicalAddr + (i * 4096)) >> 12;      // The physical memory location of the page
            mappedPages++;
            return &table->entries[PTI(virtualAddr + (i * 4096))];                                          // Return the first page table entry (allocated consecutively)
        }
    }
    return NULL;
}

void pfree(uintptr_t virtualAddr, PageDirectory* pageDir){
    PageDirectoryEntry* dirEntry = &pageDir->entries[PDI(virtualAddr)];
    PageTable* table = (PageTable*)GetPhysicalAddress(dirEntry->address);
    if(table->entries[PTI(virtualAddr)].present){
        table->entries[PTI(virtualAddr)].present = 0;
        table->entries[PTI(virtualAddr)].address = 0;
    }
}

extern uint32 __kernel_end;
extern uint32 __kernel_start;

// Page the kernel
PageDirectory* InitPaging(size_t totalMem){
    // Initialize the page directory
    memset(&pageDir[0], 0, sizeof(PageDirectoryEntry) * 1024);

    current_page_table_addr = PAGE_TABLE_AREA_BEGIN;
    PAGE_TABLE_AREA_END = PAGE_TABLE_AREA_BEGIN + (1024 * 4096);

    uint32 totalPages = totalMem / 4096;
    uint32 totalTables = totalPages / 1024;

    if(totalMem % 4096 != 0){
        // Check if there is leftover memory that doesn't span a whole page, and give it a page
        totalPages++;
    }

    allPages = totalPages;

    if(totalPages % 1024 != 0){
        // Check if there is leftover memory that doesn't span a whole table, and give it a table
        totalTables++;
    }

    size_t pagesLeft = totalPages;
    size_t addr = 0;
    for(uint32 i = 0; i < totalTables; i++){
        PageTable* table = AllocatePageTable();
        if(table == NULL){
            printk("Failed to allocate page table\n");
            return NULL;
        }
        if(i == 0){
            // Map only the kernel
            pageDir[i].address = AddressToEntryPointer((uintptr_t)table);
            pageDir[i].present = 1;
            pageDir[i].readWrite = 1;
            pageDir[i].user = 0;
            pageDir[i].writeThrough = 0;
            pageDir[i].cacheDisabled = 0;
            pageDir[i].dirty = 0;
            pageDir[i].pageSize = 0;
        }else{
            pageDir[i].address = AddressToEntryPointer((uintptr_t)table);
            pageDir[i].present = 0;
            pageDir[i].readWrite = 1;
            pageDir[i].user = 0;
            pageDir[i].writeThrough = 0;
            pageDir[i].cacheDisabled = 0;
            pageDir[i].dirty = 0;
            pageDir[i].pageSize = 0;
        }
        for(uint32 j = 0; j < 1024 && pagesLeft > 0; j++, addr += 4096, pagesLeft--){
            if(j >= __kernel_start / 4096 && j < (__kernel_start + __kernel_end) / 4096){
                palloc(addr, addr, 1, &pageDir[i], false);
            }else{
                table->entries[j].address = 0;
                table->entries[j].present = 0;
                table->entries[j].readWrite = 1;
                table->entries[j].user = 0;
                table->entries[j].writeThrough = 0;
                table->entries[j].cacheDisabled = 0;
                table->entries[j].dirty = 0;
                table->entries[j].pageSize = 0;
                table->entries[j].global = 0;
            }
        }
    }

    palloc(VGA_PIXEL_MODE_START, VGA_PIXEL_MODE_START, VGA_PIXEL_MODE_SIZE / 4096, &pageDir[0], false);

    printk("Page directory location: %x\n", &pageDir[0]);

    // Enable paging
    cr3((uint32)&pageDir[0]);
    uint32 currentCr0 = 0;
    get_cr0(currentCr0);
    cr0(currentCr0 | 0x80000001);

    return (PageDirectory*)&pageDir[0];
}
#pragma GCC pop_options