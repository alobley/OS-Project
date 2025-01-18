#ifndef ACPI_H
#define ACPI_H

#include <types.h>
#include <util.h>

typedef struct RSDP_V1{
    char signature[8];
    uint8 checksum;
    char OEMID[6];
    uint8 revision;
    uint32 rsdtAddress;
} PACKED RSDP_V1_t;

typedef struct RSDP_V2{
    char signature[8];
    uint8 checksum;
    char OEMID[6];
    uint8 revision;
    uint32 rsdtAddress;             // Deprecated in V2
    uint32 length;
    uint64 xsdtAddress;             // This is the new one in V2 (If V2, then cast this to a uint32 and use this table when on IA-32)
    uint8 extendedChecksum;
    uint8 reserved[3];
} PACKED RSDP_V2_t;

typedef struct SDTHeader {
    char signature[4];
    uint32 length;
    uint8 revision;
    uint8 checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32 OEMRevision;
    uint32 creatorID;
    uint32 creatorRevision;
} PACKED SDTHeader_t;

// Root System Description Table
typedef struct RSDT {
    struct SDTHeader header;
    uint32 tablePointers[];
} PACKED RSDT_t;

// Extended Root System Description Table
typedef struct XSDT {
    SDTHeader_t header;
    uint64 tablePointers[];
} PACKED XSDT_t;

typedef struct GenericAddressStructure {
    uint8 addressSpace;
    uint8 bitWidth;
    uint8 bitOffset;
    uint8 accessSize;
    uint64 address;
} PACKED GenericAddressStructure;

// Why so many tables just to find this?
typedef struct FADT {
    SDTHeader_t header;
    uint32 firmwareCtrl;
    uint32 dsdt;
    uint8 reserved;
    uint8 preferredPMProfile;
    uint16 sciInt;
    uint32 smiCmd;
    uint8 acpiEnable;
    uint8 acpiDisable;
    uint8 s4BIOSReq;
    uint8 pStateCtrl;
    uint32 pm1aEvtBlk;
    uint32 pm1bEvtBlk;
    uint32 pm1aCtrlBlk;
    uint32 pm1bCtrlBlk;
    uint32 pm2CtrlBlk;
    uint32 pmTimerBlk;
    uint32 gpe0Blk;
    uint32 gpe1Blk;
    uint8 pm1EvtLen;
    uint8 pm1CtrlLen;
    uint8 pm2CtrlLen;
    uint8 pmTimerLen;
    uint8 gpe0BlkLen;
    uint8 gpe1BlkLen;
    uint8 gpe1Base;
    uint8 cStateCtrl;
    uint16 worstC2Latency;
    uint16 worstC3Latency;
    uint16 flushSize;
    uint16 flushStride;
    uint8 dutyOffset;
    uint8 dutyWidth;
    uint8 dayAlarm;
    uint8 monthAlarm;
    uint8 century;

    // Reserved in ACPI 1.0; used since ACPI 2.0+
    uint16 bootArchFlags;

    uint8 reserved2;
    uint32 flags;

    GenericAddressStructure resetReg;
    uint8 resetValue;
    uint8 reserved3[3];

    uint64 xFirmwareCtrl;
    uint64 xDsdt;

    GenericAddressStructure xPM1aEvtBlk;
    GenericAddressStructure xPM1bEvtBlk;
    GenericAddressStructure xPM1aCtrlBlk;
    GenericAddressStructure xPM1bCtrlBlk;
    GenericAddressStructure xPM2CtrlBlk;
    GenericAddressStructure xPMTimerBlk;
    GenericAddressStructure xGPE0Blk;
    GenericAddressStructure xGPE1Blk;
} PACKED FADT_t;

typedef struct ACPIInfo {
    union {
        RSDP_V1_t* rsdpV1;
        RSDP_V2_t* rsdpV2;
    };
    union {
        RSDT_t* rsdt;
        XSDT_t* xsdt;
    };
    
    FADT_t* fadt;

    uint8 version;              // Determined by FADT
    bool exists;
} ACPIInfo_t;

extern ACPIInfo_t acpiInfo;

void InitializeACPI();
void AcpiShutdown();
bool PS2ControllerExists();
void AcpiReboot();
ACPIInfo_t GetACPIInfo();

#endif