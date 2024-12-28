#include "ata.h"
#include <io.h>
#include <util.h>
#include <alloc.h>
#include <vga.h>

#define NUM_BUS_PORTS 7

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

#define FLOATING_BUS 0xFF           // If the regular status byte contains this, there are no drives on this bus

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


// Get the control register of a given base
uint16 GetCtrl(uint16 basePort){
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

// Read from the control register 15 times for a 400ns delay
void DiskDelay(uint16 BasePort){
    for(int i = 0; i < 15; i++){
        inb(GetCtrl(BasePort));
    }
}

// Checks if the disk is running a command by checking the BSY bit
bool IsBusy(uint16 basePort){
    return inb(StatusPort(basePort)) & FLG_BSY;
}

// Wait for the BSY bit to clear so the next command can be sent
#pragma GCC push_options
#pragma GCC optimize("O0")
void WaitForIdle(uint16 basePort){
    while(inb(StatusPort(basePort)) & FLG_BSY);
}

void WaitForDrq(uint16 basePort){
    while(inb((StatusPort(basePort)) & FLG_DRQ));
}
#pragma GCC pop_options

// Flush the disk cache, should be done after every write command
void CacheFlush(uint16 basePort){
    outb(basePort, COMMAND_FLUSH_CACHE);
}

// Perform a soft reset on a given disk
void SoftwareReset(disk_t* disk){
    outb(disk->ctrl, CMD_SRST);
    DiskDelay(disk->base);
}


void SelectDisk(disk_t* disk){
    // Select the master or slave depending on the drive
    if(disk->slave){
        outb(DriveSelect(disk->base), SLAVE_DRIVE);
    }else{
        outb(DriveSelect(disk->base), MASTER_DRIVE);
    }

    DiskDelay(disk->base);
}

void DetermineAddressing(disk_t* disk){
    // Default to CHS
    disk->addressing = CHS_ONLY;
    disk->size = 0;

    if(disk->packet == false){
        // Disk is a hard drive or other NVRAM
        disk->sectorSize = HD_SECTORSIZE;       // It's probably safe to assume this
        if(disk->infoBuffer[83] & (1 << 10)){
            // LBA48 supported
            disk->addressing = LBA48;
        }

        if(disk->addressing == CHS_ONLY){
            // If not 48-bit LBA, check for 28-bit LBA
            uint32 lba28Sectors = (disk->infoBuffer[60] << 16) | (disk->infoBuffer[61]);
            if(lba28Sectors > 0){
                disk->addressing = LBA28;
                disk->size = lba28Sectors;
            }else{
                // The disk is truly CHS only
                // Get the size of the disk from the number of cylinders, the number of heads, and the number of sectors per track, respectively.
                uint64 chsSectors = disk->infoBuffer[9] * disk->infoBuffer[11] * disk->infoBuffer[13];
                disk->size = chsSectors;
            }
        }else{
            // If 48-bit LBA, get the number of 48-bit LBA sectors
            uint64* lba48Sectors = (uint64*)&disk->infoBuffer[100];
            if(lba48Sectors > 0){
                disk->size = *lba48Sectors;
            }else{
                // No 48-bit LBA
                uint32 lba28Sectors = *(uint32*)&disk->infoBuffer[60];
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

            uint8 packet[12] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            // This will get the size of the disk in sectors, I guess?
            for(int i = 0; i < 6; i++){
                outw(DataPort(disk->base), ((uint16*)packet)[i]);
            }

            WaitForIdle(disk->base);
            WaitForDrq(disk->base);

            // Turns out it was just two words. Interesting. (everything I read said otherwise, that it was four words.)
            uint8 data[4];
            for(int i = 0; i < 4; i++){
                data[i] = inb(DataPort(disk->base));
            }

            uint32 maxLba = (data[1] << 8) | data[0];
            uint32 sectorSize = (data[3] << 8) | data[2];

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
}


// PATAPI has NOT been implemented yet in this! It needs to be implemented at some point.
disk_t* IdentifyDisk(uint8 diskNum){
    disk_t* disk = (disk_t*)alloc(sizeof(disk_t));
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
            dealloc(disk);
            return NULL;
    }

    if(inb(StatusPort(disk->base)) == FLOATING_BUS){
        // Floating bus, no disks in this bus
        dealloc(disk);
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

    uint8 driveStatus = inb(StatusPort(disk->base));
    if(driveStatus == 0){
        // Drive does not exist
        dealloc(disk);
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
                dealloc(disk);
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

    uint8 status = inb(StatusPort(disk->base));
    while(!(status & 0x08)){
        // Wait until DRQ is ready
        if(status & FLG_ERR){
            // There was an error, the disk cannot be used
            dealloc(disk);
            return NULL;
        }
        status = inb(StatusPort(disk->base));
    }

    uint16* diskBuffer = (uint16*)alloc(512);
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

// Note: partition-relative LBA implementation may be a good idea
uint16* ReadSectors(disk_t* disk, uint16 sectorsToRead /*For LBA28 only the low byte is used*/, uint64 lba){
    printk("Reading %d sectors at LBA %llu\n", sectorsToRead, lba);
    if(disk->populated || !disk->removable){
        // If the disk exists, we can read from it
        // Select the master or slave depending on the drive
        if(disk->slave){
            if(disk->addressing == LBA28){
                sectorsToRead = (uint8)sectorsToRead;
                outb(DriveSelect(disk->base), SLAVE_DRIVE | (((uint32)lba >> 24) & 0x0F));
            }else if(disk->addressing == LBA48){
                // This is shockingly different
                outb(DriveSelect(disk->base), 0x50);
            }else{
                // Convert CHS to LBA28
                // Placeholder
                return NULL;
            }
        }else{
            if(disk->addressing == LBA28){
                sectorsToRead = (uint8)sectorsToRead;
                outb(DriveSelect(disk->base), MASTER_DRIVE | (((uint32)lba >> 24) & 0x0F));
            }else if(disk->addressing == LBA48){
                // This is shockingly different
                outb(DriveSelect(disk->base), 0x40);
            }else{
                // Convert CHS to LBA28
                // Placeholder
                return NULL;
            }
        }
        DiskDelay(disk->base);
    }else{
        // If it doesn't, there's nothing to read.
        return NULL;
    }

    if(lba + sectorsToRead > disk->size){
        // Request outside disk bounds
        return NULL;
    }

    if(sectorsToRead == 0 || (disk->addressing == LBA28 && sectorsToRead > 255)){
        // Invalid sector count
        return NULL;
    }

    // Allocate the buffer that the data will be sent to
    uint16* buffer = alloc(sectorsToRead * disk->sectorSize);
    if(buffer == NULL){
        // Memory allocation error
        return NULL;
    }

    if(disk->addressing == LBA28){
        lba = (uint32)lba;
        outb(FeaturesPort(disk->base), 0x00);
        outb(SectorCount(disk->base), sectorsToRead);
        outb(LbaLo(disk->base), (uint8)lba & 0xFF);
        outb(LbaMid(disk->base), ((uint8)(lba >> 8) & 0xFF));
        outb(LbaHi(disk->base), ((uint8)(lba >> 16) & 0xFF));

        WaitForIdle(disk->base);

        for(int sector = 0; sector < sectorsToRead; sector++){
            WaitForIdle(disk->base);
            WaitForDrq(disk->base);
            for(int i = 0; i < 4; i++){
                inb(ErrorPort(disk->base));
            }
            if(inb(ErrorPort(disk->base)) != 0){
                return NULL;
            }
            
            for(int i = 0; i < 256; i++){
                // Read the sector
                buffer[sector * 256 + i] = inw(DataPort(disk->base));
            }
        }

        // This is technically twice as long as it needs to be
        DiskDelay(disk->base);
    }else if(disk->addressing == LBA48){
        WaitForIdle(disk->base);

        // Send the high bytes
        outb(SectorCount(disk->base), (sectorsToRead >> 8) & 0xFF);
        outb(LbaLo(disk->base), (lba >> 24) & 0xFF);
        outb(LbaMid(disk->base), (lba >> 32) & 0xFF);
        outb(LbaHi(disk->base), (lba >> 40) & 0xFF);

        // Send the low bytes
        outb(SectorCount(disk->base), sectorsToRead & 0xFF);
        outb(LbaLo(disk->base), lba & 0xFF);
        outb(LbaMid(disk->base), (lba >> 8) & 0xFF);
        outb(LbaHi(disk->base), (lba >> 16) & 0xFF);

        WaitForIdle(disk->base);
        outb(CmdPort(disk->base), COMMAND_READ_SECTORS_EXT);

        int retries = 3;
        while (retries-- > 0) {
            for (int i = 0; i < 4; i++) {
                inb(ErrorPort(disk->base));  // Clear errors
            }

            if (inb(ErrorPort(disk->base)) == 0) {
                break;  // No error, proceed
            }

            if (retries == 0) {
                return NULL;  // Give up after retries
            }
        }

        // Wait for the drive to indicate it's ready to transfer data
        WaitForDrq(disk->base);

        for(int sector = 0; sector < sectorsToRead; sector++){
            WaitForIdle(disk->base);
            WaitForDrq(disk->base);
            for(int i = 0; i < 256; i++){
                // Read the sector
                buffer[sector * 256 + i] = inw(DataPort(disk->base));
            }
        }
    }else{
        // Translate CHS to LBA
    }

    return buffer;
}