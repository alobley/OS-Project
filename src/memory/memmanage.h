#ifndef MEMMANAGE_H
#define MEMMANAGE_H

#include <types.h>
#include <util.h>
#include "multiboot.h"

// For easier address interpretation and translation
typedef struct PACKED VirtualAddress {
    uint32 offset : 12;
    uint32 table : 10;
    uint32 directory : 10;
} VirtualAddress;

// Page directory entry, points to a collection of page tables
typedef struct PACKED PageDirectoryEntry {
    uint32 present : 1;                 // Is the page present in memory? 1 if true, 0 if false. (Whether or not the page is in swap space on the disk, basically)
    uint32 readWrite : 1;               // Is the page read/write? 1 if read/write, 0 if read-only
    uint32 user : 1;                    // Is the page user-mode? 1 if user-mode, 0 if kernel-mode
    uint32 writeThrough : 1;            // Should the page be write-through? 1 if write-through, 0 if write-back (write-through means ignore the cache)
    uint32 cacheDisabled : 1;           // Set = disable automatic caching of this page, clear = let the CPU cache this page (If I have a page with MMIO, this must be set)
    uint32 accessed : 1;                // I can safely ignore this bit (it will be set by the CPU)
    uint32 dirty : 1;                   // Has this page been written to? 1 if true, 0 if false
    uint32 pageSize : 1;                // Make sure this is clear for 4 KiB pages
    uint32 available : 4;               // Unused, best not to touch
    uint32 address : 20;                // The physical address of the page table (is a PageTable*)
} PageDirectoryEntry;

// Page table entry, points to a physical address
typedef struct PACKED PageTableEntry{
    uint32 present : 1;                 // Is the page present in memory? 1 if true, 0 if false. (Whether or not the page is in swap space on the disk, basically)
    uint32 readWrite : 1;               // Is the page read/write? 1 if read/write, 0 if read-only
    uint32 user : 1;                    // Is the page user-mode? 1 if user-mode, 0 if kernel-mode
    uint32 writeThrough : 1;            // Should the page be write-through? 1 if write-through, 0 if write-back (write-through means ignore the cache)
    uint32 cacheDisabled : 1;           // Set = disable automatic caching of this page, clear = let the CPU cache this page (If I have a page with MMIO, this must be set)
    uint32 accessed : 1;                // I can safely ignore this bit (it will be set by the CPU)
    uint32 dirty : 1;                   // Has this page been written to? 1 if true, 0 if false
    uint32 pageSize : 1;                // Make sure this is clear for 4 KiB pages
    uint32 global : 1;                  // Is this page global? 1 if global, 0 if not
    uint32 available : 3;               // Unused, best not to touch
    uint32 address : 20;                // The (physical?) address of the page
} page_t;

// The page table. In 32-bit x86, assuming 4GB of memory, there are 1024 entries in the page table
typedef struct ALIGNED(4096) PageDirectory {
    PageDirectoryEntry entries[1024];
} PageDirectory;

typedef struct ALIGNED(4096) PageTable {
    page_t entries[1024];
} PageTable;

#define GetPhysicalAddress(address) ((address) * 0x1000)             // Get the physical address from a page table entry pointer
#define AddressToEntryPointer(address) ((address) >> 12)             // Convert a physical address to a page table entry pointer

#define PDI(value) ((value) >> 22)
#define PTI(value) (((value) >> 12) & 0x3FF)

#define KERNEL_VIRTADDR 0xC0000000      // Don't use this yet

typedef struct memory_block {
    size_t size;                 // Size of the memory block
    struct memory_block* next;   // Pointer to the next free block of memory
    bool free;                   // Block free or bot
    bool paged;                  // Is this block paged?
} memory_block_t;

#define MEMORY_BLOCK_SIZE (sizeof(memory_block_t))
#define PAGE_SIZE 4096

void PageKernel();
uintptr_t GetVgaRegion();
page_t* palloc(uintptr_t virtualAddr, uintptr_t physicalAddr, size_t numPages, PageDirectory* pageDir, bool user);
void pfree(uintptr_t virtualAddr, PageDirectory* pageDir);
size_t GetPages();
void* alloc(size_t size);
void dealloc(void* ptr);
size_t GetTotalMemory();
void PanicFree();
PageDirectory* GetCurrentPageDir();

#endif