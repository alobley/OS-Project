#include <ata.h>
#include <kernel.h>
#include <interrupts.h>
#include <console.h>
#include <alloc.h>
#include <devices.h>
//#include <gpt.h>      // Doesn't exist yet

#define NUM_BUS_PORTS 7

#define PATADISK 0      // Hard drive, readable and writable
#define PATAPIDISK 1    // ROM drive
#define SATADISK 2      // Disk is attached via SATA.

// Drives 0 and 1
#define PRIMARY_BUS_BASE 0x1F0
#define PRIMARY_BUS_CTRL 0x3F6

// Drives 2 and 3
#define SECONDARY_BUS_BASE 0x170
#define SECONDARY_BUS_CTRL 0x376

// Drives 4 and 5
#define TERTIARY_BUS_BASE 0x1E8
#define TERTIARY_BUS_CTRL 0x3E6

// Drives 6 and 7
#define QUATERNARY_BUS_BASE 0x168
#define QUATERNARY_BUS_CTRL 0x366

// ATA commands
#define COMMAND_FLUSH_CACHE 0xE7
#define COMMAND_IDENTIFY 0xEC
#define COMMAND_IDENTIFY_PACKET 0xA1
#define COMMAND_PACKET 0xA0
#define COMMAND_READ_MULTIPLE 0xC4
#define COMMAND_WRITE_MULTIPLE 0xC5
#define COMMAND_SET_MULTIPLE 0xC6
#define COMMAND_READ_SECTORS 0x20
#define COMMAND_READ_SECTORS_EXT 0x24
#define COMMAND_WRITE_SECTORS 0x30
#define COMMAND_WRITE_SECTORS_EXT 0x34

#define MASTER_DRIVE 0xA0
#define SLAVE_DRIVE 0xB0

#define FLOATING_BUS 0xFF                       // If the regular status byte contains this, there are no drives on this bus

// These macros get ATA ports based on a given base
#define DataPort(basePort) basePort             // The data port is the base port, but this makes things more readable.
#define FeaturesPort(basePort) (basePort + 1)   // Features register (write)
#define ErrorPort(basePort) (basePort + 1)      // Error register (read)
#define SectorCount(basePort) (basePort + 2)    // The amount of sectors to read/write (read/write)
#define LbaLo(basePort) (basePort + 3)          // First (little-endian) part of the LBA address (read/write)
#define LbaMid(basePort) (basePort + 4)         // Second part of the LBA address (read/write)
#define LbaHi(basePort) (basePort + 5)          // Third part of the LBA address (read/write)
#define DriveSelect(basePort) (basePort + 6)    // Drive/Head select port
#define CmdPort(basePort) (basePort + 7)        // Write-Only
#define StatusPort(basePort) (basePort + 7)     // Read-Only

#define AltStatus(ctrlPort) (ctrlPort)          // Alternate status register. Reading from the control port will get this.
#define DriveAddress(ctrlPort) (ctrlPort + 1)   // The drive address reguster, readonly

// Flags from the error register (read)
#define ERR_AMNF (1 << 0)                       // Address mark not found
#define ERR_TZNF (1 << 1)                       // Track zero not found
#define ERR_ABRT (1 << 2)                       // Aborted command
#define ERR_MCR (1 << 3)                        // Media change request
#define ERR_IDNF (1 << 4)                       // ID not found
#define ERR_MC (1 << 5)                         // Media Changed
#define ERR_UNC (1 << 6)                        // Uncorrectable data error
#define ERR_BBLK (1 << 7)                       // Bad block

// Flags from the status register (read)
#define FLG_ERR (1 << 0)                        // Error
#define FLG_IDX (1 << 1)                        // Index (always 0)
#define FLG_CORR (1 << 2)                       // Corrected data (always 0)
#define FLG_DRQ (1 << 3)                        // Data request. When the drive is ready for PIO data transfer in or out
#define FLG_SRV (1 << 4)                        // Overlapped mode service request
#define FLG_DF (1 << 5)                         // Drive fault error (does not set the ERR flag)
#define FLG_RDY (1 << 6)                        // Device is ready
#define FLG_BSY (1 << 7)                        // Device is busy. If it doesn't clear, a software reset is a good idea


// Commands to send to the device control register (write)
#define CMD_NOINT (1 << 1)                      // Disable interrupt generation
#define CMD_SRST (1 << 2)                       // Software reset (set, wait 5 nanoseconds, clear)
#define CMD_HOB (1 << 7)                        // Set to read back the high order byte of the last lba48 value sent to an IO port (so basically useless)


