#ifndef VFS_H
#define VFS_H

#include <types.h>
#include <util.h>
#include "ata.h"


#define TYPE_FILE 0
#define TYPE_DIR 1

typedef struct File {
    char* name;
    size_t size;
    void* data;
} file_t;

typedef struct DirectoryEntry {
    char* name;
    uint8 type;
    struct DirectoryEntry* next;
} directory_entry_t;

typedef struct Directory {
    char* name;
    directory_entry_t* firstFile;
} directory_t;

typedef struct vfs_disk{
    disk_t* parent;
    directory_t* mountDir;
    uint8 fstype;
} vfs_disk_t;

// Will need to add SATA, ISO9660 and EXT2 support
#include "fat.h"

#define FS_FAT12 0
#define FS_FAT16 1
#define FS_FAT32 2

// Unsupported for now
#define FS_EXFAT 3
#define FS_ISO9660 4
#define FS_EXT2 5           // Will likely be the primary filesystem for the OS

#define FS_UNSUPPORTED 0xFF

#define ROOT_INDEX 0

#define ROOT_MNT "/"

vfs_disk_t* FindRoot();
vfs_disk_t* DefineDisk(uint8 diskNum);
file_t* GetFile(char* fileName);
void DeallocFile(file_t* file);
void InitializeDisks();

#endif