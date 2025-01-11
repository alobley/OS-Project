#ifndef FAT_H
#define FAT_H

#include <types.h>
#include <util.h>
#include "ata.h"
#include "vfs.h"

#define VALID_FSINFO_LEAD 0x41615252
#define VALID_FSINFO_MID 0x61417272
#define VALID_FSINFO_TRAIL 0xAA550000

// Note that the union combines the two into the same memory space - changing one affects the other.
typedef struct PACKED Extended_Boot_Record {
    union {
        // Pack your inner structs too! It aligned these!
        struct PACKED {
            uint8 driveNum;             // Don't use this under any circumstances
            uint8 _reserved;            // This isn't Windows
            uint8 signature;            // Must be 0x28 or 0x29
            uint32 serialNum;           // Volume serial number
            char volumeLabel[11];       // The name of this disk
            char fsType[8];             // String representation of the type of FAT filesystem. Apparently I shouldn't ever trust this.
            uint8 bootCode[448];        // The bootloader on this disk, if there is one
            uint16 bootSig;             // The boot signature. Is 0xAA55.
        } fat1216;
        struct PACKED {
            uint32 sectorsPerFat;
            uint16 flags;
            uint16 version;             // High byte = major version, low byte = minor version
            uint32 rootDirCluster;      // Cluster containing the root directory
            uint16 fsInfo;              // Sector number on the disk containing the fsinfo structure
            uint16 backupBoot;          // Sector number for the backup boot sector
            uint8 _reserved[12];
            uint8 driveNum;             // Same deal as the other one. Don't use.
            uint8 _reserved2;           // This isn't Windows
            uint8 signature;            // Must be 0x28 or 0x29
            uint32 serialNum;
            char volumeLabel[11];
            char systemID[8];           // System identifier, always "FAT32   ".
            uint8 bootCode[420];
            uint16 bootSig;
        } fat32;
    } ebr_type;
} ebr_t;

typedef struct PACKED BIOS_Parameter_Block {
    uint8 jmpnop[3];            // The JMP to the bootloader code
    char oemName[8];            // String containing the name of the "DOS" version
    uint16 bytesPerSector;
    uint8 sectorsPerCluster;
    uint16 reservedSectors;
    uint8 numFATs;
    uint16 rootDirEntries;
    uint16 totalSectors;        // The total sectors in this volume. If this is zero, the actual count is stored in the large sector count
    uint8 mediaDescriptor;
    uint16 sectorsPerFAT;       // FAT12/16 only
    uint16 sectorsPerTrack;
    uint16 numHeads;
    uint32 hiddenSectors;       // LBA of the beginning of the partition
    uint32 largeSectorCount;
    ebr_t ebr;                  // Extended Boot Record
} bpb_t;

typedef struct PACKED Filesystem_Info {
    uint32 leadSig;                 // Lead signature. Must be 0x41615252 to be a valid fsinfo structure
    uint8 _reserved[480];           // Seems like a waste of space to me, idk
    uint32 midSig;                  // Second signature. Must be 0x61417272 to be a valid fsinfo structure
    uint32 freeClusters;            // Indicates when the driver should start looking for available clusters. If 0xFFFFFFFF, then this must be computed.
    uint32 firstFreeCluster;        // Indicates the cluster number where the driver should start looking for free clusters. If 0xFFFFFFFF, start looking at 2.
    uint8 _reserved2;               // Reserved
    uint32 trailSig;                // The trail signature. Must be 0xAA550000 to be a valid fsinfo structure.
} fsinfo_t;

// exFAT completely redesigned the boot record (currently unsupported, implement later)
typedef struct PACKED exFAT_Boot_Record {
    uint8 jmpnop[3];
    char oemId[8];                  // Usually contains "EXFAT" but should not be trusted
    uint8 zero[53];                 // To prevent FAT drivers from loading an exFAT filesystem
    uint64 partitionOffset;         // LBA offset for the start of this partition.
    uint64 volumeLength;
    uint32 fatOffset;               // FAT offset in sectors from the start of the partition
    uint32 fatLength;               // FAT length in sectors
    uint32 clusterHeap;             // Cluster heap offset in sectors
    uint32 clusterCount;            // The total amount of clusters
    uint32 rootCluster;             // Root directory cluster
    uint32 serialNum;
    uint16 revision;                // Filesystem revision
    uint16 flags;
    uint8 sectorShift;
    uint8 clusterShift;
    uint8 numFATs;
    uint8 driveSelect;
    uint8 percentageUsed;
    uint8 reserved[7];
} exfat_br_t;

