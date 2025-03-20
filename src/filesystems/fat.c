#include <fat.h>
#include <stdint.h>
#include <vfs.h>
#include <mbr.h>
#include <devices.h>
#include <util.h>
#include <console.h>
#include <drivers.h>
#include <string.h>

#define FAT1216
#define FAT32
#define EXFAT

// Non-integer fixed-size types
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct PACKED BIOS_Param_Block {
    // Traditional BPB
    BYTE jmpnop[3];
    char oemID[8];
    uint16_t bytesPerSector;                    // The number of bytes per sector. If exFAT, this is zero.
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t numRootDirEntries;
    uint16_t totalSectors16;                    // Is zero if totalSectors32 is used
    uint8_t mediaDescriptor;
    FAT1216 uint16_t sectorsPerFAT16;           // FAT12/16 only
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    size_t hiddenSectors;                       // The number of hidden sectors (or, alternatively, the start of the partition)
    size_t totalSectors32;                      // The number of sectors if above USHORT_MAX

    // EBR
    union PACKED {
        struct PACKED {
            FAT1216 uint8_t driveNum;           // BIOS drive number
            FAT1216 BYTE _ntReserved;           // Reserved by Windows NT
            FAT1216 uint8_t signature;          // Must be 0x28 or 0x29
            FAT1216 DWORD serialNum;            // Can be safely ignored
            FAT1216 char volumeLabel[11];       // The name of the volume
            FAT1216 char fileSystemType[8];     // Spec says not to trust this. That's kind of weird, so I'm going to ignore that. I'll have to make sure my formatter implements this.
            FAT1216 BYTE bootCode[448];         // System boot code
            FAT1216 WORD bootSig;               // Should be 0xAA55
        } fat1216;
        struct PACKED {
            FAT32 size_t sectorsPerFAT32;       // The number of sectors per FAT in a FAT32 filesystem
            FAT32 WORD flags;                   // Unknown
            FAT32 uint8_t versionMinor;         // Minor version of the FAT filesystem
            FAT32 uint8_t versionMajor;         // Major version of the FAT filesystem
            FAT32 uint32_t rootCluster;         // Usually this is 2. Contains the root directory cluster.
            FAT32 uint16_t fsInfoSector;        // Sector containing the first FSInfo structure
            FAT32 uint16_t backupBoot;          // Location of the backup boot sector
            FAT32 BYTE _reserved[12];           // Wasted space ig
            FAT32 uint8_t driveNo;              // Best to ignore this
            FAT32 BYTE _ntReserved;             // Reserved by Windows NT, unused otherwise
            FAT32 uint8_t signature;            // Must be 0x28 or 0x29
            FAT32 DWORD serialNum;              // Can be safely ignored
            FAT32 char volumeLabel[11];         // Volume label string
            FAT32 char fileSystemType[8];       // Once again, spec says don't trust. That's dumb. Why else would this be here?
            FAT32 BYTE bootCode[420];           // Bootloader code
            FAT32 WORD bootSig;                 // Should be 0xAA55
        } fat32;
    };
} PACKED FAT1216 FAT32 bpb_t;

typedef struct PACKED exFAT_BPB {
    EXFAT BYTE jmpnop[3];
    EXFAT char oemID[8];
    EXFAT BYTE _reserved0[53];
    EXFAT lba partitionOffset;                  // Offset of the partition on the disk
    EXFAT lba volumeLength;                     // In sectors(?)
    EXFAT uint32_t FAToffset;                   // Sector offset of the FAT from the start of the partition
    EXFAT uint32_t clusterHeapOffset;           // Cluster heap offset in sectors
    EXFAT uint32_t numClusters;                 // Total number of clusters in the partition
    EXFAT uint32_t rootCluster;                 // The cluster containing the root directory
    EXFAT DWORD serialNumber;                   // Serial number of the partition
    EXFAT uint16_t revision;                    // Filesystem revision
    EXFAT WORD flags;                           // Flags of some kind, idrk
    EXFAT uint8_t sectorShift;
    EXFAT uint8_t clusterShift;
    EXFAT uint8_t numFATs;
    EXFAT uint8_t driveSelect;
    EXFAT uint8_t percentUsed;                  // How much of the disk is used
    EXFAT BYTE _reserved1[7];
} EXFAT exfat_br_t;

