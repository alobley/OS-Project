#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hmmm, might be worth learning C++ for this kind of stuff

typedef enum Device_Type{
    DEVICE_TYPE_BLOCK,              // Block device (e.g. hard drive)
    DEVICE_TYPE_INPUT,              // Human i nput device (e.g. keyboard)
    DEVICE_TYPE_NETWORK,            // Network device (e.g. Ethernet)
    DEVICE_TYPE_GPU,                // Graphics device (e.g. VGA)
    DEVICE_TYPE_AUDIO,              // Audio device (e.g. sound card)
    DEVICE_TYPE_CONSOLE,            // TTY or VGA
    DEVICE_TYPE_BRIDGE,             // Bridge (e.g. PCI, USB)
    DEVICE_TYPE_LEGACY,             // Legacy device (e.g. ISA, non-USB Serial, Parallel)
    DEVICE_TYPE_DISK_CONTROLLER,    // Disk controller (e.g. AHCI, NVMe)
    DEVICE_TYPE_MISC,               // Miscellaneous/other device
} device_type_t;

typedef struct Driver {
    // Identification for the driver
    char* name;                     // Name of the driver
    bool loaded;                    // Whether the driver is loaded and running
    device_type_t type;             // Type of device the driver supports
    uint32_t vendorId;              // Vendor ID of the driver
    uint32_t deviceId;              // Device ID of the driver

    // More driver stuff...
} driver_t;

typedef enum Device_Status {
    DEVICE_STATUS_OK,              // Device is OK
    DEVICE_STATUS_ERROR,           // Device has an error
    DEVICE_STATUS_NOT_PRESENT,     // Device is not present
    DEVICE_STATUS_NOT_SUPPORTED,   // Device is not supported
    DEVICE_STATUS_NOT_READY,       // Device is not ready
    DEVICE_STATUS_INVALID_DRIVER,  // Device has an invalid driver
    DEVICE_STATUS_UNKNOWN          // Device status is unknown
} device_status_t;

typedef struct Device {
    // Device-specific information
    device_type_t type;             // Type of device
    device_status_t status;         // Status of the device
    void* deviceData;               // Device-specific data (pointer to struct, is a void* to allow for any type of data)
    mutex_t lock;                   // Mutex for the device

    // Device operations (set by drivers)
    int (*init)(struct Device* self);                                      // Initialize the device
    int (*read)(struct Device* self, void* buffer, size_t size);           // Read from the device
    int (*write)(struct Device* self, const void* buffer, size_t size);    // Write to the device
    int (*command)(struct Device* self, ...);                              // Send a command to the device
    int (*cleanup)(struct Device* self);                                   // Cleanup the device

    // Strings for device identification
    char* name;                     // Name of the device
    char* description;              // Description of the device
    char* manufacturer;             // Manufacturer of the device
    char* model;                    // Model of the device
    char* serial;                   // Serial number of the device
    char* firmware;                 // Firmware version of the device

    // Device hierarchy
    struct Device* parent;          // Parent device (if applicable)
    struct Device* children;        // Child devices (if applicable)
    struct Device* next;            // Next sibling device in the list

    // Device driver/bus
    driver_t* driver;               // Driver for the device
    struct Device* bus;             // Bus the device is connected to (if applicable) (Define another struct specifically for buses?)
} device_t;

typedef struct Device_Registry {
    device_t* first;                // First device in the registry (is kind of an unholy abomination of a combination of a linked list and a binary tree)
    size_t numDevices;              // Number of devices
} device_registry_t;

// Functions for device and driver management
void InitializeDeviceRegistry();
device_registry_t* GetDeviceRegistry();
device_t* RegisterDevice(device_t* self);
void UnregisterDevice(device_t* self);

driver_t* RegisterDriver(driver_t* self);
void UnregisterDriver(driver_t* self);

driver_t* CreateDriver(char* name, device_type_t type, uint32_t vendorId, uint32_t deviceId, int (*probe)(device_t* device));
device_t* CreateDevice(char* name, device_type_t type, void* deviceData);

#endif // DEVICES_H