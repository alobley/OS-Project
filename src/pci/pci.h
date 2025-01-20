#ifndef PCI_H
#define PCI_H

#include <types.h>
#include <util.h>
#include <io.h>


// 32-bit I/O locations for PCI configuration
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_FORWARDING_REGISTER 0xCFA

typedef struct PCI_Header {
    union {
        struct pciDevice {
            // Register 0x0, offset 0x0
            uint16 vendorID;
            uint16 deviceID;
            // Register 0x1, offset 0x4
            uint16 command;
            uint16 status;
            // Register 0x2, offset 0x8
            uint8 revisionID;
            uint8 progIF;
            uint8 subclass;
            uint8 classCode;
            // Register 0x3, offset 0xC
            uint8 cacheLineSize;
            uint8 latencyTimer;
            uint8 headerType;
            uint8 BIST;
            // Base addresses, registers 0x4-0x9, offsets 0x10-0x24
            uint32 BAR0;
            uint32 BAR1;
            uint32 BAR2;
            uint32 BAR3;
            uint32 BAR4;
            uint32 BAR5;
            // Register 0xA, offset 0x28
            uint32 cardbusCISPointer;
            // Register 0xB, offset 0x2C
            uint16 subsystemVendorID;
            uint16 subsystemID;
            // Register 0xC, offset 0x30
            uint32 expansionROMBaseAddress;
            // Register 0xD, offset 0x34
            uint8 capabilitiesPointer;
            uint8 reserved[3];
            // Register 0xE, offset 0x38
            uint32 reserved2;
            // Register 0xF, offset 0x3C
            uint8 interruptLine;
            uint8 interruptPin;
            uint8 minGrant;
            uint8 maxLatency;
        } PACKED;
        struct pciToPci {
            // Register 0x0, offset 0x0
            uint16 vendorID;
            uint16 deviceID;
            // Register 0x1, offset 0x4
            uint16 command;
            uint16 status;
            // Register 0x2, offset 0x8
            uint8 revisionID;
            uint8 progIF;
            uint8 subclass;
            uint8 classCode;
            // Register 0x3, offset 0xC
            uint8 cacheLineSize;
            uint8 latencyTimer;
            uint8 headerType;
            uint8 BIST;
            // Base addresses, registers 0x4-0x9, offsets 0x10-0x24
            uint32 BAR0;
            uint32 BAR1;
            uint32 BAR2;
            uint32 BAR3;
            uint32 BAR4;
            uint32 BAR5;
            // Register 0xA, offset 0x2C
            uint32 cardbusCISPointer;
            // Register 0xB, offset 0x2C
            uint16 subsystemVendorID;
            uint16 subsystemID;
            // Register 0xC, offset 0x30
            uint32 expansionROMBaseAddress;
            // Register 0xD, offset 0x34
            uint8 capabilitiesPointer;      // Points to a linked list of capabilities of the device (offset is based on offset from the start of the PCI configuration space)
            uint8 reserved[3];
            // Register 0xE, offset 0x38
            uint32 reserved2;
            // Register 0xF, offset 0x3C
            uint8 interruptLine;
            uint8 interruptPin;
            uint8 minGrant;                 // Read-only. This defines the minimum length of time, in 1/4 microsecond units, that the device requires from the bus once it gains control of the bus
            uint8 maxLatency;               // Read-only. This defines the maximum number of clock cycles the device can delay the bus before responding to a request (in 1/4 microsecond units)
        } PACKED;
        struct pciToCardbus {
            // Register 0x0, offset 0x0
            uint16 vendorID;
            uint16 deviceID;
            // Register 0x1, offset 0x4
            uint16 command;
            uint16 status;
            // Register 0x2, offset 0x8
            uint8 revisionID;
            uint8 progIF;
            uint8 subclass;
            uint8 classCode;
            // Register 0x3, offset 0xC
            uint8 cacheLineSize;
            uint8 latencyTimer;
            uint8 headerType;
            uint8 BIST;
            // Base addresses, registers 0x4-0x5, offsets 0x10-0x14
            uint32 BAR0;
            uint32 BAR1;
            // Register 0x6, offset 0x18
            uint8 primaryBusNumber;
            uint8 secondaryBusNumber;
            uint8 subordinateBusNumber;
            uint8 secondaryLatencyTimer;
            // Register 0x7, offset 0x1C
            uint8 ioBase;
            uint8 ioLimit;
            uint16 secondaryStatus;
            // Register 0x8, offset 0x20
            uint16 memoryBase;
            uint16 memoryLimit;
            // Register 0x9, offset 0x24
            uint16 prefetchableMemoryBase;
            uint16 prefetchableMemoryLimit;
            // Register 0xA, offset 0x28
            uint32 prefetchableBaseUpper32;
            // Register 0xB, offset 0x2C
            uint32 prefetchableLimitUpper32;
            // Register 0xC, offset 0x30
            uint16 ioBaseUpper16;
            uint16 ioLimitUpper16;
            // Register 0xD, offset 0x34
            uint8 capabilitiesPointer;
            uint8 reserved[3];
            // Register 0xE, offset 0x38
            uint32 expansionROMBaseAddress;
            // Register 0xF, offset 0x3C
            uint8 interruptLine;
            uint8 interruptPin;
            uint16 bridgeControl;
        } PACKED;
    } PACKED;
} PACKED PCI_header_t;