#define FSINFO_VALID_LEAD 0x41615252
#define FSINFO_VALID_MID 0x61417272
#define FSINFO_VALID_TRAIL 0xAA550000
#define UNKNOWN_LAST_FREE_CLUSTER 0xFFFFFFFF
typedef struct PACKED FSInfo {
    DWORD leadSig;              // Lead signature
    BYTE _reserved[480];        // Wasted space but I get it
    DWORD midSig;               // Middle signature
    size_t freeClusterCount;    // The total amount of free clusters on the volume. If unknown, it must be (unfortunately) computed
    uint32_t firstFreeCluster;  // Cluster number where the filesystem driver should start searching for free clusters (I assume this just skips the root directory)
    BYTE _reserved1[12];
    DWORD trailSig;             // Trailing signature
} PACKED FAT32 fsInfo_t;


#define FAT32_TABLE_MASK 0x0FFFFFFF
#define FAT32_FINAL_CLUSTER 0x0FFFFFF8
#define FAT32_BAD_CLUSTER 0x0FFFFFF7

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

#define FAT_ENTRY_FREE 0x00                 // No entry at this location
#define FAT_ENTRY_UNUSED 0xE5               // Most likely a deleted file that can be replaced
typedef struct PACKED FAT_Directory_Entry {
    char name[11];                          // File name (8.3 format)
    uint8_t attributes;                     // File attributes
    uint8_t reserved;                       // Reserved for Windows NT
    uint8_t creationTimeTenth;              // Tenth of a second for file creation time
    uint16_t creationTime;                  // File creation time
    uint16_t creationDate;                  // File creation date
    uint16_t lastAccessDate;                // Last access date
    uint16_t firstClusterHigh;              // High 16 bits of the first cluster number
    uint16_t modificationTime;              // File modification time
    uint16_t modificationDate;              // File modification date
    uint16_t firstClusterLow;               // Low 16 bits of the first cluster number
    uint32_t fileSize;                      // Size of the file in bytes
} PACKED fat_entry_t;

typedef struct PACKED LFN_Entry {
    uint8_t order;                           // Order of the entry (must be the first entry)
    uint16_t name1[5];                       // First 5 characters of the name
    uint8_t attributes;                      // Must be 0x0F for LFN
    uint8_t type;                            // Must be 0x00
    uint8_t checksum;                        // Checksum of the file name
    uint16_t name2[6];                       // Next 6 characters of the name
    uint16_t reserved;                       // Must be 0x0000
    uint16_t name3[2];                       // Last 2 characters of the name
} PACKED lfn_entry_t;

typedef struct FAT_Cluster {
    void* data;                              // Pointer to the cluster data
    size_t size;                             // Size of the cluster in bytes
    size_t cluster;
    struct FAT_Cluster* next;                // Pointer to the next cluster (if any)
} fat_cluster_t;

char* partName = "p1";