typedef struct FAT_Disk {
    disk_t* parent;         // The ATA disk with a FAT filesystem
    uint8 fstype;           // The filesystem type
    uint64 firstFatSector;  // The first FAT sector
    uint64 fatSize;         // The FAT size in sectors
    uint64 volumeSectors;   // Total sectors in this volume
    uint64 rootSectors;     // Size of the root directory in sectors
    uint64 firstDataSector; // First sector where data (i.e. files) can be stored
    uint64 dataSectors;     // The total number of data sectors on the disk
    uint64 totalClusters;   // The total amount of clusters in the filesystem
    bpb_t* paramBlock;      // In case anything was forgotten
} fat_disk_t;


// These are for reading a file's attributes
#define ATTR_READONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LFN (ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)       // Long file entry
#define DELETED 0xE5


// &name[1] - sizeof(lfn_entry_t) = LFN entry
// Files and directories count as the same thing, attributes tell them apart
typedef struct PACKED FAT_File {
    //lfn_entry_t* lfnEntry;
    char name[11];
    uint8 attributes;               // Attributes of the file
    uint8 _reserved;
    uint8 creationSec;              // Creation time in hundredths of a second
    uint16 creationTime;
    uint16 creationDate;
    uint16 lastAccessedDate;
    uint16 firstClusterHigh;        // The high 16 bits of the entry's first cluster number
    uint16 lastModifiedTime;
    uint16 lastModifiedDate;
    uint16 firstClusterLow;         // The low 16 bits of the entry's first cluster number
    uint32 fileSize;                // Size of the file in bytes
} fat_entry_t;

typedef struct PACKED LFN_Entry {
    uint8 orderSequence;            // The order this is in an LFN. There are multiple values in chains.
    uint16 firstFive[5];             // The first five two-byte characters in this entry
    uint8 attribute;                // Always 0x0F
    uint8 type;                     // Long entry type. Zero if name.
    uint8 checksum;
    uint16 midSix[6];                // The next 6 two-byte characters in this entry
    uint16 alwaysZero;
    uint16 finalTwo[2];              // The last two characters of this entry
    fat_entry_t file;
} lfn_entry_t;

// exFAT isn't implemented entirely, but the datastructure exists for now. Unsure if full implementation will be done.
typedef struct PACKED exFAT_file {
    uint8 type;
    uint8 numSecondaries;           // The number of secondary entries
    uint16 checksum;
    uint16 attributes;
    uint16 _reserved;
    uint32 creationDate;
    uint32 modificationDate;
    uint32 accessDate;
    uint8 creationTime;
    uint8 modificationTime;
    uint8 utcCreationTime;
    uint8 utcModTime;
    uint8 utcAccessTime;
    uint8 _reserved1[7];
    // Stream "extension"
    uint8 entryType;
    uint8 secondaryFlags;
    uint8 _reserved2;
    uint8 nameLength;
    uint16 nameHash;
    uint16 _reserved3;
    uint64 validDataLength;
    uint32 _reserved4;
    uint32 firstCluster;
    uint64 dataLength;
    // File name entry
    uint8 nameEntryType;
    uint8 nameFlags;
    char fileName[30];
} exfat_entry_t;

// Linked list of clusters
typedef struct PACKED FAT_cluster {
    uint32 cluster;
    void* buffer;
    struct FAT_Dir* next;
} FAT_cluster_t;

void ClearFatDirectory(FAT_cluster_t* rootDir);
fat_disk_t* TryFatFS(disk_t* disk);
file_t* FatSeekFile(fat_disk_t* fatdisk, char* fileName);        // Returns a pointer to the loaded file
FAT_cluster_t* FatReadRootDirectory(fat_disk_t* fatdisk);        // Returns a linked list of clusters containing the root directory entries
directory_t* FATDirToVfsDir(FAT_cluster_t* directory, fat_disk_t* fatdisk, char* name);

#endif