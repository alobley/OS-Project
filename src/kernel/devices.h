#ifndef DEVICES_H
#define DEVICES_H

/* Notes:
 * - A USB keyboard will be like this: USB hub device -> USB device (child of hub) -> Keyboard device (child of USB device)
 * - A disk will be like this: Disk device -> Partition device (child of disk) -> Filesystem device (child of partition)
 * - To start off, the first disk driver will manually detect the devices and partitions and create the devices for them.
 * - The kernel will have a device registry that contains all the devices.
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DEVICE_STATUS_IDLE,
    DEVICE_STATUS_BUSY,
    DEVICE_STATUS_ERROR,
    DEVICE_STATUS_NO_DRIVER,
    DEVICE_STATUS_UNKNOWN
} device_status_t;

// Define overall device types but nothing specific so the kernel can classify devices and determine their interface
// Note that support for most of these will either be built-in or be contained in one comprehensive driver.
typedef enum {
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_FILESYSTEM,
    DEVICE_TYPE_PCI,
    DEVICE_TYPE_PCIE,
    DEVICE_TYPE_USB,
    DEVICE_TYPE_SERIAL,
    DEVICE_TYPE_PARALLEL,
    DEVICE_TYPE_UNKNOWN
} device_type_t;

typedef unsigned short device_id_t;
typedef unsigned short vendor_id_t;

typedef unsigned long long lba_t;

// ID assigned to a driver by the kernel
typedef unsigned short driver_id_t;

typedef int driverstatus;

#define DRIVER_SUCCESS 0
#define DRIVER_FAILURE -1
#define DRIVER_NOT_IMPLEMENTED -2
#define DRIVER_NOT_FOUND -3
#define DRIVER_DEVICE_BUSY -4

typedef struct Driver driver_t;
typedef struct Device device_t;

typedef struct Device {
    device_id_t deviceID;
    vendor_id_t vendorID;
    device_status_t status;
    char* name;
    char* description;
    void* deviceInfo;                   // Device specific information (like the Filesystem struct below)
    driver_t* driver;                   // The driver responsible for this device

    device_type_t type;
    struct Device* next;
    struct Device* prev;                // Needed?
    struct Device* parent;
    struct Device* firstChild;
} device_t;

// Definition of a filesystem on a device
// This is a structure that contains the functions for a filesystem driver.
typedef struct Filesystem {
    char* fsName;
    char* fsType;
    driverstatus (*mount)(void* device, char* path);                                      // Mount the filesystem on the VFS
    driverstatus (*unmount)(void* device, char* path);                                    // Unmount the filesystem from the VFS
    driverstatus (*read)(void* device, char* path, void* buffer, size_t size);            // Read a file from the filesystem
    driverstatus (*write)(void* device, char* path, void* buffer, size_t size, bool dir); // Write a file or directory to the filesystem
} filesystem_t; 

// A partition on a disk device
typedef struct Partition {
    filesystem_t* fs;
    lba_t startLBA;
    lba_t endLBA;
    lba_t numSectors;
    size_t sectorSize;
    char* name;
    struct Partition* next;
    struct Partition* prev;
} partition_t;

// Disk devices
typedef struct Block_Device {
    device_t* device;                   // The device this block device represents
    lba_t numSectors;
    size_t sectorSize;
    uint32_t numPartitions;
    driverstatus (*read)(lba_t sector, size_t numSectors, void* buffer);
    driverstatus (*write)(lba_t sector, size_t numSectors, void* buffer);
} block_device_t;

// Add more device types, i.e. network devices, keyboard, mouse, pci devices, etc...

typedef struct Driver {
    char* name;
    char* description;
    driverstatus (*init)();                 // Initialize the driver
    device_t* device;                       // The device this driver is responsible for
} driver_t;

typedef struct Device_Registry {
    device_t* firstDevice;
    device_t* lastDevice;
    size_t numDevices;
} device_registry_t;

extern device_registry_t* deviceRegistry;

#define REGISTER_SUCCESS 0
#define REGISTER_FAILURE -1
#define REGISTER_DEVICE_NOT_FOUND -2
#define REGISTER_OUT_OF_MEMORY -3
#define REGISTER_DEVICE_ALREADY_REGISTERED -4

int RegisterDevice(device_t* device, device_t* parent);
int UnregisterDevice(device_t* device);

int RegisterDriver(driver_t* driver, device_t* device);
int UnregisterDriver(driver_t* driver, device_t* device);

device_t* GetDeviceByID(device_id_t deviceID);
device_t* GetDeviceByName(char* name);
device_t* GetFirstDevice(device_type_t type);           // Get the first device of a certain type

device_t* CreateDevice(device_id_t deviceID, vendor_id_t vendorID, device_status_t status, char* name, char* description, void* deviceInfo, device_type_t type);
int DestroyDevice(device_t* device);

driver_t* CreateDriver(char* name, char* description, driverstatus (*init)());
int DestroyDriver(driver_t* driver);

int InitDeviceRegistry();
int DestroyDeviceRegistry();        // This should be called before the kernel exits (i.e. on shutdown)

#endif // DEVICES_H