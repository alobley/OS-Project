#include <disk.h>
#include <alloc.h>
#include <ahci.h>
#include <ata.h>


// Low-level disk function for reading sectors. Abstracts away the details of the disk interface.
void* ReadSector(media_descriptor_t* media, uint64_t firstLba, uint64_t numSectors) {
    // Check the disk interface, addressing scheme, etc...

    // Allocate memory for the sector data

    // Read the sector data

    return NULL;
}