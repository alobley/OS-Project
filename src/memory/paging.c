#include "paging.h"
#include <vga.h>

#pragma GCC push_options
#pragma GCC optimize("O0")

// The page directory for the OS
PageDirectoryEntry ALIGNED(4096) pageDir[1024] = {0};

extern uintptr_t __kernel_start;

uint32 mappedPages = 0;

#define GetPhysicalAddress(address) (address & 0xFFFFF000)         // Get the physical address from a page table entry pointer
#define AddressToEntryPointer(address) (address >> 12)             // Convert a physical address to a page table entry pointer

#define PDI(value) ((value) >> 22)
#define PTI(value) (((value) >> 12) & 0x3FF)

// Pages the kernel's memory
void PageKernel(size_t kernelSize){

}

extern uint32 PAGE_TABLE_AREA_BEGIN;
extern uint32 PAGE_TABLE_AREA_END;

uint32 current_page_table_addr = 0;

uint32 allPages = 0;

uint32 GetPages(){
    return allPages;
}

PageTable* AllocatePageTable(){
    if (current_page_table_addr + 4096 > PAGE_TABLE_AREA_END) {
        // No more space for page tables
        return NULL;
    }

    PageTable* table = (PageTable*)current_page_table_addr;
    current_page_table_addr += 4096;

    memset(table, 0, sizeof(PageTable));
    return table;
}

// Allocate pages of memory
void* palloc(uintptr_t virtualAddress, size_t size){
    uint32 startAddr = AddressToEntryPointer(virtualAddress);
    return NULL;
}

uint32 CurrentDirIndex = 0;
uint32 CurrentTableIndex = 0;

PageDirectory* InitPaging(size_t totalMem){
    // Initialize the page directory
    memset(&pageDir[0], 0, sizeof(PageDirectoryEntry) * 1024);

    current_page_table_addr = PAGE_TABLE_AREA_BEGIN;
    PAGE_TABLE_AREA_END = PAGE_TABLE_AREA_BEGIN + sizeof(PageDirectory) + (sizeof(PageTable) * 1024);

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
            return NULL;
        }
        pageDir[i].address = AddressToEntryPointer((uintptr_t)table);
        pageDir[i].present = 1;
        pageDir[i].readWrite = 1;
        pageDir[i].user = 0;
        pageDir[i].writeThrough = 0;
        pageDir[i].cacheDisabled = 0;
        pageDir[i].dirty = 0;
        pageDir[i].pageSize = 0;
        for(uint32 j = 0; j < 1024 && pagesLeft > 0; j++, addr += 4096, pagesLeft--){
            table->entries[j].address = addr >> 12;
            table->entries[j].present = 1;
            table->entries[j].readWrite = 1;
            table->entries[j].user = 0;
            table->entries[j].writeThrough = 0;
            table->entries[j].cacheDisabled = 0;
            table->entries[j].dirty = 0;
            table->entries[j].pageSize = 0;
            table->entries[j].global = 0;
        }
    }

    // Enable paging
    asm volatile("mov %0, %%cr3" : : "r" (&pageDir[0]));
    uint32 cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000001;
    asm volatile("mov %0, %%cr0" : : "r" (cr0));

    return (PageDirectory*)&pageDir[0];
}
#pragma GCC pop_options