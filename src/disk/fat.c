#include "fat.h"
#include <alloc.h>
#include <string.h>
#include <vga.h>

#define LFN_FIRST_ENTRY 0x01
#define LFN_LAST_ENTRY 0x40


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

    fatDisk->firstFatSector = bpb->reservedSectors;

    fatDisk->dataSectors = bpb->totalSectors - (bpb->reservedSectors + (bpb->numFATs * bpb->sectorsPerFAT) + fatDisk->rootSectors);

    if(fatDisk->dataSectors != 0 && bpb->sectorsPerCluster != 0){
        fatDisk->totalClusters = fatDisk->dataSectors / bpb->sectorsPerCluster;
    }else{
        fatDisk->totalClusters = 0;
    }

    if(strncmp(bpb->ebr.ebr_type.fat32.systemID, "FAT32", 5)){
        fatDisk->fstype = FS_FAT32;
        fatDisk->firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->ebr.ebr_type.fat32.sectorsPerFat) + fatDisk->rootSectors;
    }else if(strncmp(bpb->ebr.ebr_type.fat1216.fsType, "FAT12", 5)){
        fatDisk->fstype = FS_FAT12;
        fatDisk->firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->sectorsPerFAT) + fatDisk->rootSectors;
    }else if(strncmp(bpb->ebr.ebr_type.fat1216.fsType, "FAT16", 5)){
        fatDisk->fstype = FS_FAT16;
        fatDisk->firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->sectorsPerFAT) + fatDisk->rootSectors;
    }else{
        fatDisk->fstype = FS_UNSUPPORTED;
    }

    fatDisk->paramBlock = bpb;
    return fatDisk;
}

// Start it with FAT32 then go to FAT12/16 when it's figured out
// In FAT32, the root directory is a cluster chain, which is itself full of FAT entries. The first cluster is in the BPB.
// Returns a linked list of clusters that contain the root directory entries
FAT_cluster_t* ReadRootDirectory(fat_disk_t* fatdisk){
    printk("First data sector: %u\n", fatdisk->firstDataSector);
    uint64 rootClusLBA = 0;
    uint64 rootSector = 0;
    uint64 rootCluster = 0;
    if(fatdisk->fstype == FS_FAT32 || fatdisk->fstype == FS_EXFAT){
        // FAT32 root directory cluster
        rootCluster = fatdisk->paramBlock->ebr.ebr_type.fat32.rootDirCluster;
        rootSector = (fatdisk->paramBlock->ebr.ebr_type.fat32.sectorsPerFat * fatdisk->paramBlock->numFATs) + (fatdisk->paramBlock->reservedSectors);
    }else{
        // FAT12/16 root directory handling
        rootSector = fatdisk->firstDataSector - fatdisk->rootSectors;
    }

    if(rootSector == 0){
        return NULL;
    }


    bool validCluster = true;

    size_t offset;

    void* fileBuffer = NULL;

    FAT_cluster_t* rootDir = (FAT_cluster_t*)alloc(sizeof(FAT_cluster_t));
    FAT_cluster_t* current = rootDir;

    uint32 currentCluster = rootCluster;

    while (validCluster){
        uint32 fatOffset = currentCluster * 4;
        uint32 fatSector = fatdisk->firstFatSector + (fatOffset / fatdisk->paramBlock->bytesPerSector);
        uint32 fatEntryOffset = fatOffset % fatdisk->paramBlock->bytesPerSector;
        // Get the first sector of the FAT, which should (?) contain the root directory cluster
        char* buffer = ReadSectors(fatdisk->parent, 1, fatSector);
        if(buffer == NULL){
            // Memory allocation failure
            return NULL;
        }

        // Get the next cluster in the chain
        uint32 tableValue = *((uint32*)(buffer + fatEntryOffset));
        if(fatdisk->fstype == FS_FAT32){
            tableValue &= 0x0FFFFFFF;
        }

        if(tableValue == 0){
            // The cluster is empty
            validCluster = false;
        }else if(tableValue == 0x0FFFFFF7){
            // Bad cluster (need better handling in the future but this is for reading)
            validCluster = false;
        }else if(tableValue >= 0x0FFFFFF8){
            // We have reached the end of the cluster chain
            validCluster = false;
        }

        dealloc(buffer);
        buffer = ReadSectors(fatdisk->parent, fatdisk->paramBlock->sectorsPerCluster, fatdisk->firstDataSector + (currentCluster - 2) * fatdisk->paramBlock->sectorsPerCluster);

        // There's another cluster coming up that must be read
        current->cluster = currentCluster;
        current->buffer = buffer;
        current->next = (FAT_cluster_t*)alloc(sizeof(FAT_cluster_t));
        current = current->next;
        current->next = NULL;
        currentCluster = tableValue;
    }

    current = rootDir;
    while(true){
        if(current == NULL){
            break;
        }
        size_t bytesPerCluster = fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector;
        size_t numEntries = bytesPerCluster / (sizeof(fat_entry_t));

        uint64 offset = 0;
        for(size_t i = 0; i < numEntries; i++){
            fat_entry_t* file = (fat_entry_t* )(current->buffer + offset);
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
                char* ptr = (char*)file;
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
                offset += sizeof(lfn_entry_t);
            }else{
                offset += sizeof(fat_entry_t);
            }
            if(file->name[0] != DELETED){
                // The file we're looking for was found and not deleted
                file_t* foundFile = (file_t*)alloc(sizeof(file_t));

                // Copy the file name to the VFS file
                foundFile->name = (char*)alloc(12);
                memcpy(foundFile->name, file->name, 11);
                foundFile->name[11] = '\0';
                for(int i = 0; i < 11; i++){
                    if(foundFile->name[i] == ' ' && i == 7){
                        foundFile->name[i] = '.';
                    }
                }

                uint32 firstFileCluster = file->firstClusterLow | (file->firstClusterHigh << 16);
                uint32 allClusters = file->fileSize / (fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector);       // How many clusters the file takes up

                uint64 clusterLba = firstFileCluster * fatdisk->paramBlock->sectorsPerCluster + fatdisk->firstDataSector;
                printk(foundFile->name);
            }
        }

        current = current->next;
    }

    return rootDir;
}

