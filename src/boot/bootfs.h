#ifndef BOOTFS_H
#define BOOTFS_H

#include <bootutil.h>

typedef struct {
    uint8_t jmpnop[3];
    char oemid[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t numFATs;
    uint16_t numRootEntries;
    uint16_t numSectorsSmall;
    uint8_t mediaType;
    uint16_t sectorsPerFAT;                 // FAT12/16 only (should apply to this bootloader)
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    uint32_t numHiddenSectors;
    uint32_t numSectorsLarge;
} bpb;

typedef struct {
    bpb paramBlock;
    union {
        struct {
            uint8_t driveNumber;
            uint8_t NTflags;
            uint8_t extBootSignature;
            uint32_t volumeID;
            char volumeLabel[11];
            char fsType[8];
        } fat12_16;
        struct {
            uint32_t sectorsPerFAT;
            uint16_t flags;
            uint16_t version;
            uint32_t rootCluster;
            uint16_t fsInfoSector;
            uint16_t backupBootSector;
            uint8_t reserved[12];
            uint8_t driveNumber;
            uint8_t reserved2;
            uint8_t extBootSignature;
            uint32_t volumeID;
            char volumeLabel[11];
            char fsType[8];
        } fat32;
    };
} ebr;

#endif