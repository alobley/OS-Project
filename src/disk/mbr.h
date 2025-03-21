#ifndef MBR_H
#define MBR_H

// FOr partitioning information on an MBR disk

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <util.h>
#include <devices.h>

#define ID_EMPTY 0x00                       // No filesystem or unused partition
#define ID_FAT12 0x01                       // FAT12 partition
#define ID_XENIXROOT 0x02                   // Xenix root partition
#define ID_XENIXUSR 0x03                    // Xenix user partition
#define ID_SMALL_FAT16 0x04                 // FAT16 partition <= 32MB
#define ID_DOS_EXTENDED_PARTITION 0x05      // DOS 3.3+ extended partition - contains a linked list of logical partitions in a tiny partition elsewhere
#define ID_BIG_FAT16 0x06                   // FAT16 partition > 32MB
#define ID_NTFS 0x07                        // NTFS partition
#define ID_EXFAT 0x07                       // exFAT partition
#define ID_IFS 0x07                         // IFS (Installable File System) partition. Can be multiple different filesystems (for example EXT2)
#define ID_COMMODORE_DOS 0x08               // Commodore DOS logically sectored FAT partition
#define ID_DELL_PARTITION 0x08              // Dell partition (a partition that can span multiple drives)
#define ID_AIX 0x09                         // AIX data partition
#define ID_COHERENT 0x09                    // Coherent filesystem (Coherent was a type of UNIX)
#define ID_OS2_BOOT 0x0A                    // OS/2 Boot Manager partition
#define ID_COHERENT_SWAP 0x0A               // Swap partition for Coherent OS
#define ID_FAT32 0x0B                       // FAT32 partition
#define ID_FAT32_LBA 0x0C                   // FAT32 partition using LBA addressing

typedef struct PACKED {
    uint8_t bootIndicator;        // 0x80 for bootable, 0x00 for non-bootable
    uint8_t startHead;            // Starting head
    uint16_t startSector : 6;     // Starting sector (bits 0-5)
    uint16_t startCylinder : 10;  // Starting cylinder (bits 6-15)
    uint8_t systemID;             // System ID (0x07 for NTFS, 0x0B for FAT32, etc.)
    uint8_t endHead;              // Ending head
    uint16_t endSector : 6;       // Ending sector (bits 0-5)
    uint16_t endCylinder : 10;    // Ending cylinder (bits 6-15)
    uint32_t startLBA;            // Start of the partition
    uint32_t sizeLBA;             // Size of the partition
} PACKED mbr_partition_entry_t;

typedef struct PACKED {
    uint8_t bootloader[446];             // Bootloader code
    mbr_partition_entry_t partitions[4]; // Four partition entries - requires an extended partition for more
    uint16_t signature;                  // Signature (should be 0xAA55)
} PACKED mbr_t;

static inline bool IsValidMBR(mbr_t* mbr){
    return mbr->signature == 0xAA55;
}

// Looks for MBR partitions (if none, filesystem drivers will have to make their own partition)
DRIVERSTATUS GetPartitionsFromMBR(device_t* disk);

// Convert an MBR entry to a kernel partition struct
partition_t* MakePartition(mbr_partition_entry_t* entry, device_t* parent);

#endif