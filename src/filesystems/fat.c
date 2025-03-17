#include <fat.h>
#include <kernel.h>
#include <stdint.h>
#include <vfs.h>
#include <mbr.h>

char* partName = "p1";

// File Reading...

// File Writing...

// TODO: actually detect and validate a FAT filesytstem
DRIVERSTATUS ProbeFATFilesystem(device_t* device){
    //printf("Probing...\n");
    if(device->type == DEVICE_TYPE_BLOCK){
        // Check if the device is a FAT filesystem
        if(((blkdev_t*)device->deviceInfo)->firstPartition != NULL){
            // This device has a partition, check if it's FAT
            partition_t* partition = ((blkdev_t*)device->deviceInfo)->firstPartition;
            while(partition != NULL){
                if(partition->fsID == ID_FAT12 || partition->fsID == ID_SMALL_FAT16 || partition->fsID == ID_BIG_FAT16 || partition->fsID == ID_FAT32 || partition->fsID == ID_FAT32_LBA || partition->fsID == ID_EXFAT){
                    // Make a device entry in the VFS for each partition, saying this driver supports it...
                    return DRIVER_INITIALIZED; // This driver supports this device, the kernel can assign it to the device
                }
                partition = partition->next;
            }
            //printf("No FAT filesystem found on this device!\n");
            return DRIVER_NOT_SUPPORTED; // No FAT filesystem found on this device
        }else{
            // Check BPB
            mbr_t* mbr = (mbr_t*)halloc(sizeof(mbr_t));
            if(mbr == NULL){
                //printf("Out of mnemory!\n");
                return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for MBR
            }
            *(uint64_t*)mbr = 0;
            *(((uint8_t*)mbr) + 8) = 1;
            device_read(device->id, (void*)mbr, sizeof(mbr_t)); // Read the first sector of the disk
            //do_syscall(SYS_DEVICE_READ, device->id, (uint32_t)mbr, sizeof(mbr_t), 0, 0);
            if(IsValidMBR(mbr)){
                // Just return a success for now since this isn't completely implemented

                // Check if the filesystem is FAT...

                char* name = (char*)halloc(7);
                memset(name, 0, 7);
                strcpy(name, device->devName);
                strcat(name, partName);                 // No need to manipulate partName since no MBR partitioning means only one partition

                partition_t* partition = (partition_t*)halloc(sizeof(partition_t));
                memset(partition, 0, sizeof(partition_t));
                device_t* fsDevice = CreateDevice("FAT Filesystem", name, "FAT Filesystem", partition, NULL/*The kernel will assign this*/, DEVICE_TYPE_FILESYSTEM, 0, (device_flags_t){0}, NULL, NULL, NULL, device);
                register_device(fsDevice);                  // Register the device with the kernel
                add_vfs_device(fsDevice, name, "/dev");     // Add the device to the VFS

                hfree(mbr);
                return DRIVER_INITIALIZED;
            }else{
                //printf("Invalid MBR!\n");
                return DRIVER_NOT_SUPPORTED; // This driver does not support this device
            }
        }
    }
    //printf("Not a block device!\n");
    return DRIVER_NOT_SUPPORTED; // This driver does not support this device
}

// FAT driver initialization
DRIVERSTATUS InitializeFAT(){
    driver_t* fatDriver = CreateDriver("FAT", "FAT Filesystem Driver", 0x0001, DEVICE_TYPE_FILESYSTEM, NULL, NULL, ProbeFATFilesystem);
    if(fatDriver == NULL){
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for FAT driver
    }
    do_syscall(SYS_MODULE_LOAD, (uint32_t)fatDriver, 0, 0, 0, 0);
    //RegisterDriver(fatDriver, NULL);
    return DRIVER_SUCCESS;
}