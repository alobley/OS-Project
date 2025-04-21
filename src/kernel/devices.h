#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <keyboard.h>
#include <stddef.h>
#include <vfs.h>

typedef struct Device device_t;

typedef unsigned long long lba_t;         // Logical Block Addressing

typedef unsigned short modid_t;           // Module ID for kernel modules

// Result of a driver operation
typedef int dresult_t;
#define DRIVER_INVALID_ARGUMENT -3
#define DRIVER_ACCESS_DENIED -2
#define DRIVER_FAILURE -1
#define DRIVER_SUCCESS 0
#define DRIVER_NOT_SUPPORTED 1
#define DRIVER_REGISTRY_FULL 2

typedef dresult_t (*init_handle_t)(void);
typedef dresult_t (*deinit_handle_t)(void);
typedef dresult_t (*read_handle_t)(device_id_t device, void* buf, size_t len, size_t offset);               // Offset is within the given buffer
typedef dresult_t (*write_handle_t)(device_id_t device, void* buf, size_t len, size_t offset);              // Offset is the offset of the device
typedef dresult_t (*ioctl_handle_t)(int cmd, void* arg, device_id_t device);
typedef dresult_t (*irq_handle_t)(int num);
typedef dresult_t (*poll_t)(void* file, short* revents);                                                    // Reminder to me to implement a struct file or something
typedef dresult_t (*probe_t)(device_id_t device, unsigned int deviceClass, unsigned int deviceType);

// Per-device operations
typedef struct Device_Ops {
    read_handle_t read;             // Read function
    write_handle_t write;           // Write function
    ioctl_handle_t ioctl;           // IOCTL function
    irq_handle_t irqHandle;         // Interrupt handling function (if IRQs are installed)
    poll_t poll;                    // Poll the device
} device_ops_t;

enum Device_Class {
    DEVICE_CLASS_BLOCK = (1 << 0),             // Block devices, such as hard disks
    DEVICE_CLASS_CHAR = (1 << 1),              // Character devices, such as serial ports or keyboards
    DEVICE_CLASS_NET = (1 << 2),               // Network devices
    DEVICE_CLASS_INPUT = (1 << 3),             // Input devices
    DEVICE_CLASS_GRAPHICS = (1 << 4),          // Graphical output devices, like framebuffers
    DEVICE_CLASS_SOUND = (1 << 5),             // Sound output devices
    DEVICE_CLASS_SYSTEM = (1 << 6),            // System devices, like CMOS or ACPI
    DEVICE_CLASS_VIRTUAL = (1 << 7),           // Virtual devices, like TTYs or loopback devices
    DEVICE_CLASS_BUS = (1 << 8)                // Bus controllers, such as PCI or USB
    // More...
};

enum Device_Type {
    DEVICE_TYPE_KEYBOARD = (1 << 0),
    DEVICE_TYPE_MOUSE = (1 << 1),
    DEVICE_TYPE_FRAMEBUFFER = (1 << 2),
    DEVICE_TYPE_GPU = (1 << 3),
    DEVICE_TYPE_SPEAKER = (1 << 4),
    DEVICE_TYPE_SERIAL = (1 << 5),
    DEVICE_TYPE_ETHERNET = (1 << 6),
    DEVICE_TYPE_WIFI = (1 << 7),
    DEVICE_TYPE_STORAGE = (1 << 8),
    DEVICE_TYPE_PARTITION = (1 << 9),
    DEVICE_TYPE_RTC = (1 << 10),
    DEVICE_TYPE_TIMER = (1 << 11),
    DEVICE_TYPE_FILESYSTEM = (1 << 12),
    DEVICE_TYPE_TTY = (1 << 13),
    // More...
};

typedef struct Driver {
    const char* name;               // Name of the driver
    unsigned int class;             // Bitfield containing the class of device this driver supports
    unsigned int type;              // Bitfield containing the type of device this driver supports
    init_handle_t init;             // Initialization function
    deinit_handle_t deinit;         // De-initialization function
    probe_t probe;                  // Lets the kernel probe this driver to support a specific device
    modid_t id;                     // This module's ID
    bool busy;                      // Whether this driver is busy or not (set by the driver to prevent the kernel from calling its functions while it's working)
    bool inKernel;                  // Whether this driver is part of the kernel binary or not
    size_t driverSize;              // Size of the driver in memory  
    virtaddr_t startAddr;           // The start address of the driver in memory
    virtaddr_t endAddr;             // The end address of the driver in memory
} driver_t;

// Device driver specific to filesystems. No other drivers will have special treatment. Read/write functions still work with file reading/writing.
typedef struct FS_Driver {
    driver_t driver;
    dresult_t (*sync)(device_id_t device);                      // Sync the filesystem to the disk
    dresult_t (*fsync)(char* path);                             // Sync one file to the disk
    dresult_t (*delete)(char* path);                            // Delete a file from the filesystem
    dresult_t (*mount)(device_id_t device, char* path);         // The driver must mount the filesystem at the given path
    dresult_t (*unmount)(device_id_t device, char* path);       // Unmount the directiories from the given filesystem device
} fs_driver_t;