// Flags from the drive address register (read)
#define FLG_DS0 (1 << 0)                        // 0 when drive 0 is selected, 1 when drive 1 is selected
#define FLG_DS1 (1 << 1)                        // 0 when drive 1 is selected, 1 when drive 0 is selected
#define FLG_HS (0b111 << 5)                     // NOT(currently selected head) <--- why one's complement?
#define FLG_WTG (1 << 6)                        // Write gate, 0 when writing to the drive
#define FLOPPY_RESERVED (1 << 7)                // Reserved for floppy disks

// Addressing support types
#define CHS_ONLY 0
#define LBA28 1
#define LBA48 2

// For easier access
typedef struct ataDisk {
    uint8_t driveNum;       // Drive number (determined by me)
    uint16_t base;          // Base port
    uint16_t ctrl;          // Control register
    uint64_t size;          // Total size of the disk in sectors
    uint16_t sectorSize;    // The size of one sector
    uint8_t type;           // PATA, PATAPI, SATA, SATAPI
    uint8_t addressing;     // CHS, LBA28, or LBA48
    bool slave : 1;         // Whether the device is a slave or not
    bool packet : 1;        // Whether the disk is a packet disk or not
    bool removable : 1;     // Whether or not the device is a removable disk
    bool populated : 1;     // The device is populated with a disk (removable media only)
    uint16_t* infoBuffer;   // The 256 16-bit values read when the disk was identified
    uint16_t numCylinders;      // Number of cylinders
    uint16_t numHeads;          // Number of heads
    uint16_t numSectors;        // Number of sectors
} disk_t;

// Get the control register of a given base
inline uint16_t GetCtrl(uint16_t basePort){
    switch (basePort){
        case PRIMARY_BUS_BASE:
            return PRIMARY_BUS_CTRL;
        case SECONDARY_BUS_BASE:
            return SECONDARY_BUS_CTRL;
        case TERTIARY_BUS_BASE:
            return TERTIARY_BUS_CTRL;
        case QUATERNARY_BUS_BASE:
            return QUATERNARY_BUS_CTRL;
        default:
            // Invalid base
            return 0;
    }
}

void DiskDelay(uint16_t basePort){
    for(int i = 0; i < 15; i++){
        inb(GetCtrl(basePort));
    }
}

bool IsBusy(uint16_t basePort){
    uint32_t status = inb(StatusPort(basePort));
    return (status & FLG_BSY) != 0;
}

void FlushCache(uint16_t basePort){
    outb(CmdPort(basePort), COMMAND_FLUSH_CACHE);
    DiskDelay(basePort);
}