// Reads a FAT cluster, copies all the data into a single buffer, and returns a pointer to the buffer
// Much of the code was copied and pasted from my implementation before I did this, so variable names may be incorrect
fat_entry_t* FATReadCluster(device_t* this, size_t cluster){
    if(this == NULL){
        //printk("Invalid device\n");
        return NULL; // Invalid device
    }

    if(cluster == 0){
        //printk("Invalid cluster\n");
        return NULL; // Invalid cluster
    }

    filesystem_t* fs = (filesystem_t*)this->deviceInfo;
    if(fs->fs != FS_FAT12 && fs->fs != FS_FAT16 && fs->fs != FS_FAT32){
        //printk("Unsupported filesystem\n");
        return NULL; // Filesystem unsupported
    }

    device_t* disk = fs->partition->device;
    if(disk == NULL){
        //printk("No disk found\n");
        return NULL; // No disk found
    }

    blkdev_t* blkdev = (blkdev_t*)disk->deviceInfo;
    if(blkdev == NULL){
        //printk("No block device found\n");
        return NULL; // No block device found
    }

    bpb_t* bpb = (bpb_t*)fs->fsInfo;
    if(bpb == NULL){
        //printk("No BPB found\n");
        return NULL; // Filesystem info not found
    }

    //printk("Reading cluster %u...\n", cluster);

    size_t numReads = 0;
    // Implement just FAT32 for now
    if(fs->fs == FS_FAT32){
        //uint64_t rootClusLBA = 0;
        //uint64_t rootSector = 0;
        uint64_t rootCluster = 0;
        fat_cluster_t* rootDir = NULL;

        rootCluster = cluster;
        //rootSector = offset;

        bool validCluster = true;

        size_t offset;
        fat_entry_t* fileBuffer = NULL;

        uint32_t firstFatSector = bpb->reservedSectors;

        //uint32_t rootSectors = ((bpb->numRootDirEntries * 32) + (bpb->bytesPerSector - 1)) / bpb->bytesPerSector;
        uint32_t firstDataSector = bpb->reservedSectors + (bpb->numFATs * bpb->fat32.sectorsPerFAT32);

        uint32_t totalClusters = (bpb->totalSectors32 - (bpb->reservedSectors + (bpb->numFATs * bpb->fat32.sectorsPerFAT32))) / bpb->sectorsPerCluster;

        rootDir = (fat_cluster_t*)halloc(sizeof(fat_cluster_t));
        if(rootDir == NULL){
            //printk("Failed to allocate memory for the first cluster\n");
            return NULL; // Failed to allocate memory for root directory
        }
        memset(rootDir, 0, sizeof(fat_cluster_t));
        fat_cluster_t* current = rootDir;
        uint32_t currentCluster = rootCluster;

        while(validCluster){
            uint32_t fatOffset = currentCluster * 4;
            uint32_t fatSector = firstFatSector + (fatOffset / bpb->bytesPerSector);
            uint32_t fatEntryOffset = (fatOffset % bpb->bytesPerSector);

            // Allocate the buffer to read into
            char* buffer = (char*)halloc(bpb->bytesPerSector);
            if(buffer == NULL){
                hfree(rootDir);
                //printk("Failed to allocate memory for buffer\n");
                return NULL; // Failed to allocate memory for buffer
            }
            memset(buffer, 0, bpb->bytesPerSector);

            // Read the FAT
            uint64_t* fatbuf = (uint64_t*)buffer;
            fatbuf[0] = fatSector; // Set the LBA to read
            fatbuf[1] = 1; // Set the sector count to 1
            if(device_read(disk->id, buffer, bpb->bytesPerSector) == DRIVER_FAILURE){
                hfree(buffer);
                hfree(rootDir);
                //printk("Failed to read FAT table\n");
                return NULL; // Failed to read FAT table
            }

            uint32_t tableValue = *((uint32_t*)(buffer + fatEntryOffset));
            tableValue &= FAT32_TABLE_MASK;
            if(tableValue == FAT32_FINAL_CLUSTER || tableValue == FAT32_BAD_CLUSTER || tableValue == 0 || tableValue > FAT32_FINAL_CLUSTER){
                // No more clusters to read, but everything else is still valid
                validCluster = false;
            }else if(tableValue > totalClusters){
                // Invalid cluster, stop reading
                hfree(buffer);
                hfree(rootDir);
                //printk("Invalid cluster: 0x%x\n", tableValue);
                return NULL; // Invalid cluster, the driver had an error
            }

            hfree(buffer);

            // Reallocate the buffer to read the current cluster
            char* dataBuf = halloc(bpb->bytesPerSector * bpb->sectorsPerCluster);
            if(dataBuf == NULL){
                hfree(rootDir);
                //printk("Failed to allocate memory for data buffer\n");
                return NULL; // Failed to allocate memory for buffer
            }
            memset(dataBuf, 0, bpb->bytesPerSector * bpb->sectorsPerCluster);

            //printk("Reading cluster %u at LBA %u...\n", currentCluster, firstDataSector + ((currentCluster - 2) * bpb->sectorsPerCluster));
            uint64_t* buf = (uint64_t*)dataBuf;
            buf[0] = firstDataSector + ((currentCluster - 2) * bpb->sectorsPerCluster);
            buf[1] = bpb->sectorsPerCluster;
            //printk("Reading sector %lu\n", buf[0]);
            if(device_read(disk->id, dataBuf, bpb->bytesPerSector * bpb->sectorsPerCluster) == DRIVER_FAILURE){
                hfree(buffer);
                hfree(rootDir);
                //printk("Failed to read cluster data\n");
                return NULL; // Failed to read cluster data
            }

            //printk("Cluster read! Next cluster: 0x%x\n", tableValue);

            current->cluster = currentCluster;
            current->data = dataBuf;
            if(validCluster){
                current->next = (fat_cluster_t*)halloc(sizeof(fat_cluster_t));
                if(current->next == NULL){
                    hfree(dataBuf);
                    hfree(rootDir);
                    //printk("Failed to allocate memory for next cluster\n");
                    return NULL; // Failed to allocate memory for next cluster
                }
                memset(current->next, 0, sizeof(fat_cluster_t));
                current = current->next;
                current->next = NULL;
            }
            currentCluster = tableValue;
            numReads++;
        }

        // Copy the data from all the read clusters into a single buffer
        fileBuffer = halloc(bpb->sectorsPerCluster * bpb->bytesPerSector * numReads);
        if(fileBuffer == NULL){
            hfree(rootDir);
            //printk("Failed to allocate memory for file buffer\n");
            return NULL; // Failed to allocate memory for file buffer
        }
        memset(fileBuffer, 0, bpb->sectorsPerCluster * bpb->bytesPerSector * numReads);
        current = rootDir;
        offset = 0;
        while(current != NULL){
            memcpy(fileBuffer + offset, current->data, bpb->sectorsPerCluster * bpb->bytesPerSector);
            offset += bpb->sectorsPerCluster * bpb->bytesPerSector;
            current = current->next;
        }

        // Clear the linked list of clusters
        current = rootDir;
        while(current != NULL){
            fat_cluster_t* next = current->next;
            hfree(current->data);
            hfree(current);
            current = next;
        }

        // Now we have all the clusters in a single buffer, we can parse it and turn it into a VFS directory

        fat_entry_t* currentEntry = fileBuffer;

        //printk("Successfully read %u entries from the cluster\n", numEntries);

        // Read successful!

        return fileBuffer;
    }else{
        //printk("Invalid filesystem!\n");
        return NULL; // Not implemented yet
    }

    //printk("Something went wrong!\n");
    return NULL; // Something went wrong
}

