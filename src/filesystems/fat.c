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

char* partName = "p1";

int InitializeFAT(){
    return DRIVER_SUCCESS;
}