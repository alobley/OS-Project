#include <devices.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>
#include <disk.h>

#define ATA_PIMARY_BASE 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_PRIMARY_STATUS 0x3F6

#define ATA_SECONDARY_BASE 0x170
#define ATA_SECONDARY_CONTROL 0x376
#define ATA_SECONDARY_STATUS 0x376

#define ATA_TERTIARY_BASE 0x1E8
#define ATA_TERTIARY_CONTROL 0x3E6
#define ATA_TERTIARY_STATUS 0x3E6

#define ATA_QUATERNARY_BASE 0x168
#define ATA_QUATERNARY_CONTROL 0x366
#define ATA_QUATERNARY_STATUS 0x366


driver_t* ataDriver = NULL;
device_t* ataDevice = NULL;

media_descriptor_t* ataMediaDescriptor = NULL;

uint8_t numAtaDisks = 0;

void InitializeATA(){
    ataDriver = CreateDriver("ATA", DEVICE_TYPE_STORAGE, 0, 0, NULL);
    ataDevice = CreateDevice(DEVICE_TYPE_STORAGE, DEVICE_STATUS_OK, NULL, ataDriver, "ATA", "ATA Storage Device", "ATA", NULL);
    AddDevice(ataDevice, DEVICE_TYPE_STORAGE);

    char* diskNameBuf = (char*)halloc(4);
    memset(diskNameBuf, 0, 4);
    strcpy(diskNameBuf, "pat");
    char* diskName = strcat(diskNameBuf, numAtaDisks + '0');
    char* pathBuf = (char*)halloc(10);
    memset(pathBuf, 0, 10);
    strcpy(pathBuf, "/dev/pat");
    char* path = strcat(pathBuf, numAtaDisks + '0');
    numAtaDisks++;
    VfsAddDevice(ataDevice, diskName, path);
}