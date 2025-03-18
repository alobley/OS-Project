#include <fat.h>
#include <stdint.h>
#include <vfs.h>
#include <mbr.h>
#include <devices.h>
#include <util.h>

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
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t numRootDirEntries;
    uint16_t totalSectors16;                    // Is zero if totalSectors32 is used
    uint8_t mediaDescriptor;
    FAT1216 uint16_t sectorsPerFAT16;           // FAT12/16 only
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    lba hiddenSectors;                          // The number of hidden sectors (or, alternatively, the start of the partition)
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
            FAT32 BYTE _reserved[0];            // Wasted space ig
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
} PACKED bpb_t;

char* partName = "p1";

// File Reading...

// File Writing...

// TODO: actually detect and validate a FAT filesytstem
DRIVERSTATUS ProbeFATFilesystem(device_t* device){
    //printf("Probing...\n");
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
            *(uint64_t*)mbr = 0;
            *(((uint8_t*)mbr) + 8) = 1;
            device_read(device->id, (void*)mbr, sizeof(mbr_t)); // Read the first sector of the disk
            //do_syscall(SYS_DEVICE_READ, device->id, (uint32_t)mbr, sizeof(mbr_t), 0, 0);
            if(IsValidMBR(mbr)){
                // Just return a success for now since this isn't completely implemented

                // Check if the filesystem is FAT...

                char* name = (char*)halloc(7);
                memset(name, 0, 7);
                strcpy(name, device->devName);
                strcat(name, partName);                 // No need to manipulate partName since no MBR partitioning means only one partition

                partition_t* partition = (partition_t*)halloc(sizeof(partition_t));
                memset(partition, 0, sizeof(partition_t));
                device_t* fsDevice = CreateDevice("FAT Filesystem", name, "FAT Filesystem", partition, NULL/*The kernel will assign this*/, DEVICE_TYPE_FILESYSTEM, 0, (device_flags_t){0}, NULL, NULL, NULL, device);
                register_device(fsDevice);                  // Register the device with the kernel
                add_vfs_device(fsDevice, name, "/dev");     // Add the device to the VFS

                hfree(mbr);
                return DRIVER_INITIALIZED;
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
    do_syscall(SYS_MODULE_LOAD, (uint32_t)fatDriver, 0, 0, 0, 0);
    //RegisterDriver(fatDriver, NULL);
    return DRIVER_SUCCESS;
}