// PCI header types when reading the header type register
#define PCI_HEADER_TYPE_REG_MF (1 << 7)         // If 1 then the device is a multi-function device
#define PCI_HEADER_TYPE_STANDARD 0
#define PCI_HEADER_TYPE_PCI_TO_PCI 1
#define PCI_HEADER_TYPE_CARDBUS 2

#define PCI_MAX_BUS 256

// Must be 0-15 for the PIC (not APIC)
#define PCI_NOIRQ 0xFF


// PCI interrupt pins. 0 means no interrupt, 1-4 are the IRQs
#define PCI_INTERRUPT_PIN_NONE 0x0
#define PCI_INTERRUPT_PIN_A 0x1
#define PCI_INTERRUPT_PIN_B 0x2
#define PCI_INTERRUPT_PIN_C 0x3
#define PCI_INTERRUPT_PIN_D 0x4

// The bits of the PCI command register
#define PCI_CMD_IO (1 << 0)
#define PCI_CMD_MEM (1 << 1)
#define PCI_CMD_BUS_MASTER (1 << 2)
#define PCI_CMD_SPECIAL_CYCLES (1 << 3)
#define PCI_CMD_MEM_WRITE_INVALIDATE (1 << 4)
#define PCI_CMD_VGA_PALETTE_SNOOP (1 << 5)
#define PCI_CMD_PARITY_ERROR (1 << 6)
#define PCI_CMD_SERR_ENABLE (1 << 8)            // Set to 1 if the device supports SERR
#define PCI_CMD_FAST_BACK_TO_BACK (1 << 9)      // Read-Only
#define PCI_CMD_INTERRUPT_DISABLE (1 << 10)
// 11-15 are reserved

// The bits of the PCI status register
#define PCI_STATUS_INTERRUPT (1 << 3)
#define PCI_STATUS_CAPABILITIES (1 << 4)
#define PCI_STATUS_66MHZ (1 << 5)
#define PCI_STATUS_FAST_BACK_TO_BACK (1 << 7)
#define PCI_STATUS_DATA_PARITY_ERROR (1 << 8)
#define PCI_STATUS_DEVSEL_TIMING (1 << 9)
#define PCI_STATUS_SIGNAL_TARGET_ABORT (1 << 10)
#define PCI_STATUS_RECEIVED_TARGET_ABORT (1 << 11)
#define PCI_STATUS_RECEIVED_MASTER_ABORT (1 << 12)
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR (1 << 13)
#define PCI_STATUS_DETECTED_PARITY_ERROR (1 << 14)

// The bits of the PCI BIST register
#define BIST_CAPABLE (1 << 7)
#define BIST_START (1 << 6)
#define BIST_COMPLETION_CODE_MASK 0b111

// BAR layout
#define PCI_BAR_IOTYPE_MEM 0        // If the LSB in the BAR is 0, then it is a memory-mapped I/O address
#define PCI_BAR_IOTYPE_IO 1         // If the LSB in the BAR is 1, then it does not have to reside in physical RAM
#define PCI_BAR_IOTYPE_MASK 1

#define PCI_BAR_MEM_TYPE_MASK 0b110
#define PCI_BAR_MEM_PREFETCHABLE (1 << 3)
#define PCI_BAR_MEM_ADDR_MASK 0xFFFFFFF0                                    // The address mask for memory-mapped I/O (addresses based on 16-byte boundaries)
#define PCI_GET_MEM_ADDR(addr) (addr & PCI_BAR_MEM_ADDR_MASK)               // Convert a BAR address to a physical address

#define PCI_BAR_IO_ADDR_MASK 0xFFFFFFFC                                 // The address mask for I/O ports (addresses based on 4-byte boundaries)
#define PCI_GET_IO_ADDR(addr) (addr & PCI_BAR_IO_ADDR_MASK)             // Convert a BAR address to a physical address

#endif