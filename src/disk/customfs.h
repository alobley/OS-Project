#ifndef CUSTOMFS_H
#define CUSTOMFS_H

#include <types.h>
#include <util.h>

// The filesystem header, located at disk sector 2
typedef struct fsheader{
    uint64 rootLba;         // The LBA of the root directory node
    size_t reservedSectors; // How many sectors are reserved (after this one) for the filesystem or bootloader
    // More data...
} PACKED fsheader_t;

// These could probably take up one sector each
typedef struct file_node{
    uint8 type;             // The type of file node this is (can be to another directory, if it is this structure is a dir_t) (should also have a deleted flag)
    uint32 nameSize;        // How many bytes the name takes up
    uint32 fileSize;        // How many bytes this part of the file takes up
    uint64 dataLba;         // The LBA of the data
    uint64 nextLba;         // The LBA of the next part of the file
    char name[487];         // The name of the file (located at the end of the file node)
} PACKED filenode_t;

// Assuming one directory takes up two sectors
typedef struct dir {
    uint8 type;
    uint32 nameSize;                       // How many bytes the name takes up
    uint32 numSectors;                     // How many sectors the directory "inode" takes up
    uint32 numFiles;                       // How many files are in the directory
    uint64 nodeLBAs[62];                   // The LBAs of the file nodes (first ones should be read for names, and last one is LBA of next directory node, if it exists)
    char name[512];                        // The name of the directory (located at its end, insanely huge but this is just a concept)
} PACKED dir_t;

#endif