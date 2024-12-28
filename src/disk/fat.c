#include "fat.h"
#include <alloc.h>
#include <string.h>
#include <vga.h>


// Get the important filesystem info out of the BPB and return a valid FAT disk if it is in fact valid
fat_disk_t* ParseFilesystem(disk_t* disk){
    if(disk == NULL || (disk->populated == false && disk->removable == true)){
        return NULL;
    }

    bpb_t* bpb = NULL;
    bpb = (bpb_t*)ReadSectors(disk, 1, 0);

    fat_disk_t* fatDisk = (fat_disk_t*)alloc(sizeof(fat_disk_t));
    fatDisk->parent = disk;
    
    if(bpb->totalSectors == 0){
        fatDisk->volumeSectors = bpb->largeSectorCount;
    }else{
        fatDisk->volumeSectors = bpb->totalSectors;
    }

    if(bpb->sectorsPerFAT == 0){
        fatDisk->fatSize = bpb->ebr.ebr_type.fat32.sectorsPerFat;
    }else{
        fatDisk->fatSize = bpb->sectorsPerFAT;
    }

    if(bpb->rootDirEntries != 0 && bpb->bytesPerSector != 0){
        fatDisk->rootSectors = ((bpb->rootDirEntries * 32) + (bpb->bytesPerSector - 1)) / bpb->bytesPerSector;
    }else{
        fatDisk->rootSectors = 0;
    }

    fatDisk->firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->sectorsPerFAT) + fatDisk->rootSectors;

    fatDisk->firstFatSector = bpb->reservedSectors;

    fatDisk->dataSectors = bpb->totalSectors - (bpb->reservedSectors + (bpb->numFATs * bpb->sectorsPerFAT) + fatDisk->rootSectors);

    if(fatDisk->dataSectors != 0 && bpb->sectorsPerCluster != 0){
        fatDisk->totalClusters = fatDisk->dataSectors / bpb->sectorsPerCluster;
    }else{
        fatDisk->totalClusters = 0;
    }

    if(strncmp(bpb->ebr.ebr_type.fat32.systemID, "FAT32", 5)){
        fatDisk->fstype = FS_FAT32;
    }else if(strncmp(bpb->ebr.ebr_type.fat1216.fsType, "FAT12", 5)){
        fatDisk->fstype = FS_FAT12;
    }else if(strncmp(bpb->ebr.ebr_type.fat1216.fsType, "FAT16", 5)){
        fatDisk->fstype = FS_FAT16;
    }else{
        fatDisk->fstype = FS_UNSUPPORTED;
    }

    fatDisk->paramBlock = bpb;
    return fatDisk;
}

#define LFN_FIRST_ENTRY 0x01
#define LFN_LAST_ENTRY 0x40

// "It's simple and easy" they said. WHERE ARE THE FILES??????
// Find a file or directory. LFN unsupported for the time being.
fat_entry_t* SeekFile(fat_disk_t* fatdisk, char* fileName){
    if(fatdisk->fstype == FS_UNSUPPORTED){
        return NULL;
    }

    // Slice and parse the given filename
    char name[11];
    memset(name, ' ', 11);

    int i = 0;

    // Copy the filename part before the dot, up to 8 characters
    while (fileName[i] != '.' && fileName[i] != '\0' && i < 8) {
        // Get the file name part
        name[i] = toupper(fileName[i]);  // Convert to uppercase for 8.3 format compliance
        i++;
    }

    // If a dot is found, handle the extension
    if (fileName[i] == '.') {
        int extIndex = 8;  // Extensions start at the 9th position in the `name` array (index 8)
        i++;  // Move past the dot

        // Copy up to 3 characters of the extension
        for (int j = 0; j < 3 && fileName[i] != '\0'; j++, i++) {
            name[extIndex] = toupper(fileName[i]);  // Convert to uppercase for compliance
            extIndex++;
        }
    }

    // Confirms that the name was generated properly (it is)
    //WriteStrSize(&name[0], 11);
    //WriteStr("\n");

    //uint64 rootCluster = 0;
    uint64 rootSector = 0;
    if(fatdisk->fstype == FS_FAT32 || fatdisk->fstype == FS_EXFAT){
        // FAT32 root directory cluster
        uint64 rootCluster = fatdisk->paramBlock->ebr.ebr_type.fat32.rootDirCluster;
        rootSector = ((rootCluster - 2) * fatdisk->paramBlock->sectorsPerCluster) + fatdisk->firstDataSector;
        //printk("%d\n", fatdisk->paramBlock->sectorsPerCluster);
    }else{
        // FAT12/16 root directory handling
        rootSector = fatdisk->firstDataSector - fatdisk->rootSectors;
    }

    if(rootSector == 0){
        return NULL;
    }

    void* buffer = ReadSectors(fatdisk->parent, fatdisk->paramBlock->sectorsPerCluster, rootSector);
    if(buffer == NULL){
        return NULL;
    }


    size_t bytesPerCluster = fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector;
    size_t numEntries = bytesPerCluster / (sizeof(fat_entry_t));

    fat_entry_t* entry = alloc(sizeof(fat_entry_t));
    memset(entry, 0, sizeof(fat_entry_t));

    bool found = false;

    uint64 offset = 0;
    for(size_t i = 0; i < numEntries; i++){
        fat_entry_t* file = (fat_entry_t* )(buffer + offset);
        if(file->name[0] == 0){
            // End of directory entries
            printk("End of entries!\n");
            break;
        }
        if(file->name[0] == DELETED){
            if(file->attributes == 0x0F){
                offset += sizeof(lfn_entry_t);
            }else{
                offset += sizeof(fat_entry_t);
            }
            continue;
        }
        if(file->attributes == 0x0F){
            // The file is an LFN file. Process it like a regular file for now for debugging purposes.
            WriteStrSize(&file->name, 11);
            char* ptr = file;
            for(int j = 0; j < sizeof(fat_entry_t); j++){
                printk("0x%x ", (uint32)((*(ptr + j)) & 0x000000FFU));
            }
            lfn_entry_t* lfn = (lfn_entry_t*)((void*)file);
            file = &lfn->file;
            if(file->name[0] == DELETED){
                //printk("Deleted!\n");
                offset += sizeof(fat_entry_t);          // If the entry is deleted, it's there, so we have to skip it.
                continue;
            }
            // Just in case the 8.3 filename has 0 characters
            printk("LFN file found!\n");

            WriteStrSize(lfn->file.name, 11);
            printk("\n");
            ptr = file;
            for(int j = 0; j < sizeof(fat_entry_t); j++){
                printk("0x%x ", (uint32)((*(ptr + j)) & 0x000000FFU));
            }
            printk("\n");
            offset += sizeof(lfn_entry_t);
        }else{
            printk("8.3 File Found!\n");
            WriteStrSize(file->name, 11);
            printk("\n");
            char* ptr = file;
            for(int j = 0; j < sizeof(fat_entry_t); j++){
                printk("0x%x ", (uint32)((*(ptr + j)) & 0x000000FFU));
            }
            printk("\n");
            offset += sizeof(fat_entry_t);
        }
        if(file->name[0] != 0){
            if(strncmp(file->name, name, 11)){
                memcpy(entry, file, sizeof(fat_entry_t));
                found = true;
                break;
            }
        }
    }

    if(entry->fileSize == 0 || !found){
        dealloc(buffer);
        dealloc(entry);
        return NULL;
    }

    dealloc(buffer);
    return entry;
}