typedef struct Device {
    char* name;                     // Name of the device
    unsigned int class;             // Bitfield for the class of device this is
    unsigned int type;              // Bitfield for the type of device this is
    void* driverData;               // Pointer to the driver-specific data (if any)
    device_ops_t ops;               // Per-device operations
    device_id_t id;                 // The device ID of this device
    device_id_t parent;             // The device ID of a parent device, if any
    driver_t* driver;               // The driver responsible for this device
    mutex_t lock;                   // Mutex for exclusive access
    vfs_node_t* node;               // The VFS node of this driver
    pid_t caller;                   // The process currently using this device
} device_t;

typedef struct Framebuffer {
    void* address;                  // The address of the framebuffer
    size_t size;                    // Size in bytes
    uint16_t width;                 // Width in pixels (or characters)
    uint16_t height;                // Height in pixels (or characters)
    uint16_t pitch;                 // Bytes per line
    uint8_t bpp;                    // Bits per pixel
    bool text;                      // True if the framebuffer is in text mode
    uint8_t red_mask_size;          // Number of bits used for red
    uint8_t green_mask_size;        // Number of bits used for green
    uint8_t blue_mask_size;         // Number of bits used for blue
    uint8_t reserved_mask_size;     // Either bits used for alpha or reserved
} framebuffer_t;

// VFS mount point
typedef struct Mount_Point {
    device_t* blockDevice;                                      // The block device this filesystem is contained on
    device_t* fsDevice;                                         // The filesystem device used by this node
    fs_driver_t* fsDriver;                                      // The filesystem driver for this mount point
    vfs_node_t* mountRoot;                                      // The VFS node where the filesystem is mounted
} mountpoint_t;

// The system device registry
typedef struct Device_Registry {
    uint32_t* bitmap;
    device_t** devices;
    uint16_t numDevices;                                        // Nobody needs more than 65,536 devices
    size_t devArrSize;
} device_registry_t;

// The system driver registry (primarily for finding new drivers if one was released)
typedef struct Driver_Registry {
    uint32_t* bitmap;                                           // For keeping track of free array indeces
    driver_t** drivers;
    uint16_t numDrivers;
    size_t drvArrSize;
} driver_registry_t;
#define REGISTRY_BITMAP_SIZE (65536 / 32)
#define DEFAULT_REGISTRY_ARRAY_SIZE (128 * 4)                   // Start off with 128 devices and go from there

#define KERNEL_API_VERSION 1

// The kernel API for drivers
struct kernelapi {
    unsigned int version;                                                           // The version of the kernel API
    void* (*halloc)(size_t size);                                                   // Allocate memory from the kernel heap
    void (*hfree)(void* ptr);                                                       // Free memory from the kernel heap  
    void* (*rehalloc)(void* ptr, size_t size);                                      // Reallocate memory from the kernel heap
    page_result_t (*mmap)(virtaddr_t virt, physaddr_t phys, unsigned int flags);    // This "mmap" is actually physpalloc
    vfs_node_t* (*VfsFindNode)(char* path);                                         // Find a VFS node by its path
    vfs_node_t* (*VfsMakeNode)(char* name, unsigned int flags, size_t size, unsigned int permissions, uid_t owner, void* data); // Create a VFS node (does not add it to the VFS tree)
    int (*VfsRemoveNode)(vfs_node_t* node);                                         // Remove a VFS node
    int (*VfsAddChild)(vfs_node_t* parent, vfs_node_t* child);                      // Add a child to a VFS node
    device_t* (*GetDevice)(device_id_t device);                                     // Get a device by its ID   

    // IN/OUT and string functions should be implemented in the driver (the kernel's versions are static inlines)
    void (*printk)(const char* str, ...);
};

// Bitmap manipulation
index_t FindIndex(uint32_t* bitmap, size_t bitmapSize);
device_id_t AllocateDeviceID();
void ReleaseDeviceID(device_id_t id);

// Device registry management
int CreateDeviceRegistry();
void DestroyDeviceRegistry();
dresult_t RegisterDevice(device_t* userDevice, char* path, int permissions);
int UnregisterDevice(device_t* device);

// Driver registry management
int CreateDriverRegistry();
void DestroyDriverRegistry();
dresult_t RegisterDriver(driver_t* userDriver, bool inKernel);
int UnregisterDriver(driver_t* driver);


// Device/driver lookup

/// @brief Attempt to find a driver for a given device. Drivers and devices should have the exact same class.
/// @brief Filesystem drivers and block devices are an exception.
/// @param device The device to attempt to find a driver for
/// @return Success or error code
dresult_t FindDriver(device_t* device);

fs_driver_t* FindFsDriver(device_t* device);

/// @brief Get a device by its unique device ID
/// @param device The device ID to get
/// @return Pointer to the device, or NULL if none
device_t* GetDeviceByID(device_id_t device);

size_t GetNumDevices();

int LoadModule(char* path);
int UnloadModule(modid_t driver);

#endif // DEVICES_H