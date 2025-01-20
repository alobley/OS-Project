#include "pci.h"

// Read a 16-bit value from the PCI configuration space
uint16 PciReadConfig16(uint8 bus, uint8 slot, uint8 function, uint8 offset){
    uint32 addr;
    uint32 lbus = (uint32)bus;
    uint32 lslot = (uint32)slot;
    uint32 lfunc = (uint32)function;
    uint16 tmp = 0;

    // Create configuration address
    addr = (uint32)((uint32)lbus << 16 | (uint32)lslot << 11 | (uint32)lfunc << 8 | (uint32)(offset & 0xFC) | ((uint32)0x80000000));

    // Write the address to the address port
    outl(PCI_CONFIG_ADDRESS, addr);

    // Read the data from the data port
    tmp = (uint16)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}


// Get a PCI device's vendor ID (incomplete)
uint16 PciGetVendor(uint8 bus, uint8 slot){
    uint16 vendor;
    uint16 device;
    vendor = PciReadConfig16(bus, slot, 0, 0);
    if(vendor == 0xFFFF){
        // Device does not exist!
        return 0xFFFF;
    }else{
        //device = PciReadConfig16(bus, slot, 0, 2);
        return vendor;
    }
}