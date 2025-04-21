#include <fat.h>
#include <stdint.h>
#include <vfs.h>
#include <devices.h>
#include <util.h>
#include <console.h>
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
    EXFAT lba_t partitionOffset;                // Offset of the partition on the disk
    EXFAT lba_t volumeLength;                   // In sectors(?)
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

typedef enum {
    FS_UNSUPPORTED,
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
    FS_EXFAT
} validfs_t;

// Internal struct to keep track of supported devices
typedef struct Supported_Device {
    device_id_t id;                     // The device ID of the parition device
    vfs_node_t* node;                   // The VFS node of this device
    vfs_node_t* mountPoint;             // The mount point of this device (if any)
    mountpoint_t* mount;                // The mount point structure of this device (if any)
    validfs_t fs;                       // The filesystem type of this device

    size_t clusterSize;                 // The size of a cluster in sectors
    size_t clusterCount;                // The number of clusters on this device
    size_t sectorSize;                  // The size of a sector in bytes
    size_t numDataSectors;              // The number of data sectors on this device
    lba_t firstFatSector;               // The first sector of the FAT
    lba_t firstDataSector;              // The first sector of the data area
    lba_t numSectors;                   // The number of sectors on this device
    size_t numRootDirSectors;           // The number of sectors in the root directory (if applicable)
    size_t fatSize;                     // The size of the FAT in sectors

    struct Supported_Device* next;      // Pointer to the next supported device
} supported_device_t;

supported_device_t* firstSupportedDevice = NULL; // Pointer to the first supported device

static fs_driver_t* this = NULL;

// If this driver needs to define a partition device, it should be done here
char* partitionName = "p1";

dresult_t AddSupportedDevice(supported_device_t* device){
    if(device == NULL){
        return DRIVER_FAILURE;
    }

    // Add the device to the list of supported devices
    if(firstSupportedDevice == NULL){
        firstSupportedDevice = device;
    }else{
        supported_device_t* current = firstSupportedDevice;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = device;
    }

    return DRIVER_SUCCESS;
}