void SelectDisk(disk_t* disk){
    if(disk->slave){
        outb(DriveSelect(disk->base), SLAVE_DRIVE);
    }else{
        outb(DriveSelect(disk->base), MASTER_DRIVE);
    }
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void WaitForIdle(uint16_t basePort){
    while(inb(StatusPort(basePort)) & FLG_BSY);
}

void WaitForDrq(uint16_t basePort){
    while(!(inb((StatusPort(basePort))) & FLG_DRQ));
}
#pragma GCC pop_options

void SoftwareReset(disk_t* disk){
    // Software reset
    outb(CmdPort(disk->base), CMD_SRST);
    DiskDelay(disk->base);
}

void DetermineAddressing(disk_t* disk){
    // Default to CHS
    disk->addressing = CHS_ONLY;
    disk->size = 0;

    ataInfo_t* info = (ataInfo_t*)disk->infoBuffer;
    if(info->additionalSupported.deviceEncryptsAllData){
        // This is a security device, so we don't support it
        disk->size = 0;
        return;
    }

    disk->sectorSize = info->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector == 0 ? 512 : (1 << info->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector);

    if(disk->packet == false){
        // Disk is a hard drive or other NVRAM
        if(disk->infoBuffer[83] & (1 << 10)){
            // LBA48 supported
            disk->addressing = LBA48;
        }

        if(disk->addressing == CHS_ONLY){
            // If not 48-bit LBA, check for 28-bit LBA
            uint32_t lba28Sectors = info->addressableSectors;
            if(lba28Sectors > 0){
                disk->addressing = LBA28;
                disk->size = lba28Sectors;
            }else{
                // The disk is truly CHS only
                // Get the size of the disk from the number of cylinders, the number of heads, and the number of sectors per track, respectively.
                uint64_t chsSectors = (uint64_t)disk->numCylinders * (uint64_t)disk->numHeads * (uint64_t)disk->numSectors;
                disk->size = chsSectors;
            }
        }else{
            // If 48-bit LBA, get the number of 48-bit LBA sectors
            uint64_t lba48Sectors = ((uint64_t)info->Max48BitLBA[1] << 32) | info->Max48BitLBA[0];
            if(lba48Sectors > 0){
                disk->size = lba48Sectors;
            }else{
                // No 48-bit LBA
                uint32_t lba28Sectors = info->addressableSectors;
                disk->size = lba28Sectors;
                disk->addressing = LBA28;
            }
        }
    }else if(disk->packet){
        if(disk->infoBuffer[0] & (1 << 7)){
            // Removable device
            disk->removable = true;
        }else{
            disk->removable = false;
        }

        if(disk->infoBuffer[49] & (1 << 9)){
            // LBA supported, no 48-bit LBA on PATAPI
            disk->addressing = LBA28;

            // For PIO mode (which we start in by default)
            outb(FeaturesPort(disk->base), 0);

            // I think this is what the guide meant? Pretty sure PATA only has 8-bit registers
            // May be LbaLo and LbaMid instead
            outb(LbaLo(disk->base), 0x00);
            outb(LbaMid(disk->base), 0x08);
            //outw(LbaHi(disk->base), 0x0008);

            outb(CmdPort(disk->base), COMMAND_PACKET);
            WaitForIdle(disk->base);
            WaitForDrq(disk->base);

            uint8_t packet[12] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            // This will get the size of the disk in sectors, I guess?
            for(int i = 0; i < 6; i++){
                outw(DataPort(disk->base), ((uint16_t*)packet)[i]);
            }

            outb(CmdPort(disk->base), COMMAND_PACKET);
            WaitForIdle(disk->base);
            WaitForDrq(disk->base);

            // Turns out it was just two words. Interesting. (everything I read said otherwise, that it was four words.)
            uint8_t data[4];
            for(int i = 0; i < 4; i++){
                data[i] = inb(DataPort(disk->base));
            }

            uint32_t maxLba = (data[1] << 8) | data[0];
            uint32_t sectorSize = (data[3] << 8) | data[2];

            if(maxLba == 0 || inb(StatusPort(disk->base)) & 0x01){
                // No disk or error
                disk->populated = false;
                disk->size = maxLba;
                disk->sectorSize = 0;
            }else{
                // There is an inserted disk
                disk->populated = true;
                disk->size = maxLba + 1;
                disk->sectorSize = sectorSize;
            }
        }else{
            // CHS only - should be impossible, likely an empty tray
            disk->size = 0;
            disk->populated = false;
        }
    }

    disk->numCylinders = info->numCurrentCylinders;
    disk->numHeads = info->numCurrentHeads;
    disk->numSectors = info->numCurrentSectorsPerTrack;

    printk("Number of cylinders: %u\n",  info->numCurrentCylinders);
    printk("Number of heads: %u\n", info->numCurrentHeads);
    printk("Number of sectors: %u\n", info->numCurrentSectorsPerTrack);
    printk("Disk sector size: %u\n", disk->sectorSize);
    //STOP
}

disk_t* IdentifyDisk(uint8_t diskNum){
    disk_t* disk = (disk_t*)halloc(sizeof(disk_t));
    switch (diskNum){
        // Enter disk information based on the disk number provided
        case 0:
            disk->driveNum = diskNum;
            disk->slave = false;
            disk->base = PRIMARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 1:
            disk->driveNum = diskNum;
            disk->slave = true;
            disk->base = PRIMARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 2:
            disk->driveNum = diskNum;
            disk->slave = false;
            disk->base = SECONDARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 3:
            disk->driveNum = diskNum;
            disk->slave = true;
            disk->base = SECONDARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 4:
            disk->driveNum = diskNum;
            disk->slave = false;
            disk->base = TERTIARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 5:
            disk->driveNum = diskNum;
            disk->slave = true;
            disk->base = TERTIARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 6:
            disk->driveNum = diskNum;
            disk->slave = false;
            disk->base = QUATERNARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        case 7:
            disk->driveNum = diskNum;
            disk->slave = true;
            disk->base = QUATERNARY_BUS_BASE;
            disk->ctrl = GetCtrl(disk->base);
            break;
        default:
            // Invalid disk number
            hfree(disk);
            return NULL;
    }

    if(inb(StatusPort(disk->base)) == FLOATING_BUS){
        // Floating bus, no disks in this bus
        hfree(disk);
        return NULL;
    }

    SelectDisk(disk);

    // Set the control register to 0
    outb(disk->ctrl, 0);

    outb(SectorCount(disk->base), 0);
    outb(LbaLo(disk->base), 0);
    outb(LbaMid(disk->base), 0);
    outb(LbaHi(disk->base), 0);

    outb(CmdPort(disk->base), COMMAND_IDENTIFY);

    uint8_t driveStatus = inb(StatusPort(disk->base));
    if(driveStatus == 0){
        // Drive does not exist
        hfree(disk);
        return NULL;
    }else{
        WaitForIdle(disk->base);
        if(inb(LbaMid(disk->base)) == 0x3C && inb(LbaHi(disk->base)) == 0xC3){
            // Drive is SATA, which is unsupported but the infrastructure should be here
            disk->type = SATADISK;
        }else if(inb(SectorCount(disk->base)) == 0x01 && inb(LbaLo(disk->base)) == 0x01 && inb(LbaMid(disk->base)) == 0x14 && inb(LbaHi(disk->base)) == 0xEB){
            disk->type = PATAPIDISK;
            disk->packet = true;
            outb(CmdPort(disk->base), COMMAND_IDENTIFY_PACKET);
            WaitForIdle(disk->base);
        }else{
            if(driveStatus & 0x01){
                // There was an error, the drive can't be used
                hfree(disk);
                return NULL;
            }else{
                disk->packet = false;
                // Regular PATA disk. Used normally.
                if(inb(LbaMid(disk->base)) == 0x14 && inb(LbaHi(disk->base)) == 0xEB){
                    // Drive is non-removable PATAPI
                    disk->removable = false;
                    disk->type = PATAPIDISK;
                }else{
                    // We can finally confirm the existence of a PATA drive
                    disk->removable = false;
                    disk->populated = true;
                    disk->type = PATADISK;
                }
            }
        }
    }

    uint8_t status = inb(StatusPort(disk->base));
    while(!(status & 0x08)){
        // Wait until DRQ is ready
        if(status & FLG_ERR){
            // There was an error, the disk cannot be used
            hfree(disk);
            return NULL;
        }
        status = inb(StatusPort(disk->base));
    }

    uint16_t* diskBuffer = (uint16_t*)halloc(512);
    memset(diskBuffer, 0, 512);
    disk->infoBuffer = diskBuffer;
    
    for(int i = 0; i < 256; i++){
        // Read the buffer into memory
        disk->infoBuffer[i] = inw(DataPort(disk->base));
    }

    // All disks should be the same
    DetermineAddressing(disk);

    return disk;
}

disk_t* FindDisk(uint8_t diskno, device_t* ataDevice){
    // Search for PATA devices
    uint8_t status = inb(StatusPort(PRIMARY_BUS_BASE));
    if(status != FLOATING_BUS){
        disk_t* disk = IdentifyDisk(diskno);
        if(disk == NULL){
            return NULL;
        }
        return disk;
    }
    return NULL;
}

// First 64 bits of buffer = LBA offset
// Next 16 bits are the sector count
int ReadSectors(device_id_t device, void* buffer, size_t size, size_t ignored) {
    if(buffer == NULL || size == 0) {
        return DRIVER_FAILURE; // Invalid buffer or size
    }

    device_t* this = GetDeviceByID(device);
    
    // Extract parameters from buffer
    uint64_t* buf = (uint64_t*)buffer;
    lba_t offset = buf[0];
    uint64_t sectorCount = buf[1];

    disk_t* disk = (disk_t*)this->driverData;
    
    // Clear the buffer before reading
    memset(buffer, 0, size);
    uint16_t* dataBuffer = (uint16_t*)buffer;
    
    // Wait for drive to be ready
    WaitForIdle(disk->base);
    
    // Reset error register by reading it
    inb(ErrorPort(disk->base));
    
    // Select the drive and prepare for command
    if(disk->addressing == LBA28) {
        // Limit to 256 sectors for LBA28
        sectorCount = (sectorCount > 256) ? 256 : sectorCount;
        
        // Select master/slave with correct drive bits
        if(disk->slave) {
            outb(DriveSelect(disk->base), SLAVE_DRIVE | ((((uint32_t)offset >> 24) & 0x0F) | 0xE0));
        } else {
            outb(DriveSelect(disk->base), MASTER_DRIVE | ((((uint32_t)offset >> 24) & 0x0F) | 0xE0));
        }
        
        // Make sure drive select completes (crucial on real hardware)
        DiskDelay(disk->base);
        
        // Send command parameters
        outb(FeaturesPort(disk->base), 0);
        
        // If sectorCount is 0, it means 256 sectors in LBA28
        outb(SectorCount(disk->base), (uint8_t)(sectorCount == 256 ? 0 : sectorCount));
        outb(LbaLo(disk->base), (uint8_t)(offset & 0xFF));
        outb(LbaMid(disk->base), (uint8_t)((offset >> 8) & 0xFF));
        outb(LbaHi(disk->base), (uint8_t)((offset >> 16) & 0xFF));
        
        // Wait for drive to be ready to accept command
        WaitForIdle(disk->base);
        
        // Send the read command
        outb(CmdPort(disk->base), COMMAND_READ_SECTORS);
        
        // Handle read process
        for(size_t i = 0; i < sectorCount; i++) {
            // Wait for BSY clear and DRQ set
            uint8_t status;
            int timeout = 1000;  // Add timeout protection
            
            do {
                DiskDelay(disk->base);
                status = inb(StatusPort(disk->base));
                
                // Check for errors
                if(status & FLG_ERR) {
                    uint8_t error = inb(ErrorPort(disk->base));
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
                
                if(--timeout <= 0) {
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
            } while((status & FLG_BSY) || !(status & FLG_DRQ));
            
            // Read the data
            for(size_t j = 0; j < 256; j++) {
                dataBuffer[j + (256 * i)] = inw(DataPort(disk->base));
            }
        }
        
    } else if(disk->addressing == LBA48) {
        // Select drive
        if(disk->slave) {
            outb(DriveSelect(disk->base), 0x50);
        } else {
            outb(DriveSelect(disk->base), 0x40);
        }
        
        // Make sure drive select completes
        DiskDelay(disk->base);
        WaitForIdle(disk->base);
        
        // Send high bytes first
        outb(FeaturesPort(disk->base), 0);
        outb(SectorCount(disk->base), (uint8_t)((sectorCount >> 8) & 0xFF));
        outb(LbaLo(disk->base), (uint8_t)((offset >> 24) & 0xFF));
        outb(LbaMid(disk->base), (uint8_t)((offset >> 32) & 0xFF));
        outb(LbaHi(disk->base), (uint8_t)((offset >> 40) & 0xFF));
        
        // Send low bytes next
        outb(FeaturesPort(disk->base), 0);
        outb(SectorCount(disk->base), (uint8_t)(sectorCount & 0xFF));
        outb(LbaLo(disk->base), (uint8_t)(offset & 0xFF));
        outb(LbaMid(disk->base), (uint8_t)((offset >> 8) & 0xFF));
        outb(LbaHi(disk->base), (uint8_t)((offset >> 16) & 0xFF));
        
        // Send the read extended command
        outb(CmdPort(disk->base), COMMAND_READ_SECTORS_EXT);
        
        // Handle read process with retries
        for(size_t i = 0; i < sectorCount; i++) {
            // Wait for BSY clear and DRQ set with timeout
            uint8_t status;
            int timeout = 1000;
            
            do {
                DiskDelay(disk->base);
                status = inb(StatusPort(disk->base));
                
                // Check for errors
                if(status & FLG_ERR) {
                    uint8_t error = inb(ErrorPort(disk->base));
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
                
                if(--timeout <= 0) {
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
            } while((status & FLG_BSY) || !(status & FLG_DRQ));
            
            // Read the data
            for(size_t j = 0; j < 256; j++) {
                dataBuffer[j + (256 * i)] = inw(DataPort(disk->base));
            }
        }
        
    } else {
        // CHS mode (legacy)
        uint16_t cylinder = offset / (disk->numHeads * disk->numSectors);
        uint8_t head = (offset / disk->numSectors) % disk->numHeads;
        uint8_t sector = (offset % disk->numSectors) + 1;
        
        // Select drive with head information
        if(disk->slave) {
            outb(DriveSelect(disk->base), SLAVE_DRIVE | (head & 0x0F));
        } else {
            outb(DriveSelect(disk->base), MASTER_DRIVE | (head & 0x0F));
        }
        
        // Make sure drive select completes
        DiskDelay(disk->base);
        WaitForIdle(disk->base);
        
        // Send parameters
        outb(SectorCount(disk->base), (uint8_t)sectorCount);
        outb(LbaLo(disk->base), sector);
        outb(LbaMid(disk->base), (uint8_t)(cylinder & 0xFF));
        outb(LbaHi(disk->base), (uint8_t)((cylinder >> 8) & 0xFF));
        
        // Send read command
        outb(CmdPort(disk->base), COMMAND_READ_SECTORS);
        
        // Handle read with timeout protection
        for(size_t i = 0; i < sectorCount; i++) {
            uint8_t status;
            int timeout = 1000;
            
            do {
                DiskDelay(disk->base);
                status = inb(StatusPort(disk->base));
                
                if(status & FLG_ERR) {
                    uint8_t error = inb(ErrorPort(disk->base));
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
                
                if(--timeout <= 0) {
                    SoftwareReset(disk);
                    return DRIVER_FAILURE;
                }
            } while((status & FLG_BSY) || !(status & FLG_DRQ));
            
            // Read the data
            for(size_t j = 0; j < 256; j++) {
                dataBuffer[j + (256 * i)] = inw(DataPort(disk->base));
            }
        }
    }
    
    // Ensure flush completes
    FlushCache(disk->base);
    
    // Reset the drive for next operation
    SoftwareReset(disk);
    
    return DRIVER_SUCCESS;
}

int WriteSectors(device_id_t this, void* buffer, size_t size, size_t offset){
    // Not implemented yet
    return DRIVER_FAILURE;
}

// Write sectors function...

int ProcessIoctl(int cmd, void* argp, device_id_t device){
    disk_t* disk = (disk_t*)GetDeviceByID(device)->driverData;
    switch(cmd){
        // IOCTL requests
        case 1:{
            FlushCache(disk->base);
            return DRIVER_SUCCESS;
        }
        case 2:{
            SoftwareReset(disk);
            return DRIVER_SUCCESS;
        }
        default:{
            return DRIVER_FAILURE;
        }
    }
}

static device_t** devices = NULL;

/* TODO:
 * - Add a write function
 * - Replace the many allocations with a single allocation and split that up here, at least for the drive names (known as an arena)
 * - Make a driver specifically for more advanced disk interaction and load it from the disk
*/
int InitializeAta(){
    driver_t* ataDriver = halloc(sizeof(driver_t));
    if(ataDriver == NULL){
        return DRIVER_FAILURE;
    }
    memset(ataDriver, 0, sizeof(driver_t));

    ataDriver->name = "ata";
    ataDriver->class = DEVICE_CLASS_BLOCK;
    ataDriver->type = DEVICE_TYPE_STORAGE;
    ataDriver->init = InitializeAta;
    ataDriver->deinit = NULL;
    ataDriver->probe = NULL;

    // Register the driver
    RegisterDriver(ataDriver, true);
    
    uint8_t currentDisk = 0;
    uint8_t foundDisks = 0;
    char diskName[10] = "/dev/pat0";

    devices = halloc(sizeof(device_t*) * MAX_ATA_DRIVES);
    if(devices == NULL){
        hfree(ataDriver);
        return DRIVER_FAILURE;
    }
    memset(devices, 0, sizeof(device_t*) * MAX_ATA_DRIVES);

    for(int i = 0; i < MAX_ATA_DRIVES; i++){
        devices[i] = halloc(sizeof(device_t));
        if(devices[i] == NULL){
            // Failed to allocate memory for device
            hfree(devices);
            hfree(ataDriver);
            return DRIVER_FAILURE;
        }
        memset(devices[i], 0, sizeof(device_t));
        disk_t* currentDisk = FindDisk(i, devices[i]);
        if(currentDisk == NULL){
            // No disk at this position
            hfree(devices[i]);
            devices[i] = NULL;
            continue;
        }
        devices[i]->name = "ATA disk";
        devices[i]->class = DEVICE_CLASS_BLOCK;
        devices[i]->type = DEVICE_TYPE_STORAGE;
        devices[i]->driver = ataDriver;
        devices[i]->ops.read = ReadSectors;
        devices[i]->ops.write = WriteSectors;
        devices[i]->ops.ioctl = ProcessIoctl;
        devices[i]->driverData = (void*)currentDisk;
        RegisterDevice(devices[i], diskName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        diskName[9]++;
    }

    if(foundDisks == 0){
        // No disks found
        hfree(ataDriver);
        return DRIVER_NOT_SUPPORTED;
    }

    return DRIVER_SUCCESS;
}