DRIVERSTATUS MountFS(device_t* this, char* mountPath) {
    filesystem_t* fs = (filesystem_t*)this->deviceInfo;
    if(fs->fs != FS_FAT12 && fs->fs != FS_FAT16 && fs->fs != FS_FAT32) {
        return DRIVER_INVALID_STATE; // Filesystem unsupported
    }
    
    device_t* disk = fs->partition->device;
    if(disk == NULL) {
        return DRIVER_INVALID_STATE; // No disk found
    }
    
    // Allocate mount point structure
    mountpoint_t* mountPoint = (mountpoint_t*)halloc(sizeof(mountpoint_t));
    if(mountPoint == NULL) {
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for mount point
    }
    memset(mountPoint, 0, sizeof(mountpoint_t));  // Zero-initialize the structure
    
    // Set up mount path
    mountPoint->mountPath = (char*)halloc(strlen(mountPath) + 1);
    if(mountPoint->mountPath == NULL) {
        hfree(mountPoint);
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for mount path
    }
    strcpy(mountPoint->mountPath, mountPath);
    
    // Find the VFS node for the mount path
    vfs_node_t* mountNode = VfsFindNode(mountPath);
    if(mountNode == NULL) {
        hfree(mountPoint->mountPath);
        hfree(mountPoint);
        return DRIVER_INVALID_ARGUMENT; // Mount path does not exist
    }
    
    // Verify needed structures
    blkdev_t* blkdev = (blkdev_t*)disk->deviceInfo;
    if(blkdev == NULL) {
        hfree(mountPoint->mountPath);
        hfree(mountPoint);
        return DRIVER_INVALID_STATE; // No block device found
    }
    
    bpb_t* bpb = (bpb_t*)fs->fsInfo;
    if(bpb == NULL) {
        hfree(mountPoint->mountPath);
        hfree(mountPoint);
        return DRIVER_INVALID_STATE; // Filesystem info not found
    }
    
    // Implement just FAT32 for now
    if(fs->fs == FS_FAT32) {
        // Read the root directory
        uint64_t rootCluster = bpb->fat32.rootCluster;
        fat_entry_t* entries = FATReadCluster(this, rootCluster);
        if(entries == NULL) {
            hfree(mountPoint->mountPath);
            hfree(mountPoint);
            return DRIVER_FAILURE;
        }
        
        // Clear existing children from the mount node
        vfs_node_t* rootVfsNode = mountNode->firstChild;
        while(rootVfsNode != NULL) {
            vfs_node_t* next = rootVfsNode->next;
            VfsRemoveNode(rootVfsNode);
            rootVfsNode = next;
        }
        mountNode->firstChild = NULL;
        
        // Count valid entries in the directory
        size_t numEntries = 0;
        bool validEntry = true;
        while(validEntry) {
            if(entries[numEntries].name[0] == FAT_ENTRY_FREE) {
                validEntry = false;
                break;
            } else if((uint8_t)entries[numEntries].name[0] == FAT_ENTRY_UNUSED) {
                numEntries++;
                continue;
            }
            
            numEntries++;
            if(numEntries >= (bpb->sectorsPerCluster * bpb->bytesPerSector) / sizeof(fat_entry_t)) {
                validEntry = false;
                break;
            }
        }
        
        // Process each directory entry
        fat_entry_t* currentEntry = entries;
        size_t numRootEntries = 0;

        mountNode->mountPoint = mountPoint;
        
        for(size_t j = 0; j < numEntries; j++) {
            if(currentEntry->name[0] == FAT_ENTRY_FREE || 
               (uint8_t)currentEntry->name[0] == FAT_ENTRY_UNUSED) {
                currentEntry++;
                continue;
            }
            
            // Convert FAT name to readable format
            char fileName[13];
            memset(fileName, 0, 13);
            int i = 0;
            while(currentEntry->name[i] != ' ' && i < 8){
                fileName[i] = currentEntry->name[i];
                i++;
            }

            // Get the extension
            int nameOffset = i;
            while(currentEntry->name[i] == ' '){
                i++;
            }

            if(currentEntry->name[i] != ' ' && i < 11){
                fileName[nameOffset++] = '.';
                while(currentEntry->name[i] != ' ' && i < 11){
                    fileName[i] = currentEntry->name[i];
                    i++;
                }
            }
            fileName[12] = '\0';
            
            // Create VFS node
            vfs_node_t* newNode = NULL;
            if(currentEntry->attributes & FAT_ATTR_DIRECTORY) {
                newNode = VfsMakeNode(strdup(fileName), true, false, false, false, 0, 0755, ROOT_UID, NULL);
            } else {
                newNode = VfsMakeNode(strdup(fileName), false, false, true, true, currentEntry->fileSize, 0644, ROOT_UID, NULL);
            }
            
            if(newNode == NULL) {
                hfree(entries);
                hfree(mountPoint->mountPath);
                hfree(mountPoint);
                return DRIVER_OUT_OF_MEMORY;
            }
            
            // Set up the mountpoint relationship
            VfsAddChild(mountNode, newNode);
            
            currentEntry++;
            numRootEntries++;
        }
        
        // Set up the mount node properties
        mountNode->size = numRootEntries;
        mountNode->isDirectory = true;
        mountNode->readOnly = false;
        mountNode->writeOnly = false;
        mountNode->isResizeable = false;
        mountNode->permissions = 0755;
        //mountNode->mountPoint = mountPoint;

        mountPoint->filesystem = fs;
        mountPoint->device = this;
        
        fs->device = this;
        fs->mountPoint = mountPoint;
        fs->fsInfo = bpb;
        
        hfree(entries);  // Free the directory entries buffer we no longer need
        
        printk("Mountpoint address in driver: 0x%x\n", mountPoint);
        //STOP
        return DRIVER_SUCCESS;
    } else {
        hfree(mountPoint->mountPath);
        hfree(mountPoint);
        return DRIVER_NOT_SUPPORTED; // Not implemented yet
    }
}

