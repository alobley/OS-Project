#ifndef CUSTOMFS_H
#define CUSTOMFS_H

#include <types.h>
#include <util.h>

// These could probably take up one sector each
typedef struct file_node{
    uint32 nameSize;        // How many bytes the name takes up
    uint32 fileSize;        // How many bytes this part of the file takes up
    uint64 dataLba;         // The LBA of the data
    uint64 nextLba;         // The LBA of the next part of the file
    bool deleted;           // If the file has been deleted (this means it can be overwritten at will)
    uint8 type;             // The type of file node this is (can point to another directory, like this one)
    char name[487];         // The name of the file (located at the end of the file node)
} PACKED filenode_t;

// Assuming one directory takes up two sectors
typedef struct dir {
    uint32 nameSize;                       // How many bytes the name takes up
    uint32 numSectors;                     // How many sectors the directory "inode" takes up
    uint32 numFiles;                       // How many files are in the directory
    uint64 nodeLBAs[62];                   // The LBAs of the file nodes (first ones should be read for names, and last one is LBA of next directory node, if it exists)
    char name[512];                        // The name of the directory (located at its end, insanely huge but this is just a concept)
} PACKED dir_t;

#endif