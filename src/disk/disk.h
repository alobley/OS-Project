#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>
#include <multitasking.h>
#include <devices.h>
#include <vfs.h>

#define WRITE_SUCCESS 0
#define WRITE_FAILURE -1

// Common interface for disk drivers

typedef enum Disk_Interface {
    DISK_INTERFACE_PATA,
    DISK_INTERFACE_AHCI,
    DISK_INTERFACE_NVME,
    DISK_INTERFACE_RAMDISK,
    // More to be added
} disk_interface_t;

typedef enum Disk_Type {
    DISK_TYPE_NVRAM,                // Non-volatile RAM (e.g. flash memory)
    DISK_TYPE_ROM,                  // Read-only memory (e.g. CD-ROM)
    DISK_TYPE_RW,                   // Read-write memory (e.g. hard drive)
    DISK_TYPE_RAMDISK,              // RAM disk (e.g. tmpfs)
    // More...
} disktype_t;

// Filesystems supported by the kernel
typedef enum Filesystem {
    FS_FAT12,            // FAT12 filesystem
    FS_FAT16,            // FAT16 filesystem
    FS_FAT32,            // FAT32 filesystem
    FS_ExFAT,            // ExFAT filesystem
    FS_EXT2,             // EXT2 filesystem
    FS_NONE,             // No filesystem (raw disk)
    FS_OTHER,            // Other filesystem (not supported by the kernel)
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
    device_t* device;               // Pointer to the device structure (which also points to this)
    uint8_t version;                // Version of the media descriptor
    uint64_t size;                  // Size of the media in bytes
    uint32_t sectorSize;            // Size of a sector in bytes
    uint64_t sectorCount;           // Number of sectors on the media
    disk_interface_t interface;     // Interface used to access the media
    disktype_t type;                // Type of media
    diskflags_t flags;              // Flags defining the disk's capabilities
    disk_status_t status;           // Current status of the disk
    uint16_t diskNum;               // Disk number (for use by the kernel to identify disks)

    // OEM information
    uint16_t vendorID;              // Vendor ID (if applicable)
    uint16_t deviceID;              // Device ID (if applicable)
    char model[64];                 // Model name (if applicable)
    char serial[32];                // Serial number (if applicable)
    char firmware[16];              // Firmware version (if applicable)

    // Filesystem
    driver_t* fsDriver;             // Driver for the filesystem
    mediafs_t filesystem;           // Filesystem used on the media
    char* fsName;                   // Name of the filesystem (if none, "RAW")

    // NOTE: make sure to update the VFS, and when the VFS adds a directory to this disk, update the disk!
    vfs_node_t* (*ReadFile)(struct Media_Descriptor* self, char* path);
    int (*WriteFile)(struct Media_Descriptor* self, char* path, vfs_node_t* file);
    int (*DeleteFile)(struct Media_Descriptor* self, char* path);
    int (*CreateFile)(struct Media_Descriptor* self, char* path, size_t size);
    int (*CreateDirectory)(struct Media_Descriptor* self, char* path);
    int (*DeleteDirectory)(struct Media_Descriptor* self, char* path);
    int (*Rename)(struct Media_Descriptor* self, char* oldPath, char* newPath);
    int (*Move)(struct Media_Descriptor* self, char* oldPath, char* newPath);

    // Physical geometry (if CHS, otherwise 0)
    uint16_t cylinders;             // Number of cylinders
    uint16_t heads;                 // Number of heads
    uint16_t sectorsPerTrack;       // Number of sectors per track

    // Miscellaneous information, if applicable based on interface
    uint16_t basePort;              // Base port address
    uint16_t controlPort;           // Control port address
    bool slave;                     // "Slave" device (if applicable)
} media_descriptor_t;

#endif