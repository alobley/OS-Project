#include <fat.h>
#include <kernel.h>
#include <stdint.h>
#include <vfs.h>
#include <mbr.h>

char* partName = "p1";

DRIVERSTATUS ProbeFATFilesystem(device_t* device){
    //printf("Probing...\n");
    if(device->type == DEVICE_TYPE_BLOCK){
        // Check if the device is a FAT filesystem
        if(((blkdev_t*)device->deviceInfo)->firstPartition != NULL){
            // This device has a partition, check if it's FAT
            partition_t* partition = ((blkdev_t*)device->deviceInfo)->firstPartition;
            while(partition != NULL){
                if(partition->filesystem != NULL && (partition->filesystem->fs == FS_FAT12 || partition->filesystem->fs == FS_FAT16 || partition->filesystem->fs == FS_FAT32)){
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
            do_syscall(SYS_DEVICE_READ, (uint32_t)device, (uint32_t)mbr, sizeof(mbr_t), 0, 0);
            if(IsValidMBR(mbr)){
                // Just return a success for now since this isn't completely implemented

                // Check if the filesystem is FAT...

                char* name = (char*)halloc(7);
                memset(name, 0, 7);
                strcpy(name, device->devName);
                strcat(name, partName);                 // No need to manipulate partName since no MBR partitioning means only one partition
                do_syscall(SYS_ADD_VFS_DEV, 0 /*Change this to the filesystem struct*/, (uint32_t)name, (uint32_t)"/dev", 0, 0);

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

// File Reading...

// File Writing...

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