#include <disk.h>
#include <alloc.h>
#include <ahci.h>
#include <ata.h>
#include <vfs.h>

media_descriptor_t* CreateMediaDescriptor(device_t* device, uint8_t version, uint64_t size, disktype_t type, mediafs_t fs, disk_status_t status, diskflags_t flags, disk_interface_t* interface) {
    media_descriptor_t* media = (media_descriptor_t*)halloc(sizeof(media_descriptor_t));
    media->device = device;
    media->version = version;
    media->size = size;
    media->type = type;
    media->fs = fs;
    media->status = status;
    media->flags = flags;
    media->interface = interface;
    return media;
}


// Low-level disk function for reading sectors. Abstracts away the details of the disk interface.
void* ReadSector(media_descriptor_t* media, uint64_t firstLba, uint64_t numSectors) {
    // Check the disk interface, addressing scheme, etc...

    // Allocate memory for the sector data

    // Read the sector data

    return NULL;
}

int WriteSector(media_descriptor_t* media, uint64_t firstLba, uint64_t numSectors, void* data) {
    // Check the disk interface, addressing scheme, etc...

    // Write the sector data

    return WRITE_SUCCESS;
}