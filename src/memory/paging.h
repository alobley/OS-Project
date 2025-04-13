/***********************************************************************************************************************************************************************
 * Copyright (c) 2025, xWatexx. All rights reserved.
 * This software is licensed under the MIT license. See the LICENSE file in the root directory for more details.
 * 
 * This file is part of Dedication OS.
 * This file contains definitions, typedefs, macros and functions for the Dedication OS paging and virtual memory subsystem.
 * The abstractions in this file can be very helpful.
 ***********************************************************************************************************************************************************************/
#ifndef PAGING_H
#define PAGING_H

#include <common.h>
#include <multiboot.h>

#define PAGE_SIZE 0x1000                                    // The size of a single standard (4KiB) page
#define HUGE_PAGE_SIZE 0x400000                             // The size of a signle huge (4MiB) page

// This should always be the last 4MiB of virtual memory (the last page directory entry should always point to and hold the pages containing its tables)
// The directory, however, needs an extra 4KiB elsewhere. I have inserted it just before the page directory.
#define PD_VIRTADDR 0xFFFFF000                              // The virtual address of the page directory. One extra page of space is needed before this, meaning 2 PDEs.

// The page tables will all be mapped linearly into contiguous virtual memory. This means that the page tables are a big array of struct Page_Table.
#define PT_VIRTADDR 0xFFC00000

// The first virtual memory address containing user-mode RAM
#define USER_MEM_START 0x40000000                           // TODO: Fix this and replace with bounds for a higher-half architecture

// The last virtual memory address containing user-mode RAM
#define USER_MEM_END PD_VIRTADDR                            // TODO: Fix this and replace with bounds for a higher-half architecture
// Can I get around the address discrepancy using PIC?
#define KERNEL_PHYSADDR 0x200000                            // The kernel will expect to be loaded at this address.
#define KERNEL_VIRTADDR 0x200000                            // TODO: Fix this and create atrampoline section to get to a higher-half kernel (this value should be 0xC0000000)

#define ADDRESS_LIMIT 0xFFFFFFFF

// One million pages
#define MAX_PAGES 0x100000
#define TOTAL_BITS (MAX_PAGES / 8)
#define BITS_PER_BITMAP_ENTRY 8

// Variables defined by the linker script (IMPORTANT: Don't read from or write to the actual data at these locations)
extern const void __kernel_physaddr;
extern const void __kernel_start;
extern const void __kernel_end;

static const void* kernel_start = &__kernel_start;
static const void* kernel_end = &__kernel_end;
static const void* kernel_physaddr = &__kernel_physaddr;

typedef unsigned int physaddr_t;
typedef unsigned int virtaddr_t;

// Page directory entry flags (although many of these are based on individual pages)
#define PDE_PRESENT (1 << 0)                                // Whether this PDE is present in physical memory (if disabled, reading and writing to this PDE will cause a page fault)
#define PDE_RW (1 << 1)                                     // Allow both reading and writing of this PDE (read-only if disabled)
#define PDE_USER (1 << 2)                                   // This PDE is accessible by ring 3 code
#define PDE_WRITE_THROUGH (1 << 3)                          // Enable write-through caching (all memory writes go directly to RAM)
#define PDE_CACHE_DISABLE (1 << 4)                          // Disable caching of this PDE
#define PDE_ACCESSED (1 << 5)                               // Whether this PDE has been accessed by a virtual address translation
#define PDE_NOSWAP (1 << 9)                                 // This bit is available for use by the OS, so if set the OS won't swap out pages in this directory.
#define PDE_ADDR_MASK(addr) (addr & 0xFFFFF000)             // Mask a physical address (pointing to a page table) to align it properly and set flags. It should already be aligned regardless. Small pages only.

// Huge pages (requites PSE/PAE(?) to be enabled) - these PDEs point to actual pages rather than page tables.
#define PDE_DIRTY (1 << 6)                                  // Huge pages only. Whether this page directory entry has been written to. This should be set by the CPU.
#define PDE_HUGEPAGE (1 << 7)                               // Enable huge (4MB) pages
#define PDE_GLOBAL (1 << 8)                                 // Huge pages only. This page directory entry is global, meaning the PDE is never invalidated, even if CR3 changes
#define PDE_PAT (1 << 12)                                   // Huge pages only. If PAT is enabled, then this indicates caching type. Otherwise reserved.
// Pretty sure these are correct
#define PDE_HUGEADDR_HIGH_MASK(addr) (addr & 0x001FE000)    // Get bits 32-39 of the physical address of the page tables (PAE only?)
#define PDE_HUGEADDR_LOW_MASK(addr) (addr & 0xFFC00000)     // Get bits 22-31 of the physical address of the page tables

// Page table entry (actual page) flags
#define PAGE_PRESENT (1 << 0)                               // This page is present in physical memory
#define PAGE_RW (1 << 1)                                    // This page is writable and readable
#define PAGE_USER (1 << 2)                                  // This page is accessible by user programs
#define PAGE_WRITE_THROUGH (1 << 3)                         // This page writes through the cache
#define PAGE_CACHE_DISABLE (1 << 4)                         // This page ignores the cache
#define PAGE_ACCESSED (1 << 5)                              // The page was read during a virtual address translation
#define PAGE_DIRTY (1 << 6)                                 // This page has had data written to it. Set by the CPU.
#define PAGE_PAT (1 << 7)                                   // The page attribute table (PAT) is enabled for this page
#define PAGE_GLOBAL (1 << 8)                                // The page is never invalidated, even if CR3 changes
#define PAGE_NOSWAP (1 << 9)                                // Available for the OS to use. Disables swapping by the OS.
#define PAGE_NOREMAP (1 << 10)                              // Available for the OS to use. Determines whether the page's frame can be reused as regular memory.
#define PAGE_SHARED (1 << 11)                               // This page is shared between multiple processes. This is a software flag and not set by the CPU.

