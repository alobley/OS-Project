#include <mbr.h>
#include <drivers.h>
#include <kernel.h>
#include <alloc.h>

lba ChsToLba(uint16_t head, uint16_t sector, uint16_t cylinder, blkdev_t* disk){
    // Ensure valid CHS values
    lba result = ((cylinder * disk->numHeads) + head) * disk->numSectors + (sector - 1);
    printk("CHS (%u, %u, %u) -> LBA %llu\n", cylinder, head, sector, result);
    return result;
}

DRIVERSTATUS GetPartitionsFromMBR(device_t* disk){
    if(disk == NULL){
        printk("No disk!\n");
        //STOP
        return DRIVER_FAILURE; // Invalid disk
    }
    mbr_t* mbr = (mbr_t*)halloc(sizeof(mbr_t));
    if(mbr == NULL){
        printk("Out of memory!\n");
        //STOP
        return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for MBR
    }
    memset(mbr, 0, sizeof(mbr_t));

    uintptr_t addr = (uintptr_t)mbr;
    uint64_t* buf = (uint64_t*)addr;
    buf[0] = 0;                                                     // Set the LBA to read
    buf[1] = 1;                                                     // Set the sector count to 1
    if(device_read(disk->id, (void*)mbr, sizeof(mbr_t)) == DRIVER_FAILURE){
        hfree(mbr);
        printk("Read failed!\n");
        //STOP
        return DRIVER_FAILURE; // Failed to read MBR
    }

    if(!IsValidMBR(mbr)){
        hfree(mbr);
        printk("Invalid MBR!\n");
        //STOP
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
                printk("Out of memory!\n");
                STOP
                return DRIVER_OUT_OF_MEMORY; // Failed to allocate memory for partition
            }
            memset(partition, 0, sizeof(partition_t));

            uint64_t diskSize = ((blkdev_t*)disk->deviceInfo)->size * ((blkdev_t*)disk->deviceInfo)->sectorSize;

            partition->start = mbr->partitions[i].startLBA;
            partition->end = mbr->partitions[i].startLBA + mbr->partitions[i].sizeLBA - 1;
            printk("Size of the disk: %llu Gigabytes\n", diskSize / 1024 / 1024 / 1024 / 1024);
            printk("Partition start head: %u, start cylinder: %u, start sector: %u\n", mbr->partitions[i].startHead, mbr->partitions[i].startCylinder, mbr->partitions[i].startSector);
            //printk("Partition end head: %u, end cylinder: %u, end sector: %u\n", mbr->partitions[i].endHead, mbr->partitions[i].endCylinder, mbr->partitions[i].endSector);
            printk("Partition start: %llu\n", partition->start);
            printk("Partition end: %llu\n", partition->end);
            //printk("Size of the disk: %llu\n", diskSize);
            //STOP
            partition->size = (partition->end - partition->start + 1) / ((blkdev_t*)disk->deviceInfo)->sectorSize;
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
            //return DRIVER_FAILURE;
        }
    }

    printk("Partitions successfully retrieved from MBR!\n");
    return DRIVER_SUCCESS;
}