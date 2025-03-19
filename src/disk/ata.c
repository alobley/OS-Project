#include <ata.h>
#include <kernel.h>
#include <interrupts.h>
#include <console.h>
#include <alloc.h>
#include <mbr.h>
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
        do_syscall(SYS_IO_PORT_READ, GetCtrl(basePort), 8, 0, 0, 0);
    }
}

bool IsBusy(uint16_t basePort){
    uint32_t status = 0;
    do_syscall(SYS_IO_PORT_READ, StatusPort(basePort), 8, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (status));
    return (status & FLG_BSY) != 0;
}

void FlushCache(uint16_t basePort){
    do_syscall(SYS_IO_PORT_WRITE, CmdPort(basePort), 8, COMMAND_FLUSH_CACHE, 0, 0);
    DiskDelay(basePort);
}

void SelectDisk(disk_t* disk){
    if(disk->slave){
        do_syscall(SYS_IO_PORT_WRITE, DriveSelect(disk->base), 8, SLAVE_DRIVE, 0, 0);
    }else{
        do_syscall(SYS_IO_PORT_WRITE, DriveSelect(disk->base), 8, MASTER_DRIVE, 0, 0);
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

void SoftwareReset(blkdev_t* blkdev){
    // Software reset
    outb(CmdPort(blkdev->basePort), CMD_SRST);
    DiskDelay(blkdev->basePort);
}

void DetermineAddressing(disk_t* disk){
    // Default to CHS
    disk->addressing = CHS_ONLY;
    disk->size = 0;

    if(disk->packet == false){
        // Disk is a hard drive or other NVRAM
        disk->sectorSize = 512;                             // Unfortunately I have to assume this (better way?)
        if(disk->infoBuffer[83] & (1 << 10)){
            // LBA48 supported
            disk->addressing = LBA48;
        }

        if(disk->addressing == CHS_ONLY){
            // If not 48-bit LBA, check for 28-bit LBA
            uint32_t lba28Sectors = (disk->infoBuffer[60] << 16) | (disk->infoBuffer[61]);
            if(lba28Sectors > 0){
                disk->addressing = LBA28;
                disk->size = lba28Sectors;
            }else{
                // The disk is truly CHS only
                // Get the size of the disk from the number of cylinders, the number of heads, and the number of sectors per track, respectively.
                uint64_t chsSectors = disk->infoBuffer[9] * disk->infoBuffer[11] * disk->infoBuffer[13];
                disk->size = chsSectors;
            }
        }else{
            // If 48-bit LBA, get the number of 48-bit LBA sectors
            uint64_t* lba48Sectors = (uint64_t*)&disk->infoBuffer[100];
            if(*lba48Sectors > 0){
                disk->size = *lba48Sectors;
            }else{
                // No 48-bit LBA
                uint32_t lba28Sectors = *(uint32_t*)&disk->infoBuffer[60];
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

DRIVERSTATUS FindDisk(uint8_t diskno, device_t* ataDevice, blkdev_t* ataBlkDev){
    // Search for PATA devices
    uint32_t status = 0;
    do_syscall(SYS_IO_PORT_READ, StatusPort(PRIMARY_BUS_BASE), 8, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (status));
    if(status != FLOATING_BUS){
        disk_t* disk = IdentifyDisk(diskno);
        if(disk == NULL){
            return -1;
        }
        ataBlkDev->chs = disk->addressing == CHS_ONLY;
        ataBlkDev->size = disk->size * disk->sectorSize;
        ataBlkDev->sectorSize = disk->sectorSize;
        ataBlkDev->lba28 = disk->addressing == LBA28;
        ataBlkDev->lba48 = disk->addressing == LBA48;
        ataDevice->flags.removable = disk->removable;
        ataDevice->flags.readonly = disk->type == PATAPIDISK;
        ataBlkDev->slave = disk->slave;
        ataBlkDev->basePort = disk->base;
        hfree(disk->infoBuffer);
        hfree(disk);
    }
    return 0;
}

// First 64 bits of buffer = LBA offset
// Next 16 bits are the sector count
DRIVERSTATUS ReadSectors(device_t* this, void* buffer, size_t size){
    if(buffer == NULL || size == 0){
        //printk("Invalid buffer or size\n");
        return DRIVER_FAILURE; // Invalid buffer or size
    }
    uint64_t* buf = (uint64_t*)buffer;
    lba offset = buf[0];
    uint64_t sectorCount = buf[1];
    //printk("%u\n", sectorCount);

    //printk("Reading LBA: %lu, Sector Count: %u\n", offset, sectorCount);

    blkdev_t* blkdev = (blkdev_t*)this->deviceInfo;
    if(blkdev->slave){
        // Read from the slave drive
        if(blkdev->lba28){
            // Read using LBA28
            sectorCount = (sectorCount > 256) ? 256 : sectorCount; // Limit to 256 sectors
            outb(DriveSelect(blkdev->basePort), SLAVE_DRIVE | (((uint32_t)offset >> 24) & 0x0F));
        }else if(blkdev->lba48){
            // Read using LBA48
            outb(DriveSelect(blkdev->basePort), 0x50);
        }else{
            // Convert CHS to LBA...
            //printk("CHS to LBA conversion not implemented yet\n");
            return DRIVER_FAILURE; // Not implemented yet
        }
    }else{
        if(blkdev->lba28){
            // Read using LBA28
            sectorCount = (sectorCount > 256) ? 256 : sectorCount; // Limit to 256 sectors
            outb(DriveSelect(blkdev->basePort), MASTER_DRIVE | (((uint32_t)offset >> 24) & 0x0F));
        }else if(blkdev->lba48){
            // Read using LBA48
            outb(DriveSelect(blkdev->basePort), 0x40);
        }else{
            // Convert CHS to LBA...
            //printk("CHS to LBA conversion not implemented yet\n");
            return DRIVER_FAILURE; // Not implemented yet
        }
    }
    DiskDelay(blkdev->basePort);

    if(offset > blkdev->size){
        //printk("Invalid offset\n");
        return DRIVER_FAILURE; // Invalid offset
    }

    // Clear the buffer
    memset(buffer, 0, size);
    uint16_t* dataBuffer = (uint16_t*)buffer;

    if(size < blkdev->sectorSize * sectorCount){
        //printk("%u\n", sectorCount);
        //STOP
        return DRIVER_FAILURE; // Invalid size
    }

    if(blkdev->lba28){
        offset = (uint32_t)offset;
        outb(FeaturesPort(blkdev->basePort), 0);
        outb(SectorCount(blkdev->basePort), sectorCount);
        outb(LbaLo(blkdev->basePort), (uint8_t)(offset & 0xFF));
        outb(LbaMid(blkdev->basePort), (uint8_t)((offset >> 8) & 0xFF));
        outb(LbaHi(blkdev->basePort), (uint8_t)((offset >> 16) & 0xFF));

        WaitForIdle(blkdev->basePort);

        outb(CmdPort(blkdev->basePort), COMMAND_READ_SECTORS);

        for(size_t i = 0; i < sectorCount; i++){
            WaitForIdle(blkdev->basePort);
            WaitForDrq(blkdev->basePort);
            for(size_t j = 0; j < 256; j++){
                dataBuffer[j + (256 * i)] = inw(DataPort(blkdev->basePort));
                if(inb(ErrorPort(blkdev->basePort)) != 0){
                    //printk("Error reading from disk\n");
                    SoftwareReset(blkdev);
                    return DRIVER_FAILURE; // Error reading
                }
            }
        }

        DiskDelay(blkdev->basePort);
        FlushCache(blkdev->basePort);
    }else if(blkdev->lba48){
        WaitForIdle(blkdev->basePort);

        // Send the high bytes
        outb(SectorCount(blkdev->basePort), (uint8_t)((sectorCount >> 8) & 0xFF));
        outb(LbaLo(blkdev->basePort), (uint8_t)((offset >> 24) & 0xFF));
        outb(LbaMid(blkdev->basePort), (uint8_t)((offset >> 32) & 0xFF));
        outb(LbaHi(blkdev->basePort), (uint8_t)((offset >> 40) & 0xFF));

        // Send the low bytes
        outb(SectorCount(blkdev->basePort), (uint8_t)((sectorCount & 0xFF)));
        outb(LbaLo(blkdev->basePort), (uint8_t)(offset & 0xFF));
        outb(LbaMid(blkdev->basePort), (uint8_t)((offset >> 8) & 0xFF));
        outb(LbaHi(blkdev->basePort), (uint8_t)((offset >> 16) & 0xFF));

        WaitForIdle(blkdev->basePort);
        outb(CmdPort(blkdev->basePort), COMMAND_READ_SECTORS_EXT);

        int retries = 3;
        while(retries-- > 0){
            for(int i = 0; i < 4; i++){
                inb(ErrorPort(blkdev->basePort));
            }

            if(inb(ErrorPort(blkdev->basePort)) == 0){
                break;
            }

            if(retries == 0){
                SoftwareReset(blkdev);
                return DRIVER_FAILURE; // Error reading
            }
        }

        WaitForDrq(blkdev->basePort);
        for(size_t i = 0; i < sectorCount; i++){
            WaitForIdle(blkdev->basePort);
            WaitForDrq(blkdev->basePort);
            for(size_t j = 0; j < 256; j++){
                dataBuffer[j + (256 * i)] = inw(DataPort(blkdev->basePort));
                if(inb(ErrorPort(blkdev->basePort)) != 0){
                    //printk("Error reading from disk\n");
                    SoftwareReset(blkdev);
                    return DRIVER_FAILURE; // Error reading
                }
            }
        }
    }else{
        // CHS to LBA conversion not implemented yet
        //printk("CHS to LBA conversion not implemented yet\n");
        return DRIVER_FAILURE; // Not implemented yet
    }

    SoftwareReset(blkdev);
    return DRIVER_SUCCESS;
}

DRIVERSTATUS WriteSectors(device_t* this, void* buffer, size_t size){
    // Not implemented yet
    return DRIVER_FAILURE;
}

// Write sectors function...

DRIVERSTATUS ProcessIoctl(device_t* this, IOCTL_CMD request, void* argp){
    switch(request){
        // IOCTL requests
        case IOCTL_FLUSH_CACHE:{
            blkdev_t* blkdev = (blkdev_t*)this->deviceInfo;
            FlushCache(blkdev->basePort);
            return DRIVER_SUCCESS;
        }
        case IOCTL_GET_STATUS:{
            // Get the status of the device
            return this->status;
        }
        case IOCTL_RESET_DEVICE:{
            blkdev_t* blkdev = (blkdev_t*)this->deviceInfo;
            SoftwareReset(blkdev);
            return DRIVER_SUCCESS;
        }
        default:{
            return DRIVER_FAILURE;
        }
    }
}

char* name = "pat0";

/* TODO:
 * - Add a write function
 * - Replace the many allocations with a single allocation and split that up here, at least for the drive names (known as an arena)
 * - Make a driver specifically for more advanced disk interaction and load it from the disk
*/
DRIVERSTATUS InitializeAta(){
    driver_t* ataDriver = CreateDriver("ATA Driver", "A simple PIO PATA driver", 1, DEVICE_TYPE_BLOCK, NULL, NULL, NULL);
    if(ataDriver == NULL){
        return DRIVER_OUT_OF_MEMORY;
    }

    do_syscall(SYS_GET_PID, 0, 0, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (ataDriver->driverProcess));

    blkdev_t* ataBlkDev = (blkdev_t*)halloc(sizeof(blkdev_t));
    if(ataBlkDev == NULL){
        hfree(ataDriver);
        return DRIVER_OUT_OF_MEMORY;
    }
    memset(ataBlkDev, 0, sizeof(blkdev_t));

    // Create the first PATA device to assign
    device_t* ataDevice = CreateDevice("PATA Disk", strcpy(halloc(5), name), "PATA Hard Drive", (void*)ataBlkDev, ataDriver, DEVICE_TYPE_BLOCK, 0, (device_flags_t){0}, ReadSectors, WriteSectors, ProcessIoctl, NULL);
    
    ataBlkDev->device = memcpy(halloc(sizeof(user_device_t)), ataDevice->userDevice, sizeof(user_device_t));
    ataBlkDev->firstPartition = NULL;
    ataBlkDev->next = NULL;
    ataDevice->deviceInfo = ataBlkDev;

    blkdev_t* currentBlk = ataBlkDev;
    device_t* currentDev = ataDevice;

    uint8_t currentDisk = 0;

    for(int i = 0; i < MAX_ATA_DRIVES; i++){
        if(FindDisk(currentDisk, ataDevice, ataBlkDev) == 0){
            do_syscall(SYS_REGISTER_DEVICE, (uintptr_t)ataDevice, 0, 0, 0, 0);
            do_syscall(SYS_ADD_VFS_DEV, (uint32_t)ataDevice, (uint32_t)ataDevice->devName, (uint32_t)"/dev", 0, 0);

            currentBlk = ataBlkDev;
            currentDev = ataDevice;

            name[3]++; // Increment the device name (i.e. pat0 -> pat1)

            if(i != MAX_ATA_DRIVES - 1){
                ataBlkDev = (blkdev_t*)halloc(sizeof(blkdev_t));
                if(ataBlkDev == NULL){
                    hfree(ataDriver);
                    hfree((char*)ataDevice->devName);
                    hfree(ataDevice);
                    hfree(ataBlkDev);
                    return DRIVER_OUT_OF_MEMORY;
                }
                memset(ataBlkDev, 0, sizeof(blkdev_t));
                ataDevice = CreateDevice("PATA Disk", strcpy(halloc(5), name), "PATA Hard Drive", (void*)ataBlkDev, ataDriver, DEVICE_TYPE_BLOCK, 0, (device_flags_t){0}, ReadSectors, WriteSectors, ProcessIoctl, NULL);
                //currentBlk->next = ataBlkDev;
                //currentDev->next = ataDevice;
                ataBlkDev->device = memcpy(halloc(sizeof(user_device_t)), ataDevice->userDevice, sizeof(user_device_t));
                ataBlkDev->firstPartition = NULL;
                ataBlkDev->next = NULL;
                ataDevice->deviceInfo = ataBlkDev;
            }
        }else{
            currentBlk->next = NULL;
            currentDev->next = NULL;
        }
        currentDisk++;
    }

    if(currentDisk == 0){
        // No disks found
        hfree(ataDriver);
        hfree((char*)ataDevice->devName);
        hfree(ataDevice);
        hfree(ataBlkDev);
        return DRIVER_NOT_SUPPORTED;
    }

    // Register the driver
    do_syscall(SYS_MODULE_LOAD, (uint32_t)ataDriver, (uint32_t)ataDevice, 0, 0, 0);

    return 0;
}