// "It's simple and easy" they said. WHERE ARE THE FILES??????
// Find a file or directory. LFN unsupported for the time being.
file_t* SeekFile(fat_disk_t* fatdisk, char* fileName){
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

    //uint64 rootLBA = 0;
    uint64 rootSector = 0;
    if(fatdisk->fstype == FS_FAT32 || fatdisk->fstype == FS_EXFAT){
        // FAT32 root directory cluster
        uint64 rootCluster = fatdisk->paramBlock->ebr.ebr_type.fat32.rootDirCluster;
        //rootLBA = ClusterToLBA(rootCluster, fatdisk);
        //printk("Root Directory cluster LBA: %llu\n", rootLBA);
        //rootSector = ((rootCluster - 2) * fatdisk->paramBlock->sectorsPerCluster) + fatdisk->firstDataSector;

        // I WAS LIED TO! This is the correct calculation for finding files!
        rootSector = (fatdisk->paramBlock->ebr.ebr_type.fat32.sectorsPerFat * fatdisk->paramBlock->numFATs) + (fatdisk->paramBlock->reservedSectors);
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
            char* ptr = (char*)file;
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
            offset += sizeof(lfn_entry_t);
        }else{
            offset += sizeof(fat_entry_t);
        }
        if(file->name[0] != DELETED){
            // The file we're looking for was found and not deleted
            file_t* foundFile = (file_t*)alloc(sizeof(file_t));

            // Copy the file name to the VFS file
            foundFile->name = (char*)alloc(12);
            memcpy(foundFile->name, file->name, 11);
            foundFile->name[11] = '\0';
            for(int i = 0; i < 11; i++){
                if(foundFile->name[i] == ' ' && i == 7){
                    foundFile->name[i] = '.';
                }
            }

            uint32 firstFileCluster = file->firstClusterLow | (file->firstClusterHigh << 16);
            uint32 allClusters = file->fileSize / (fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector);       // How many clusters the file takes up

            uint64 clusterLba = firstFileCluster * fatdisk->paramBlock->sectorsPerCluster + fatdisk->firstDataSector;
            printk("First cluster LBA: %llu\n", clusterLba);

            dealloc(buffer);
            buffer = ReadSectors(fatdisk->parent, fatdisk->paramBlock->sectorsPerCluster, clusterLba);

            printk("0x%x\n", *(uint64*)buffer);

            foundFile->data = NULL;         // Will load the data later
            return foundFile;
        }
    }

    dealloc(buffer);
    return NULL;
}