supported_device_t* GetSupportedDevice(device_id_t device){
    // Get the supported device by ID
    supported_device_t* current = firstSupportedDevice;
    while(current != NULL){
        if(current->id == device){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

uint8_t* ReadClusters(device_id_t device, size_t cluster){
    // Get the cluster from the device
    supported_device_t* dev = GetSupportedDevice(device);
    if(dev == NULL || dev->mount == NULL || cluster >= dev->clusterCount){
        return NULL;
    }

    size_t numReads = 0;
    if(dev->fs == FS_FAT32){
        // FAT32 filesystem
        size_t firstCluster = cluster;
        fat_cluster_t* clusterData = halloc(sizeof(fat_cluster_t));
        if(clusterData == NULL){
            return NULL;
        }
        memset(clusterData, 0, sizeof(fat_cluster_t));

        bool validCluster = true;

        fat_cluster_t* current = clusterData;
        size_t currentCluster = firstCluster;

        while(validCluster){
            uint32_t fatOffset = currentCluster * 4;
            lba_t fatSector = (fatOffset / dev->sectorSize) + dev->firstFatSector;
            uint32_t fatEntryOffset = fatOffset % dev->sectorSize;

            uint8_t* buffer = halloc(dev->sectorSize);
            if(buffer == NULL){
                hfree(clusterData);
                return NULL;
            }
            memset(buffer, 0, dev->sectorSize);

            uint64_t* fatBuf = (uint64_t*)buffer;
            fatBuf[0] = fatSector;
            fatBuf[1] = 1;
            if(dev->mount->blockDevice->ops.read(dev->mount->blockDevice->id, buffer, dev->sectorSize, fatSector) == DRIVER_FAILURE){
                hfree(buffer);
                hfree(clusterData);
                return NULL;
            }

            uint32_t fatEntry = *(uint32_t*)(buffer + fatEntryOffset);
            fatEntry &= FAT32_TABLE_MASK;
            if(fatEntry == FAT32_FINAL_CLUSTER || fatEntry == FAT32_BAD_CLUSTER || fatEntry > dev->clusterCount){
                // End of the chain
                validCluster = false;
            }

            hfree(buffer);

            uint64_t* dataBuf = halloc(dev->sectorSize * dev->clusterSize);
            if(dataBuf == NULL){
                hfree(clusterData);
                return NULL;
            }
            memset(dataBuf, 0, dev->sectorSize * dev->clusterSize);

            lba_t dataSector = ((currentCluster - 2) * dev->clusterSize) + dev->firstDataSector;
            dataBuf[0] = dataSector;
            dataBuf[1] = dev->clusterSize;
            if(dev->mount->blockDevice->ops.read(dev->mount->blockDevice->id, dataBuf, dev->sectorSize * dev->clusterSize, dataSector) == DRIVER_FAILURE){
                hfree(dataBuf);
                hfree(clusterData);
                return NULL;
            }

            current->cluster = currentCluster;
            current->data = dataBuf;
            if(validCluster){
                current->next = halloc(sizeof(fat_cluster_t));
                if(current->next == NULL){
                    hfree(dataBuf);
                    hfree(clusterData);
                    return NULL;
                }
                memset(current->next, 0, sizeof(fat_cluster_t));
                current = current->next;
                current->next = NULL;
            }

            currentCluster = fatEntry;
            numReads++;
        }

        uint8_t* fileBuffer = halloc(numReads * dev->sectorSize * dev->clusterSize);
        if(fileBuffer == NULL){
            hfree(clusterData);
            return NULL;
        }
        memset(fileBuffer, 0, numReads * dev->sectorSize * dev->clusterSize);
        current = clusterData;
        size_t offset = 0;
        while(current != NULL){
            memcpy(fileBuffer + offset, current->data, dev->sectorSize * dev->clusterSize);
            offset += dev->sectorSize * dev->clusterSize;
            fat_cluster_t* next = current->next;
            hfree(current->data);
            hfree(current);
            current = next;
        }

        return fileBuffer;
    }else{
        return NULL;
    }

    // This should never be reached
    return NULL;
}

dresult_t Sync(device_id_t device){
    // Sync the filesystem to the disk
    return DRIVER_FAILURE;
}

dresult_t Delete(char* path){
    // Delete a file from the filesystem
    return DRIVER_FAILURE;
}

dresult_t Mount(device_id_t device, char* path){
    // Mount the filesystem at the given path
    supported_device_t* dev = GetSupportedDevice(device);
    if(dev == NULL || dev->mountPoint != NULL || *path != '/'){
        // Invalid device, already mounted, or invalid path (path must be absolute from the VFS root)
        return DRIVER_FAILURE;
    }

    this->driver.busy = true;

    // Create a new mount point
    mountpoint_t* mount = halloc(sizeof(mountpoint_t));
    if(mount == NULL){
        return DRIVER_FAILURE;
    }
    memset(mount, 0, sizeof(mountpoint_t));

    mount->fsDevice = GetDeviceByID(device);
    mount->blockDevice = GetDeviceByID(mount->fsDevice->parent);
    mount->fsDriver = this;
    mount->mountRoot = VfsFindNode(path);
    if(mount->mountRoot == NULL){
        // Invalid mount point
        hfree(mount);
        return DRIVER_FAILURE;
    }

    dev->mountPoint = mount->mountRoot;
    dev->mount = mount;
    dev->mountPoint->mountPoint = mount;
    dev->mountPoint->flags |= NODE_FLAG_MOUNTED;

    uint64_t* buf = halloc(dev->sectorSize);
    if(buf == NULL){
        hfree(mount);
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }
    memset(buf, 0, dev->sectorSize);

    // Read the first sector of the device
    buf[0] = 0;
    buf[1] = 1;
    dresult_t res = mount->blockDevice->ops.read(mount->blockDevice->id, buf, dev->sectorSize, 0);
    if(res == DRIVER_FAILURE || ((bpb_t*)(buf))->fat32.bootSig != 0xAA55){
        // Invalid boot signature
        hfree(buf);
        hfree(mount);
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }

    bpb_t* bpb = (bpb_t*)buf;

    if(dev->fs == FS_FAT32){
        // FAT32 filesystem
        size_t rootCluster = bpb->fat32.rootCluster;
        fat_entry_t* entries = (fat_entry_t*)ReadClusters(device, rootCluster);
        if(entries == NULL){
            // Failed to read the root directory
            hfree(bpb);
            hfree(mount);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }

        // Clear existing children from the mount node
        vfs_node_t* childNode = dev->mountPoint->firstChild;
        while(childNode != NULL) {
            vfs_node_t* next = childNode->next;
            VfsRemoveNode(childNode);
            childNode = next;
        }
        dev->mountPoint->firstChild = NULL;

        size_t numEntries = 0;
        bool validEntry = true;
        while(validEntry){
            if(entries[numEntries].name[0] == FAT_ENTRY_FREE){
                // No more entries
                validEntry = false;
            }else{
                numEntries++;
            }
        }

        fat_entry_t* currentEntry = entries;
        for(size_t i = 0; i < numEntries; i++){
            if((uint8_t)currentEntry[i].name[0] == FAT_ENTRY_UNUSED){
                // Deleted file
                continue;
            }
            
            // TODO: Support LFN entries
            if(currentEntry[i].attributes & FAT_ATTR_LFN){
                // Long file name entry
                continue;
            }
            char name[13];
            memset(name, 0, 13);
            memcpy(name, currentEntry[i].name, 11);

            int j = 0;
            while(currentEntry->name[j] != ' ' && j < 8){
                name[j] = currentEntry->name[j];
                j++;
            }

            int nameOffset = j;
            bool hasExtension = false;
            for(j = 8; j < 11; j++){
                if(currentEntry->name[j] != ' '){
                    hasExtension = true;
                    break;
                }
            }

            j = 8;
            if(hasExtension){
                name[nameOffset++] = '.';
                while(j < 11){
                    if(currentEntry->name[j] != ' '){
                        name[nameOffset++] = currentEntry->name[j];
                    }
                    j++;
                }
            }
            name[nameOffset] = '\0';


            vfs_node_t* newNode = NULL;
            if(currentEntry[i].attributes & FAT_ATTR_DIRECTORY){
                // Directory entry
                newNode = VfsMakeNode(name, NODE_FLAG_DIRECTORY | NODE_FLAG_MOUNTED | NODE_FLAG_NOTREAD, 0, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP, ROOT_UID, 0);
            }else{
                // File entry
                newNode = VfsMakeNode(name, NODE_FLAG_MOUNTED | NODE_FLAG_NOTREAD, currentEntry[i].fileSize, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP, ROOT_UID, 0);
            }

            if(newNode == NULL){
                // Failed to create the node
                hfree(entries);
                hfree(bpb);
                hfree(mount);
                this->driver.busy = false;
                return DRIVER_FAILURE;
            }

            VfsAddChild(dev->mountPoint, newNode);
            currentEntry++;
        }

        dev->mountPoint->flags &= ~NODE_FLAG_NOTREAD;
        dev->mountPoint->flags |= NODE_FLAG_MOUNTED;
        
        hfree(entries);
        this->driver.busy = false;
        return DRIVER_SUCCESS;
    }else{
        // Unsupported filesystem
        hfree(bpb);
        hfree(mount);
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }

    this->driver.busy = false;
    return DRIVER_FAILURE;
}

dresult_t Unmount(device_id_t device, char* path){
    // Unmount the filesystem at the given path
    return DRIVER_FAILURE;
}

dresult_t ReadFile(device_id_t device, void* buf, size_t len, size_t offset){
    // Read a file from the filesystem
    supported_device_t* dev = GetSupportedDevice(device);
    if(dev == NULL){
        return DRIVER_FAILURE;
    }

    // Check if the device is mounted
    if(dev->mount == NULL){
        return DRIVER_FAILURE;
    }

    vfs_node_t* readInto = VfsFindNode((char*)buf);
    if(readInto == NULL || (readInto->flags & NODE_FLAG_DIRECTORY)){
        // Invalid path or trying to read into a directory
        return DRIVER_FAILURE;
    }

    mountpoint_t* mount = dev->mount;
    this->driver.busy = true;

    if(dev->fs == FS_FAT32){
        // FAT32 filesystem
        char* fileName = strrchr((char*)buf, '/') + 1;
        if(strcmp(fileName, readInto->name) != 0){
            // Invalid file name
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }
        
        uint64_t* buf = halloc(dev->sectorSize);
        if(buf == NULL){
            hfree(buf);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }
        memset(buf, 0, dev->sectorSize);

        // Read the first sector of the device
        bpb_t* bpb = (bpb_t*)buf;
        buf[0] = 0;
        buf[1] = 1;
        dresult_t res = mount->blockDevice->ops.read(device, buf, dev->sectorSize, 0);
        if(res == DRIVER_FAILURE || bpb->fat32.bootSig != 0xAA55){
            // Invalid boot signature
            hfree(bpb);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }

        size_t rootCluster = bpb->fat32.rootCluster;
        fat_entry_t* entries = (fat_entry_t*)ReadClusters(device, rootCluster);
        if(entries == NULL){
            // Failed to read the root directory
            hfree(bpb);
            return DRIVER_FAILURE;
        }

        char upperFileName[12];
        memset(upperFileName, ' ', 12);
        char* newName = strdup(fileName);
        if(newName == NULL){
            hfree(entries);
            hfree(bpb);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }

        char* extension = strchr(newName, '.');
        if(extension != NULL){
            *extension = '\0';
            extension++;
        }
        char name[9] = {0};
        char ext[4] = {0};

        // Split filename into name and extension parts
        if (extension != NULL) {
            *extension = '\0';  // Split the string at the '.'
            extension++;        // Move past the dot to the actual extension
            
            // Copy up to 3 chars from extension
            size_t extLen = strlen(extension);
            size_t i;
            for (i = 0; i < 3 && i < extLen; i++) {
                ext[i] = toupper(extension[i]);
            }
        }

        // Copy up to 8 chars from name part
        size_t nameLen = strlen(newName);
        size_t i;
        for (i = 0; i < 8 && i < nameLen; i++) {
            name[i] = toupper(newName[i]);
        }

        // Copy name part (first 8 chars)
        for (i = 0; i < 8; i++) {
            upperFileName[i] = (i < strlen(name)) ? name[i] : ' ';
        }

        // Copy extension part (last 3 chars)
        for (i = 0; i < 3; i++) {
            upperFileName[i+8] = (i < strlen(ext)) ? ext[i] : ' ';
        }

        hfree(newName);
        upperFileName[11] = '\0'; // Terminate the string
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
            char* fileBuffer = (char*)ReadClusters(device, fileCluster);
            if(fileBuffer == NULL){
                //printk("Failed to read file cluster!\n");
                this->driver.busy = false;
                return DRIVER_FAILURE; // Failed to read file cluster
            }
            // Now we have the file in a buffer, we can copy it to the output buffer
            readInto->data = (void*)fileBuffer;
            hfree(entries);
            this->driver.busy = false;
            return DRIVER_SUCCESS; // File successfully read
        }else{
            //printk("File not found!\n");
            //printk("Tried to find: %s\n", upperFileName);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }
    }else{
        // Unsupported filesystem
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }
}

dresult_t WriteFile(device_id_t device, void* buf, size_t len, size_t offset){
    // Write a file to the filesystem
    supported_device_t* dev = GetSupportedDevice(device);
    if(dev == NULL){
        return DRIVER_FAILURE;
    }

    vfs_node_t* node = (vfs_node_t*)buf;
    if(node == NULL){
        return DRIVER_FAILURE;
    }

    // Check if the device is mounted
    if(dev->mount == NULL){
        return DRIVER_FAILURE;
    }

    this->driver.busy = true;



    this->driver.busy = false;
    return DRIVER_FAILURE;
}

ssize_t HandleIoctl(int cmd, void* argp, device_id_t device){
    // Handle IOCTL requests
    supported_device_t* dev = GetSupportedDevice(device);
    if(dev == NULL){
        return DRIVER_FAILURE;
    }

    switch(cmd){
        case 1:{
            return DRIVER_FAILURE;
        }
        default:{
            return DRIVER_FAILURE;
        }
    }
}

dresult_t PollHandle(void* file, short* revents){
    // Poll the device for events
    return DRIVER_FAILURE;
}

dresult_t ProbeFs(device_id_t device, unsigned int deviceClass, unsigned int deviceType){
    // Check if the device is a valid storage device
    if(!(deviceType & DEVICE_TYPE_STORAGE)){
        printk("Device is not a storage device!\n");
        return DRIVER_FAILURE;
    }

    device_t* dev = GetDeviceByID(device);
    if(dev == NULL){
        printk("Device not found!\n");
        return DRIVER_FAILURE;
    }

    if(!(dev->class & DEVICE_CLASS_BLOCK)){
        printk("Device is not a block device!\n");
        return DRIVER_FAILURE;
    }

    this->driver.busy = true;

    // Lock the device for exclusive access (TODO: add a device locking function in the kernel API struct for drivers to lock other devices)
    MLock(&dev->lock, GetCurrentProcess());

    // Get the sector size of the device
    size_t sectorSize = (size_t)dev->ops.ioctl(3, NULL, device);
    if((ssize_t)sectorSize == DRIVER_FAILURE || sectorSize > 512 || sectorSize < 512){
        // Most likely not a valid filesystem device or the sector size is invalid for a FAT filesystem
        MUnlock(&dev->lock);
        this->driver.busy = false;
        printk("Invalid sector size!\n");
        return DRIVER_FAILURE;
    }

    uint64_t* buf = halloc(sectorSize);
    bpb_t* bpb = (bpb_t*)buf;
    if(bpb == NULL){
        MUnlock(&dev->lock);
        this->driver.busy = false;
        printk("Failed to allocate memory for the BPB!\n");
        return DRIVER_FAILURE;
    }
    memset(bpb, 0, sectorSize);

    // Read the first sector of the device
    buf[0] = 0;
    buf[1] = 1;
    dresult_t res = dev->ops.read(device, buf, sectorSize, 0);
    if(res == DRIVER_FAILURE){
        printk("Failed to read the first sector of the device!\n");
        hfree(bpb);
        MUnlock(&dev->lock);
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }

    // Check the filesystem for support
    if(bpb->fat32.bootSig != 0xAA55){
        // Invalid boot signature
        printk("Invalid boot signature!\n");
        hfree(bpb);
        MUnlock(&dev->lock);
        this->driver.busy = false;
        return DRIVER_FAILURE;
    }

    // This driver expects this string to be accurate.
    if(strncmp(bpb->fat32.fileSystemType, "FAT32   ", 8) == 0){
        // FAT32 filesystem
        supported_device_t* supportedDevice = halloc(sizeof(supported_device_t));
        if(supportedDevice == NULL){
            printk("Failed to allocate memory for the supported device!\n");
            hfree(bpb);
            MUnlock(&dev->lock);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }
        memset(supportedDevice, 0, sizeof(supported_device_t));

        supportedDevice->id = device;
        supportedDevice->node = dev->node;
        supportedDevice->mountPoint = NULL;
        supportedDevice->mount = NULL;
        supportedDevice->fs = FS_FAT32;

        supportedDevice->clusterSize = bpb->sectorsPerCluster;
        supportedDevice->sectorSize = sectorSize;
        if(bpb->totalSectors16 != 0){
            // The total number of sectors is in the 16-bit field
            supportedDevice->numSectors = bpb->totalSectors16;
        }else{
            // The total number of sectors is in the 32-bit field
            supportedDevice->numSectors = bpb->totalSectors32;
        }

        supportedDevice->numDataSectors = supportedDevice->numSectors - (bpb->reservedSectors + (bpb->numFATs * bpb->fat32.sectorsPerFAT32));

        supportedDevice->firstFatSector = bpb->reservedSectors;
        supportedDevice->fatSize = bpb->fat32.sectorsPerFAT32;
        supportedDevice->numRootDirSectors = 0;                                                                             // FAT32 does not have a root directory, so this is always 0
        supportedDevice->firstDataSector = supportedDevice->firstFatSector + (supportedDevice->fatSize * bpb->numFATs);     // The first data sector is the first FAT sector plus the number of FAT sectors times the number of FATs
        supportedDevice->clusterCount = supportedDevice->numDataSectors / supportedDevice->clusterSize;                     // The number of clusters is the number of sectors divided by the cluster size
        supportedDevice->next = NULL;

        // If the device is not a partition device, we need to create a partition device for it
        if(!(dev->type & DEVICE_TYPE_PARTITION)){
            device_t* partitionDevice = halloc(sizeof(device_t));
            if(partitionDevice == NULL){
                printk("Failed to allocate memory for the partition device!\n");
                hfree(supportedDevice);
                hfree(bpb);
                MUnlock(&dev->lock);
                this->driver.busy = false;
                return DRIVER_FAILURE;
            }
            memset(partitionDevice, 0, sizeof(device_t));
            partitionDevice->name = halloc(strlen(dev->node->name) + sizeof(partitionName) + 1);
            if(partitionDevice->name == NULL){
                printk("Failed to allocate memory for the partition device name!\n");
                hfree(supportedDevice);
                hfree(bpb);
                hfree(partitionDevice);
                MUnlock(&dev->lock);
                this->driver.busy = false;
                return DRIVER_FAILURE;
            }
            memset(partitionDevice->name, 0, strlen(dev->node->name) + sizeof(partitionName) + 1);
            strcpy(partitionDevice->name, dev->node->name);
            strcat(partitionDevice->name, partitionName);
            partitionDevice->class = DEVICE_CLASS_BLOCK;
            partitionDevice->type = (DEVICE_TYPE_STORAGE | DEVICE_TYPE_PARTITION | DEVICE_TYPE_FILESYSTEM);
            partitionDevice->parent = device;
            partitionDevice->driver = &this->driver;
            partitionDevice->driverData = supportedDevice;
            partitionDevice->ops.read = ReadFile;
            partitionDevice->ops.write = WriteFile;
            partitionDevice->ops.ioctl = HandleIoctl;
            partitionDevice->ops.poll = PollHandle;

            char* path = halloc(strlen(partitionDevice->name) + strlen("/dev/") + 1);
            if(path == NULL){
                printk("Failed to allocate memory for the partition device path!\n");
                hfree(supportedDevice);
                hfree(bpb);
                hfree(partitionDevice->name);
                hfree(partitionDevice);
                MUnlock(&dev->lock);
                this->driver.busy = false;
                return DRIVER_FAILURE;
            }
            memset(path, 0, strlen(partitionDevice->name) + strlen("/dev/") + 1);
            strcpy(path, "/dev/");
            strcat(path, partitionDevice->name);
            dresult_t result = RegisterDevice(partitionDevice, path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if(result == DRIVER_FAILURE){
                printk("Failed to register the partition device!\n");
                hfree(supportedDevice);
                hfree(bpb);
                hfree(partitionDevice->name);
                hfree(partitionDevice);
                hfree(path);
                MUnlock(&dev->lock);
                this->driver.busy = false;
                return DRIVER_FAILURE;
            }
            hfree(path);
            supportedDevice->id = partitionDevice->id;
            supportedDevice->node = partitionDevice->node;
        }else{
            // The device is a partition device, so we need to set the parent to the block device
            supportedDevice->id = dev->id;
            supportedDevice->node = dev->node;
        }

        // Add the supported device to the list of supported devices
        dresult_t res = AddSupportedDevice(supportedDevice);
        if(res == DRIVER_FAILURE){
            printk("Failed to add the supported device!\n");
            hfree(supportedDevice);
            hfree(bpb);
            MUnlock(&dev->lock);
            this->driver.busy = false;
            return DRIVER_FAILURE;
        }

        hfree(bpb);
        MUnlock(&dev->lock);
        this->driver.busy = false;
        return DRIVER_SUCCESS;
    }else{
        // TODO: Add support for FAT12, FAT16, and exFAT
        MUnlock(&dev->lock);
        hfree(bpb);
        this->driver.busy = false;
        printk("Unsupported filesystem type!\n");
        return DRIVER_FAILURE;
    }

    MUnlock(&dev->lock);
    hfree(bpb);
    this->driver.busy = false;
    return DRIVER_FAILURE;
}

dresult_t InitializeFAT(){
    fs_driver_t* driver = halloc(sizeof(fs_driver_t));
    if(driver == NULL){
        return DRIVER_FAILURE;
    }

    this = driver;

    driver->driver.name = "fat";
    driver->driver.class = DEVICE_CLASS_BLOCK;
    driver->driver.type = (DEVICE_TYPE_STORAGE | DEVICE_TYPE_PARTITION | DEVICE_TYPE_FILESYSTEM);
    driver->driver.init = InitializeFAT;
    driver->driver.deinit = NULL;
    driver->driver.probe = ProbeFs;
    driver->delete = Delete;
    driver->sync = Sync;
    driver->mount = Mount;
    driver->unmount = Unmount;

    RegisterDriver((driver_t*)driver, true);

    return DRIVER_SUCCESS;
}