#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <console.h>
#include <util.h>
#include <string.h>
#include <multiboot.h>

#define PAGE_SIZE 4096

typedef unsigned int virtaddr_t;
typedef unsigned int physaddr_t;

// This points to a page table (a huge array of pages) in a page directory
typedef uint32_t pde_t;

// This is a page table entry
typedef uint32_t page_t;

typedef struct PACKED PageTable {
    page_t pages[1024];
} PACKED page_table_t;

#define PTE_FLAG_PRESENT (1 << 0)
#define PTE_FLAG_RW (1 << 1)
#define PTE_FLAG_USER (1 << 2)
#define PTE_FLAG_WRITE_THROUGH (1 << 3)
#define PTE_FLAG_CACHE_DISABLED (1 << 4)
#define PTE_FLAG_ACCESSED (1 << 5)
#define PTE_FLAG_DIRTY (1 << 6)
#define PTE_FLAG_PAT (1 << 7)
#define PTE_FLAG_GLOBAL (1 << 8)

#define PDE_FLAG_PRESENT (1 << 0)
#define PDE_FLAG_RW (1 << 1)
#define PDE_FLAG_USER (1 << 2)
#define PDE_FLAG_WRITE_THROUGH (1 << 3)
#define PDE_FLAG_CACHE_DISABLED (1 << 4)
#define PDE_FLAG_ACCESSED (1 << 5)
#define PDE_FLAG_IGNORED (1 << 6)
#define PDE_FLAG_PAGE_SIZE (1 << 7)

void MapBitmap(uint32_t memSize, mmap_entry_t* mmap, size_t mmapLength /* In total entries */);

void pfree(virtaddr_t virt);

int palloc(virtaddr_t virt, uint32_t flags);

void ConstructPageDirectory(pde_t* pageDirectory, page_table_t* pageTables);
int physpalloc(physaddr_t phys, virtaddr_t virt, uint32_t flags);

void PageKernel(size_t memSize);

// This is a non-page-aligned address, so it is not a valid pageable address
#define INVALID_ADDRESS 3

extern pde_t* currentPageDir;
extern page_table_t* currentPageTables;
extern size_t totalPages;

extern size_t totalMemSize;

extern int __kernel_end;
extern int __kernel_start;

#endif