// Define some default flags which will most commonly be used for convenience
#define DEFAULT_KERNEL_PDE_FLAGS (PDE_PRESENT | PDE_RW)
#define DEFAULT_USER_PDE_FLAGS (PDE_PRESENT | PDE_RW | PDE_USER)

#define DEFAULT_KERNEL_PAGE_FLAGS (PAGE_PRESENT | PAGE_RW)
#define DEFAULT_USER_PAGE_FLAGS (PAGE_PRESENT | PAGE_RW | PAGE_USER)

// Mask a physical address of a page (4KiB of physical memory) to align it properly and set flags. The address should already be aligned regardless.
#define PAGE_ADDR_MASK(addr) ((addr) & 0xFFFFF000)

// Align a number up to the nearest alignment boundary (align must be a power of 2)
#define ALIGNUP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define INVALID_ADDRESS 3                                   // An address that is not page-aligned and therefore indicates an invalid page.
#define PAGE_NULL ((void*)INVALID_ADDRESS)                  // Must be different than actual NULL because NULL is a valid page address

// A success or error code returned by a paging operation
typedef enum PAGE_RESULT {
    // Errors (negative values)
    FAULT_FATAL              = -11,  // Page fault occurred in the kernel  or critical code and therefore is unrecoverable. The OS should halt execution.
    FAULT_NONFATAL           = -10,  // The page fault could not be handled and was likely in user mode. The OS should kill the offending process
    ADDRESS_NOT_ALIGNED      = -9,   // A given address wasn't aligned
    PAGE_NOT_PRESENT         = -8,   // A requested page did not have the present flag
    PAGE_NOT_MAPPED          = -7,   // A requested page had no mapping
    PROTECTION_VIOLATION     = -6,   // Protection flags were violated (e.g., write to read-only)
    NO_MORE_MEMORY           = -5,   // System ran out of memory
    TABLE_FULL               = -4,   // Page table is full
    NO_FRAME_FOUND           = -3,   // Could not find a frame for the page
    GENERIC_ERROR            = -1,   // Internal/unknown error

    // Success and info (zero or positive)
    PAGE_OK                  = 0,    // Successful operation
    PAGE_ALREADY_MAPPED      = 1,    // Page was already mapped
    CANNOT_SWAP              = 2,    // Page marked NOSWAP cannot be swapped
    NOTHING_TO_DO            = 3,    // No operation needed
    FAULT_HANDLED            = 4     // A page fault was handled gracefully
} page_result_t;

// The total amount of system RAM in bytes
extern size_t totalMemSize;

// The total number of mapped pages (including those not currently in the page directory)
extern size_t mappedPages;

/// @brief Take a virtual address and return its actual address in physical memory (does not require alignment)
/// @param virt The virtual address to convert
/// @return The physical address of the memory
physaddr_t GetPhysicalAddress(virtaddr_t virt);

/// @brief Map the memory bitmap based on the multiboot memory map
/// @param memoryMap The array of multiboot memory map entries
/// @return Success or error code
void MapBitmap(mmap_entry_t* memoryMap, size_t memmapLength);

/// @brief Allocate a page with a given virtual address
/// @param address The address of the new page (must be aligned)
/// @return Success or error code
page_result_t palloc(virtaddr_t address, unsigned int flags);

/// @brief Allocate a page based on a given physical address
/// @param virt The virtual address of the page to allocate
/// @param phys The physical address of the page to allocate
/// @return Success or error code
page_result_t physpalloc(virtaddr_t virt, physaddr_t phys, unsigned int flags);

/// @brief Release a page of virtual memory
/// @param address The virtual address of the page frame to release
/// @return Success or error code
page_result_t pfree(virtaddr_t address);

/// @brief Used for allocating pages to user programs. This checks the kernel's bounds and ensures the kernel isn't overwritten or unpaged.
/// @param address The virtual address of the page to allocate
/// @param flags The flags of the page to allocate
/// @return Success or error code
page_result_t user_palloc(virtaddr_t address, unsigned int flags);

/// @brief Specifically meant for programs to call, this checks the kernel's bounds and ensures the kernel isn't overwritten or unpaged.
/// @param address The address of the page to free
/// @return success or error code
page_result_t user_pfree(virtaddr_t address);

/// @brief Create the initial page directory and map the kernel to it
/// @param bootInfo The multiboot info structure
/// @param systemMem The total amount of system RAM in bytes
/// @return Success or error code
page_result_t PageKernel(size_t systemMem);

/// @brief Handle a page fault within the system
/// @param errCode The error code of the fault
/// @param cr2 The value of CR2
/// @return Whether the fault was handled or not
page_result_t HandlePageFault(uint32_t errCode, uint32_t cr2);

/// @brief Check all of the page tables and page directories for sanity
/// @warning This function will take a lot of time to complete. It checks every possible page frame for every possible page.
/// @return True if the sanity check passed, false if it failed
bool SanityCheck();

#endif