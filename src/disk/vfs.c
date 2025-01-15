#include "vfs.h"
#include <memmanage.h>
#include <string.h>

#define DEFAULT_ROOTDISK 0

uint8 rootDisk = 0;         // The ATA index of the root disk, NOT the VFS index

char* rootDir = ROOT_MNT;

char* GetRootDir(){
    return rootDir;
}

// An array of pointers to all the ATA disks
vfs_disk_t* disks[MAX_DRIVES];

vfs_disk_t** GetDisks(){
    return &disks[0];
}

// It's probably safe not to have to mount other disks, but it's a good idea to have the functionality
// I'll need a way to copy the kernel files to the disk, so I'll need ISO9660 support and USB support
void InitializeDisks(){
    disks[0] = FindRoot();
    int accumulator = 0;
    for(int i = 1; i < MAX_DRIVES; i++){
        disks[i] = DefineDisk(accumulator);
        accumulator++;
        if(accumulator == rootDisk){
            accumulator++;
        }
    }
}

directory_t* GetRoot(vfs_disk_t* disk){
    if(disk->fstype == FS_UNSUPPORTED){
        return NULL;
    }else if(disk->fstype == FS_FAT32){
        fat_disk_t* fatdisk = alloc(sizeof(fat_disk_t));
        fatdisk->parent = disk->parent;
        fatdisk->fstype = disk->fstype;
        fatdisk->paramBlock = (bpb_t*)ReadSectors(fatdisk->parent, 1, 0);
        FAT_cluster_t* fatroot = FatReadRootDirectory(fatdisk);
        return FATDirToVfsDir(fatroot, fatdisk, rootDir);
    }else{
        // Other FS types (unimplemented)
        return NULL;
    }
}

vfs_disk_t* DefineDisk(uint8 diskNum){
    vfs_disk_t* disk = (vfs_disk_t*)alloc(sizeof(vfs_disk_t));
    if(disk == NULL){
        return NULL;
    }
    disk->parent = IdentifyDisk(diskNum);
    if(disk->parent == NULL){
        // Invalid disk
        return NULL;
    }

    fat_disk_t* fatdisk =  TryFatFS(disk->parent);
    if(fatdisk == NULL){
        // Invalid disk
        dealloc(disk);
        return NULL;
    }

    disk->fstype = fatdisk->fstype;
    dealloc(fatdisk);

    while(disk->fstype == FS_UNSUPPORTED){
        // Try next filesystems... (implement later)
        dealloc(disk);
        return NULL;
    }
    return disk;
}

vfs_disk_t* FindRoot(){
    uint8 tryDisk = DEFAULT_ROOTDISK;
    disk_t* disk = IdentifyDisk(tryDisk);
    while(disk->type != PATADISK){
        // Get the first PATA disk
        if(tryDisk >= MAX_DRIVES){
            return NULL;
        }
        tryDisk++;
        dealloc(disk);
        disk = IdentifyDisk(tryDisk);
    }
    rootDisk = tryDisk;
    vfs_disk_t* root = DefineDisk(rootDisk);

    root->parent = disk;
    root->mountDir = GetRoot(root);
    root->mountDir->name = rootDir;
    return root;
}

// Don't forget to implement directory searching. Right now it just searches the VFS root.
file_t* GetFile(char* filePath){
    vfs_disk_t* disk = disks[DEFAULT_ROOTDISK];
    char* fileName = strtok(filePath, ROOT_MNT);

    //printk(fileName);

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