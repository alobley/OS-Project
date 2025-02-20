#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Built-in device types
// TODO: add detection for, say a VESA driver overtaking the VGA driver.
// The VESA driver will have higher priority than VGA driver
typedef enum {
    DEVICE_TYPE_MOTHERBOARD = 0,          // Motherboard (root bus)
    DEVICE_TYPE_PCI_BUS,                  // PCI bus
    DEVICE_TYPE_USBUS,                    // USB bus
    DEVICE_TYPE_SATA_BUS,                 // SATA bus
    DEVICE_TYPE_IDE_BUS,                  // IDE bus
    DEVICE_TYPE_SERIAL_BUS,               // Legacy serial bus
    DEVICE_TYPE_PARALLEL_BUS,             // Legacy parallel bus
    DEVICE_TYPE_ISA_BUS,                  // ISA bus
    DEVICE_TYPE_PCIE_BUS,                 // PCIe bus
    DEVICE_TYPE_UNKNOWN_BUS,              // Unknown bus type
    DEVICE_TYPE_USB_DEVICE,               // USB device
    DEVICE_TYPE_SATA_DEVICE,              // SATA device
    DEVICE_TYPE_IDE_DEVICE,               // IDE device
    DEVICE_TYPE_SERIAL_DEVICE,            // Legacy serial device
    DEVICE_TYPE_PARALLEL_DEVICE,          // Legacy parallel device
    DEVICE_TYPE_PCIE_DEVICE,              // PCIe device
    DEVICE_TYPE_GPU,                      // GPU device (overtakes PCI/PCIe/other device - yes I have seen USB graphics processors)
    DEVICE_TYPE_AUDIO_DEVICE,             // Audio device
    DEVICE_TYPE_NETWORK_DEVICE,           // Network device
    DEVICE_TYPE_INPUT_DEVICE,             // Input device
    DEVICE_TYPE_STORAGE_DEVICE,           // Storage device
    DEVICE_TYPE_FS_DEVICE,                // Filesystem device
    DEVICE_TYPE_TTY_DEVICE,               // TTY device
    DEVICE_TYPE_MISC_DEVICE,              // Miscellaneous device
} device_type_t;

typedef enum {
    DRIVER_RETURN_OK,              // Command completed successfully
    DRIVER_RETURN_ERROR,           // Command failed
    DRIVER_RETURN_NOT_SUPPORTED,   // Driver does not support the operation
    DRIVER_RETURN_NOT_PRESENT,     // Driver is not present
    DRIVER_RETURN_NOT_READY,       // Driver is not ready
    DRIVER_RETURN_UNKNOWN          // Driver status is unknown
} driver_status;

typedef struct {
    // Identification for the driver
    char* name;                     // Name of the driver
    bool loaded;                    // Whether the driver is loaded and running
    device_type_t type;             // Type of device the driver supports
    uint32_t vendorId;              // Vendor ID of the driver
    uint32_t deviceId;              // Device ID of the driver
    pcb_t* pcb;                     // Process control block for the driver

    // More driver stuff...
} driver_t;

typedef enum {
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

typedef enum {
    DEVICE_COMMAND_READ,            // Read data from the device
    DEVICE_COMMAND_WRITE,           // Write data to the device
    DEVICE_COMMAND_CONTROL,         // Control command (tell the device to do something)
    DEVICE_COMMAND_STATUS,          // Status command (e.g. get device status)
    DEVICE_COMMAND_RESET,           // Reset command (e.g. reset device)
    DEVICE_COMMAND_INIT,            // Initialize command (e.g. initialize device)
    DEVICE_COMMAND_CLEANUP,         // Cleanup command (e.g. cleanup device)
} device_command_t;

typedef struct Device {
    // Device-specific information
    device_type_t type;             // Type of device
    device_status_t status;         // Status of the device
    bool isBus;                     // Whether the device is a bus
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
    uint32_t id;                    // Device ID (unique identifier)
    uint32_t numDevices;            // Number of devices
    struct Device* parent;          // Parent device (if applicable)
    struct Device* children;        // Child devices (if applicable)
    struct Device* next;            // Next sibling device in the list

    // Device driver/bus
    driver_t* driver;               // Driver for the device
} device_t;

typedef struct Device_Registry {
    device_t* rootBus;              // System root bus (represents the motherboard)
    size_t numDevices;              // Number of devices
} device_registry_t;

extern device_registry_t* deviceRoot;

// Functions for device and driver management
void InitializeDeviceRegistry();
device_t* CreateDevice(device_type_t type, device_status_t status, void* deviceData, driver_t* driver, char* name, char* description, char* manufacturer, 
                       char* model, char* serial, char* firmware, uint32_t deviceId, bool isBus, driver_status (*Command)(device_t* self, device_command_t command, void* data, 
                       size_t bufferLen));
driver_t* CreateDriver(char* name, device_type_t type, uint16_t deviceId, uint16_t vendorId, void* pcb);
int AddDevice(device_t* device, device_type_t type);

#endif // DEVICES_H