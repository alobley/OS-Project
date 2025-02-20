#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum Driver_Return {
    DRIVER_RETURN_OK,              // Driver loaded successfully
    DRIVER_RETURN_ERROR,           // Driver failed to load
    DRIVER_RETURN_NOT_SUPPORTED,   // Driver does not support the device
    DRIVER_RETURN_NOT_PRESENT,     // Driver is not present
    DRIVER_RETURN_NOT_READY,       // Driver is not ready
    DRIVER_RETURN_UNKNOWN          // Driver status is unknown
} driver_status;

// Note: filesystems will be implemented in the disk I/O code, as they are not devices
typedef enum Device_Type{
    DEVICE_TYPE_BUS,                // Bus (e.g. PCI, USB, AHCI, chipset)
    DEVICE_TYPE_BLOCK,              // Block device (e.g. hard drive)
    DEVICE_TYPE_INPUT,              // Human input device (e.g. keyboard)
    DEVICE_TYPE_NETWORK,            // Network device (e.g. Ethernet)
    DEVICE_TYPE_GPU,                // Graphics device (e.g. VGA)
    DEVICE_TYPE_AUDIO,              // Audio device (e.g. sound card)
    DEVICE_TYPE_CONSOLE,            // TTY or VGA
    DEVICE_TYPE_LEGACY,             // Legacy device (e.g. ISA, non-USB Serial, Parallel)
    DEVICE_TYPE_MISC,               // Miscellaneous/other device
} device_type_t;

typedef struct Driver {
    // Identification for the driver
    char* name;                     // Name of the driver
    bool loaded;                    // Whether the driver is loaded and running
    device_type_t type;             // Type of device the driver supports
    uint32_t vendorId;              // Vendor ID of the driver
    uint32_t deviceId;              // Device ID of the driver
    pcb_t* pcb;                     // Process control block for the driver

    // More driver stuff...
} driver_t;

typedef enum Device_Status {
    DEVICE_STATUS_OK,              // Device is OK and ready for next command
    DEVICE_STATUS_BUSY,            // Device is busy
    DEVICE_STATUS_TIMEOUT,         // Device timed out
    DEVICE_STATUS_BUFFER_FULL,     // Device buffer is full
    DEVICE_STATUS_ERROR,           // Device has an error
    DEVICE_STATUS_NOT_PRESENT,     // Device is not present
    DEVICE_STATUS_NOT_SUPPORTED,   // Device is not supported
    DEVICE_STATUS_NOT_READY,       // Device is not ready
    DEVICE_STATUS_INVALID_DRIVER,  // Device has an invalid driver
    DEVICE_STATUS_UNKNOWN          // Device status is unknown
} device_status_t;

typedef enum Device_Command {
    DEVICE_COMMAND_READ,            // Read data from the device
    DEVICE_COMMAND_WRITE,           // Write data to the device
    DEVICE_COMMAND_CONTROL,         // Control command (e.g. enable/disable device)
    DEVICE_COMMAND_STATUS,          // Status command (e.g. get device status)
    DEVICE_COMMAND_RESET,           // Reset command (e.g. reset device)
    DEVICE_COMMAND_INIT,            // Initialize command (e.g. initialize device)
    DEVICE_COMMAND_CLEANUP,         // Cleanup command (e.g. cleanup device)
} device_command_t;

typedef struct Device {
    // Device-specific information
    device_type_t type;             // Type of device
    device_status_t status;         // Status of the device
    void* deviceData;               // Device-specific data (pointer to struct, is a void* to allow for any type of data)
    mutex_t lock;                   // Mutex for the device

    // Device operations (set by drivers)
    driver_status (*Command)(struct Device* self, device_command_t command, void* data, size_t bufferLen); // Command function (e.g. read, write, control, status, reset, init, cleanup)

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
    device_t* rootBus;              // System root bus (represents the motherboard)
    size_t numDevices;              // Number of devices
} device_registry_t;

extern device_registry_t* deviceRoot;

// Functions for device and driver management
void InitializeDeviceRegistry();

#endif // DEVICES_H