// Buffer should contain the whole path to the file to read
DRIVERSTATUS ReadFile(device_t* this, void* buffer, size_t size){
    //printk("Reading file...\n");
    //printk("Buffer: %s\n", (char*)buffer);
    filesystem_t* fs = (filesystem_t*)this->deviceInfo;
    if(fs->mountPoint == NULL || (fs->fs != FS_FAT12 && fs->fs != FS_FAT16 && fs->fs != FS_FAT32)){
        //printk("Filesystem is not mounted or is not a FAT filesystem!\n");
        return DRIVER_INVALID_STATE; // Filesystem is not mounted or is not a FAT filesystem
    }

    device_t* disk = fs->partition->device;
    if(disk == NULL){
        //printk("No disk found!\n");
        return DRIVER_INVALID_STATE; // No disk found
    }

    blkdev_t* blkdev = (blkdev_t*)disk->deviceInfo;
    if(blkdev == NULL){
        //printk("No block device found!\n");
        return DRIVER_INVALID_STATE; // No block device found
    }

    bpb_t* bpb = (bpb_t*)fs->fsInfo;
    if(bpb == NULL){
        //printk("No BPB found!\n");
        return DRIVER_INVALID_STATE; // Filesystem info not found
    }

    mountpoint_t* mountpoint = fs->mountPoint;
    if(mountpoint == NULL){
        //printk("No mount point found!\n");
        return DRIVER_INVALID_STATE; // No mount point found
    }

    vfs_node_t* readInto = VfsFindNode((char*)buffer);
    if(readInto == NULL){
        //printk("File %s not found!\n", (char*)buffer);
        return DRIVER_INVALID_ARGUMENT; // File not found
    }

    if(readInto == NULL){
        printk("File %s not found!\n", (char*)buffer);
        //hfree(fullPath);
        STOP
        return DRIVER_INVALID_ARGUMENT; // File not found
    }
    if(readInto->isDirectory){
        //printk("Cannot read a directory!\n");
        return DRIVER_INVALID_ARGUMENT; // Cannot read a directory
    }

    //hfree(fullPath);
    
    //return 0;

    // Implement just FAT32 for now
    if(fs->fs == FS_FAT32){
        // Only support the root directory for now
        char* fileName = strrchr((char*)buffer, '/') + 1;
        if(strcmp(fileName, readInto->name) != 0){
            //printk("File name is not in the disk's root directory!\n");
            return DRIVER_INVALID_ARGUMENT; // File name does not match
        }
        uint32_t rootCluster = bpb->fat32.rootCluster;
        fat_entry_t* entries = FATReadCluster(this, rootCluster);
        if(entries == NULL){
            printk("Failed to read root cluster!\n");
            return DRIVER_FAILURE; // Failed to read cluster
        }
        // Read for the file in the root directory
        
        // Convert the file name into uppercase and into 8.3 format
        char upperFileName[12];
        memset(upperFileName, ' ', 12);
        char* newName = strdup(fileName);
        char* dot = strchr(newName, '.');
        if(dot != NULL){
            *dot = ' ';
        }
        for(int i = 0; i < 8; i++){
            if(newName[i] == ' '){
                upperFileName[i] = ' ';
            }else{
                upperFileName[i] = toupper(newName[i]);
            }
        }
        for(int i = 8; i < 11; i++){
            if(newName[i] == ' '){
                upperFileName[i] = ' ';
            }else{
                upperFileName[i] = newName[i];
            }
        }

        hfree(newName);
        upperFileName[11] = '\0';
        bool found = false;

        size_t currentEntry = 0;
        while(!found){
            if(entries[currentEntry].name[0] == FAT_ENTRY_FREE){
                // Not found
                break;
            }else if((uint8_t)(entries[currentEntry].name[0]) == FAT_ENTRY_UNUSED){
                // Deleted entry, we can ignore this
                currentEntry++;
                continue;
            }

            // We have a valid entry here
            if(strncmp(entries[currentEntry].name, upperFileName, 11) == 0){
                // Found the file!
                found = true;
            }
            currentEntry++;
        }

        if(found){
            // We have found the file, now we need to read its data
            //printk("We found a file!\n");

            size_t fileCluster = (entries[currentEntry - 1].firstClusterLow | (entries[currentEntry - 1].firstClusterHigh << 16));
            char* fileBuffer = (char*)FATReadCluster(this, fileCluster);
            if(fileBuffer == NULL){
                //printk("Failed to read file cluster!\n");
                return DRIVER_FAILURE; // Failed to read file cluster
            }
            // Now we have the file in a buffer, we can copy it to the output buffer
            readInto->data = (void*)fileBuffer;
            hfree(entries);
            return DRIVER_SUCCESS; // File successfully read
        }else{
            //printk("File not found!\n");
            //printk("Tried to find: %s\n", upperFileName);
            return DRIVER_FAILURE;
        }
    }else{
        // Implement FAT12 and FAT16
        //printk("Invalid filesystem!\n");
        return DRIVER_NOT_SUPPORTED; // Not implemented yet
    }
}

