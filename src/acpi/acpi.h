#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stdbool.h>
#include <util.h>

typedef struct RSDP_V1{
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
} PACKED RSDP_V1_t;

typedef struct RSDP_V2{
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdtAddress;             // Deprecated in V2
    uint32_t length;
    uint64_t xsdtAddress;             // This is the new one in V2 (If V2, then cast this to a uint32_t and use this table when on IA-32)
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} PACKED RSDP_V2_t;

typedef struct SDTHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} PACKED SDTHeader_t;

// Root System Description Table
typedef struct RSDT {
    struct SDTHeader header;
    uint32_t tablePointers[];
} PACKED RSDT_t;

// Extended Root System Description Table
typedef struct XSDT {
    SDTHeader_t header;
    uint64_t tablePointers[];
} PACKED XSDT_t;

typedef struct GenericAddressStructure {
    uint8_t addressSpace;
    uint8_t bitWidth;
    uint8_t bitOffset;
    uint8_t accessSize;
    uint64_t address;
} PACKED GenericAddressStructure;

// Why so many tables just to find this?
typedef struct PACKED FADT {
    SDTHeader_t header;
    uint32_t firmwareCtrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferredPMProfile;
    uint16_t sciInt;
    uint32_t smiCmd;
    uint8_t acpiEnable;
    uint8_t acpiDisable;
    uint8_t s4BIOSReq;
    uint8_t pStateCtrl;
    uint32_t pm1aEvtBlk;
    uint32_t pm1bEvtBlk;
    uint32_t pm1aCtrlBlk;
    uint32_t pm1bCtrlBlk;
    uint32_t pm2CtrlBlk;
    uint32_t pmTimerBlk;
    uint32_t gpe0Blk;
    uint32_t gpe1Blk;
    uint8_t pm1EvtLen;
    uint8_t pm1CtrlLen;
    uint8_t pm2CtrlLen;
    uint8_t pmTimerLen;
    uint8_t gpe0BlkLen;
    uint8_t gpe1BlkLen;
    uint8_t gpe1Base;
    uint8_t cStateCtrl;
    uint16_t worstC2Latency;
    uint16_t worstC3Latency;
    uint16_t flushSize;
    uint16_t flushStride;
    uint8_t dutyOffset;
    uint8_t dutyWidth;
    uint8_t dayAlarm;
    uint8_t monthAlarm;
    uint8_t century;

    // Reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t bootArchFlags;

    uint8_t reserved2;
    uint32_t flags;

    GenericAddressStructure resetReg;
    uint8_t resetValue;
    uint8_t reserved3[3];

    uint64_t xFirmwareCtrl;
    uint64_t xDsdt;

    GenericAddressStructure xPM1aEvtBlk;
    GenericAddressStructure xPM1bEvtBlk;
    GenericAddressStructure xPM1aCtrlBlk;
    GenericAddressStructure xPM1bCtrlBlk;
    GenericAddressStructure xPM2CtrlBlk;
    GenericAddressStructure xPMTimerBlk;
    GenericAddressStructure xGPE0Blk;
    GenericAddressStructure xGPE1Blk;
} PACKED FADT_t;

// This can be defined later
typedef struct PACKED MADT {
    SDTHeader_t header;
    uintptr_t localAPICAddress;
    uint32_t flags;
    uint8_t entries[];
} PACKED MADT_t;

typedef struct ACPIInfo {
    union {
        RSDP_V1_t* rsdpV1;
        RSDP_V2_t* rsdpV2;
    } rsdp;
    union {
        RSDT_t* rsdt;
        XSDT_t* xsdt;
    } rsdt;
    
    FADT_t* fadt;

    uint8_t version;              // Determined by FADT
    bool exists;
} ACPIInfo_t;

extern ACPIInfo_t acpiInfo;

void InitializeACPI();
void AcpiShutdown();
bool PS2ControllerExists();
void AcpiReboot();
ACPIInfo_t GetACPIInfo();

#endif // ACPI_H