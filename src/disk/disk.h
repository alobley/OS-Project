#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>

// Common interface for disk drivers

typedef enum Disk_Interface {
    DISK_INTERFACE_PATA,
    DISK_INTERFACE_AHCI,
    DISK_INTERFACE_NVME
    // More to be added
} disk_interface_t;

typedef enum Disk_Type {
    DISK_TYPE_NVRAM,                // Non-volatile RAM (e.g. flash memory)
    DISK_TYPE_ROM,                  // Read-only memory (e.g. CD-ROM)
    DISK_TYPE_RW,                   // Read-write memory (e.g. hard drive)
    // More...
} disktype_t;

// Filesystems supported by the kernel
typedef enum Filesystem {
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
    FS_ExFAT,
    FS_EXT2
} mediafs_t;

typedef enum Disk_Status {
    DISK_STATUS_OK,
    DISK_STATUS_ERROR,
    DISK_STATUS_NOT_PRESENT,
    DISK_STATUS_NOT_SUPPORTED,
    DISK_STATUS_NOT_READY,
    DISK_STATUS_NO_MEDIA,
    DISK_STATUS_MEDIA_CHANGED,
    DISK_STATUS_BUSY,
    DISK_STATUS_TIMEOUT,
    DISK_STATUS_UNKNOWN
} disk_status_t;

typedef struct Features {
    bool removable : 1;             // Removable media
    bool writable : 1;              // Media is writable
    bool GPT : 1;                   // GUID Partition Table (as opposed to MBR) is present
    bool LBA : 1;                   // LBA is supported (if true, at least LBA28 is supported)
    bool LBA48 : 1;                 // LBA48 is supported
    bool DMA : 1;                   // DMA is supported
    bool present : 1;               // Disk is present
} diskflags_t;

typedef struct Media_Descriptor {
    // General information
    uint8_t version;                // Version of the media descriptor
    uint64_t size;                  // Size of the media in bytes
    uint32_t sectorSize;            // Size of a sector in bytes
    uint64_t sectorCount;           // Number of sectors on the media
    disk_interface_t interface;     // Interface used to access the media
    disktype_t type;                // Type of media
    mediafs_t filesystem;           // Filesystem used on the media
    diskflags_t flags;              // Flags defining the disk's capabilities
    disk_status_t status;           // Current status of the disk
    uint16_t diskNum;               // Disk number (for use by the kernel to identify disks)

    // OEM information
    uint16_t vendorID;              // Vendor ID (if applicable)
    uint16_t deviceID;              // Device ID (if applicable)
    char model[64];                 // Model name (if applicable)
    char serial[32];                // Serial number (if applicable)
    char firmware[16];              // Firmware version (if applicable)

    // Physical information (if CHS, otherwise 0)
    uint16_t cylinders;             // Number of cylinders
    uint16_t heads;                 // Number of heads
    uint16_t sectorsPerTrack;       // Number of sectors per track

    // Miscellaneous information, if applicable based on interface
    uint16_t basePort;              // Base port number
    uint16_t controlPort;           // Control port number
    bool slave;                     // "Slave" device (if applicable)
} media_descriptor_t;

#endif