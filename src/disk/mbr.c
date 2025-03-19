#include <mbr.h>
#include <system.h>
#include <kernel.h>
#include <alloc.h>

lba ChsToLba(uint8_t head, uint8_t sector, uint16_t cylinder){
    return (cylinder * 255 * 63) + (head * 63) + (sector - 1);
}

DRIVERSTATUS GetPartitionsFromMBR(device_t* disk){
    if(disk == NULL){
        return DRIVER_FAILURE; // Invalid disk
    }
    mbr_t* mbr = (mbr_t*)halloc(sizeof(mbr_t));
    if(mbr == NULL){
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for MBR
    }
    
    // Read the first sector of the disk to get the MBR
    uint8_t* buffer = halloc(512);
    if(buffer == NULL){
        hfree(mbr);
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for buffer
    }
    
    do_syscall(SYS_DEVICE_READ, (uint32_t)disk->id, (uint32_t)buffer, 512, 0, 0);
    int result = 0;
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != DRIVER_SUCCESS){
        hfree(buffer);
        hfree(mbr);
        return DRIVER_FAILURE; // Failed to read from disk
    }
    memcpy(mbr, buffer, sizeof(mbr_t));
    hfree(buffer);
    if(!IsValidMBR(mbr)){
        hfree(mbr);
        return DRIVER_FAILURE; // Invalid MBR
    }

    int numPartitions = 0;
    // Process the partitions in the MBR
    for(int i = 0; i < 4; i++){
        if(mbr->partitions[i].systemID != 0){
            // Create a partition structure and add it to the disk's partition list
            printk("Partition found!\n");
            partition_t* partition = (partition_t*)halloc(sizeof(partition_t));
            if(partition == NULL){
                hfree(mbr);
                return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for partition
            }
            memset(partition, 0, sizeof(partition_t));
            partition->start = mbr->partitions[i].startLBA;
            partition->end = ChsToLba(mbr->partitions[i].endHead, mbr->partitions[i].endSector, mbr->partitions[i].endCylinder);
            partition->size = partition->end - partition->start + 1;
            partition->device = disk;
            partition->blkdev = (blkdev_t*)disk->deviceInfo;
            if(numPartitions == 0){
                partition->blkdev->firstPartition = partition;
            }else{
                partition_t* current = partition->blkdev->firstPartition;
                while(current->next != NULL){
                    current = current->next;
                }
                current->next = partition;
                partition->previous = current;
            }
            partition->fsID = mbr->partitions[i].systemID;
            partition->next = NULL;
            partition->previous = NULL;

            // Add the partition to the disk's partition list
            if(((blkdev_t*)disk->deviceInfo)->firstPartition == NULL){
                ((blkdev_t*)disk->deviceInfo)->firstPartition = partition;
            }else{
                partition_t* current = ((blkdev_t*)disk->deviceInfo)->firstPartition;
                while(current->next != NULL){
                    current = current->next;
                }
                current->next = partition;
                partition->previous = current;
            }

            numPartitions++;
        }else{
            printk("No partition found!\n");
        }
    }
    return DRIVER_SUCCESS;
}