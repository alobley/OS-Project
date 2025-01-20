#include "vfs.h"
#include <memmanage.h>
#include <string.h>

// This is a bare bones VFS implementation. It only supports FAT32 for now, but it will be expanded to support other filesystems.

#define DEFAULT_ROOTDISK 0

uint8 rootDisk = 0;         // The ATA index of the root disk, NOT the VFS index

char* rootDir = ROOT_MNT;

char* GetRootDir(){
    return rootDir;
}

// An array of pointers to all the ATA disks
vfs_disk_t* disks[MAX_DRIVES] = {0};

//vfs_disk_t* GetDisks(){
    //return &disks[0];
//}

// It's probably safe not to have to mount other disks, but it's a good idea to have the functionality
// I'll need a way to copy the kernel files to the disk, so I'll need ISO9660 support and USB support
void InitializeDisks(){
    disks[0] = FindRoot();
    if(disks[0] == NULL){
        return;
    }
    int accumulator = 0;
    for(int i = 1; i < MAX_DRIVES; i++){
        disks[i] = DefineDisk(accumulator);
        accumulator++;
        if(accumulator == rootDisk){
            accumulator++;
        }
    }
}

// Get the root directory of a disk
directory_t* GetRoot(vfs_disk_t* disk){
    if(disk->fstype == FS_UNSUPPORTED){
        return NULL;
    }else if(disk->fstype == FS_FAT32){
        fat_disk_t* fatdisk = TryFatFS(disk->parent);
        FAT_cluster_t* fatroot = NULL;//FatReadRootDirectory(fatdisk);
        directory_t* dir = FATDirToVfsDir(fatroot, fatdisk, rootDir);
        return dir;
    }else{
        // Other FS types (unimplemented)
        return NULL;
    }
}

vfs_disk_t* DefineDisk(disk_t* parent){
    vfs_disk_t* disk = (vfs_disk_t*)alloc(sizeof(vfs_disk_t));
    if(disk == NULL){
        return NULL;
    }
    disk->parent = parent;

    fat_disk_t* fatdisk = TryFatFS(disk->parent);
    if(fatdisk == NULL){
        // Invalid disk
        dealloc(disk);
        return NULL;
    }
    
    disk->fstype = fatdisk->fstype;
    dealloc(fatdisk->paramBlock);
    dealloc(fatdisk);

    while(disk->fstype == FS_UNSUPPORTED){
        // Try next filesystems... (implement later)
        return NULL;
    }
    return disk;
}

vfs_disk_t* FindRoot(){
    disk_t* disk;
    for(int i = 0; i < MAX_DRIVES; i++){
        disk = IdentifyDisk(i);
        if(disk == NULL){
            return NULL;
        }
        if(disk->type == PATADISK){
            break;
        }
        dealloc(disk);
    }

    rootDisk = disk->driveNum;
    vfs_disk_t* root = DefineDisk(disk);
    if(root == NULL){
        return NULL;
    }

    root->parent = disk;
    //printk("Disk parent address in FindRoot: 0x%x\n", root->parent);
    root->mountDir = GetRoot(root);
    root->mountDir->name = rootDir;
    return root;
}

// Don't forget to implement directory searching. Right now it just searches the VFS root.
file_t* GetFile(char* filePath){
    vfs_disk_t* disk = disks[DEFAULT_ROOTDISK];
    char* fileName = strtok(filePath, ROOT_MNT);

    if(disk->fstype == FS_UNSUPPORTED){
        return NULL;
    }else if(disk->fstype == FS_FAT32){
        fat_disk_t* fatdisk = TryFatFS(disk->parent);
        file_t* file = FatSeekFile(fatdisk, fileName);
        dealloc(fatdisk->paramBlock);
        dealloc(fatdisk);
        return file;
    }
}

void DeallocFile(file_t* file){
    dealloc(file->data);
    dealloc(file->name);
    dealloc(file);
}