// TODO: actually detect and validate a FAT filesytstem
DRIVERSTATUS ProbeFATFilesystem(device_t* device){
    //printf("Probing...\n");
    bool validFatFs = false;
    if(device->type == DEVICE_TYPE_BLOCK){
        // Check if the device is a FAT filesystem
        if(((blkdev_t*)device->deviceInfo)->firstPartition != NULL){
            // This device has a partition, check if it's FAT
            partition_t* partition = ((blkdev_t*)device->deviceInfo)->firstPartition;
            while(partition != NULL){
                if(partition->fsID == ID_FAT12 || partition->fsID == ID_SMALL_FAT16 || partition->fsID == ID_BIG_FAT16 || partition->fsID == ID_FAT32 || partition->fsID == ID_FAT32_LBA || partition->fsID == ID_EXFAT){
                    // Make a device entry in the VFS for each partition, saying this driver supports it...
                    return DRIVER_INITIALIZED; // This driver supports this device, the kernel can assign it to the device
                }
                partition = partition->next;
            }
            //printf("No FAT filesystem found on this device!\n");
            return DRIVER_NOT_SUPPORTED; // No FAT filesystem found on this device
        }else{
            // Check BPB
            mbr_t* mbr = (mbr_t*)halloc(sizeof(mbr_t));
            if(mbr == NULL){
                //printf("Out of mnemory!\n");
                return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for MBR
            }
            memset(mbr, 0, sizeof(mbr));

            uintptr_t addr = (uintptr_t)mbr;

            uint64_t* buf = (uint64_t*)addr;
            buf[0] = 0;                                                     // Set the LBA to read
            buf[1] = 1;                                                     // Set the sector count to 1
            if(device_read(device->id, (void*)mbr, sizeof(mbr_t)) == DRIVER_FAILURE){
                //printk("Read failed!\n");
                hfree(mbr);
                return DRIVER_NOT_SUPPORTED; // Failed to read MBR
            }
            //do_syscall(SYS_DEVICE_READ, device->id, (uint32_t)mbr, sizeof(mbr_t), 0, 0);
            if(IsValidMBR(mbr)){
                // Cast the MBR to a BPB instead
                bpb_t* bpb = (bpb_t*)mbr;
                filesystem_t* fs = (filesystem_t*)halloc(sizeof(filesystem_t));
                partition_t* partition = (partition_t*)halloc(sizeof(partition_t));
                memset(partition, 0, sizeof(partition_t));
                memset(fs, 0, sizeof(filesystem_t));

                if(strncmp(bpb->fat1216.fileSystemType, "FAT12   ", 8) == 0){
                    // We have found a valid FAT12 filesystem!
                    partition->start = bpb->hiddenSectors;
                    partition->end = partition->start + bpb->totalSectors16;
                    partition->size = bpb->totalSectors16;
                    partition->type = "FAT12";
                    partition->device = device;
                    partition->blkdev = (blkdev_t*)device->deviceInfo;
                    partition->fsID = FS_FAT12;
                    partition->next = NULL;
                    partition->previous = NULL;
                    fs->partition = partition;
                    fs->device = device;
                    fs->devName = (char*)halloc(7);
                    memset(fs->devName, 0, 7);
                    strcpy(fs->devName, device->devName);
                    strcat(fs->devName, partName);
                    fs->fsInfo = bpb;
                    fs->name = "FAT12 Filesystem";
                    fs->volumeName = (char*)halloc(12);
                    memset(fs->volumeName, 0, 12);
                    strncpy(fs->volumeName, bpb->fat1216.volumeLabel, 11);
                    fs->volumeName[11] = '\0';
                    fs->mountPoint = NULL;
                    validFatFs = true;
                    fs->fs = FS_FAT12;
                    fs->mount = MountFS;
                }

                if(strncmp(bpb->fat1216.fileSystemType, "FAT16   ", 8) == 0){
                    // We have found a valid FAT16 filesystem!
                    partition->start = bpb->hiddenSectors;
                    partition->end = partition->start + bpb->totalSectors16;
                    partition->size = bpb->totalSectors16;
                    partition->type = "FAT16";
                    partition->device = device;
                    partition->blkdev = (blkdev_t*)device->deviceInfo;
                    partition->fsID = FS_FAT16;
                    partition->next = NULL;
                    partition->previous = NULL;
                    fs->partition = partition;
                    fs->device = device;
                    fs->devName = (char*)halloc(7);
                    memset(fs->devName, 0, 7);
                    strcpy(fs->devName, device->devName);
                    strcat(fs->devName, partName);
                    fs->fsInfo = bpb;
                    fs->name = "FAT16 Filesystem";
                    fs->volumeName = (char*)halloc(12);
                    memset(fs->volumeName, 0, 12);
                    strncpy(fs->volumeName, bpb->fat1216.volumeLabel, 11);
                    fs->volumeName[11] = '\0';
                    fs->mountPoint = NULL;
                    validFatFs = true;
                    fs->fs = FS_FAT16;
                    fs->mount = MountFS;
                }

                if(strncmp(bpb->fat32.fileSystemType, "FAT32   ", 8) == 0){
                    // We have found a FAT32 filesystem!
                    partition->start = bpb->hiddenSectors;
                    partition->end = partition->start + bpb->totalSectors16;
                    partition->size = bpb->totalSectors16;
                    partition->type = "FAT32";
                    partition->device = device;
                    partition->blkdev = (blkdev_t*)device->deviceInfo;
                    partition->fsID = FS_FAT32;
                    partition->next = NULL;
                    partition->previous = NULL;
                    fs->partition = partition;
                    fs->device = device;
                    fs->devName = (char*)halloc(7);
                    memset(fs->devName, 0, 7);
                    strcpy(fs->devName, device->devName);
                    strcat(fs->devName, partName);
                    fs->fsInfo = bpb;
                    fs->name = "FAT32 Filesystem";
                    fs->volumeName = (char*)halloc(12);
                    memset(fs->volumeName, 0, 12);
                    strncpy(fs->volumeName, bpb->fat32.volumeLabel, 11);
                    fs->volumeName[11] = '\0';
                    fs->mountPoint = NULL;
                    validFatFs = true;
                    fs->fs = FS_FAT32;
                    fs->mount = MountFS;
                }

                // Debug output
                //WriteStringSize(bpb->fat32.fileSystemType, 8);
                //STOP

                if(validFatFs){
                    // Do all this for each FAT partition...
                    char* name = (char*)halloc(7);
                    memset(name, 0, 7);
                    strcpy(name, device->devName);
                    strcat(name, partName);                 // No need to manipulate partName since no MBR partitioning means only one partition

                    partition_t* partition = (partition_t*)halloc(sizeof(partition_t));
                    memset(partition, 0, sizeof(partition_t));
                    device_t* fsDevice = CreateDevice("FAT Filesystem", name, "FAT Filesystem", fs, NULL/*The kernel will assign this*/, DEVICE_TYPE_FILESYSTEM, 0, (device_flags_t){0}, ReadFile, NULL, NULL, device);
                    device->firstChild = fsDevice;              // Set the first child of the device to be the filesystem
                    register_device(fsDevice);                  // Register the device with the kernel
                    add_vfs_device(fsDevice, name, "/dev");     // Add the device to the VFS
                    return DRIVER_INITIALIZED;
                }

                hfree(mbr);
                if(fs->devName != NULL){
                    hfree((char*)fs->devName);
                }
                if(fs->volumeName != NULL){
                    hfree((char*)fs->volumeName);
                }
                if(fs != NULL){
                    hfree(fs);
                }
                if(partition != NULL){
                    hfree(partition);
                }
                return DRIVER_NOT_SUPPORTED;
            }else{
                //printf("Invalid MBR!\n");
                return DRIVER_NOT_SUPPORTED; // This driver does not support this device
            }
        }
    }
    //printf("Not a block device!\n");
    return DRIVER_NOT_SUPPORTED; // This driver does not support this device
}

// FAT driver initialization
DRIVERSTATUS InitializeFAT(){
    driver_t* fatDriver = CreateDriver("FAT", "FAT Filesystem Driver", 0x0001, DEVICE_TYPE_FILESYSTEM, NULL, NULL, ProbeFATFilesystem);
    if(fatDriver == NULL){
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for FAT driver
    }
    module_load(fatDriver, NULL);
    //do_syscall(SYS_MODULE_LOAD, (uint32_t)fatDriver, 0, 0, 0, 0);
    //RegisterDriver(fatDriver, NULL);
    return DRIVER_SUCCESS;
}