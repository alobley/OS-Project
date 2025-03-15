#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <keyboard.h>
#include <stddef.h>

// Values returned by driver functions
typedef signed int DRIVERSTATUS;
#define DRIVER_INITIALIZED 1
#define DRIVER_SUCCESS 0
#define DRIVER_FAILURE -1
#define DRIVER_NOT_SUPPORTED -2
#define DRIVER_OUT_OF_MEMORY -3
#define DRIVER_INVALID_MEMORY -4
#define DRIVER_INVALID_ARGUMENT -5
#define DRIVER_NOT_INITIALIZED -6
#define DRIVER_ALREADY_INITIALIZED -7

// These two are assigned by the hardware of the device
typedef unsigned short device_id_t;
typedef unsigned short vendor_id_t;

typedef unsigned short driver_id_t;    // Unique ID for the driver (assigned by the kernel)

typedef enum {
    DEVICE_STATUS_IDLE,                         // Device is ready to be used
    DEVICE_STATUS_BUSY,                         // Device is currently in use
    DEVICE_STATUS_ERROR,                        // Device is in an error state
    DEVICE_STATUS_UNKNOWN,                      // Typically means no driver
} DEVICE_STATUS;

typedef enum {
    DEVICE_TYPE_ROOTDEV,                        // Motherboards and daughterboards (essentially the root of the device tree) (needed?)
    DEVICE_TYPE_BLOCK,                          // Block device (i.e. hard drive, SSD)
    DEVICE_TYPE_CHAR,                           // Character device (i.e. keyboard, mouse, serial port)
    DEVICE_TYPE_NETWORK,                        // Network device (i.e. Ethernet, Wi-Fi)
    DEVICE_TYPE_INPUT,                          // Input device (i.e. keyboard, mouse)
    DEVICE_TYPE_DISPLAY,                        // Display device (i.e. GPU, framebuffer)
    DEVICE_TYPE_SOUND,                          // Sound device (i.e. sound card)
    DEVICE_TYPE_USB,                            // USB device
    DEVICE_TYPE_PCI,                            // PCI device
    DEVICE_TYPE_PCIe,                           // PCIe device
    DEVICE_TYPE_VIRTUAL,                        // Virtual device (i.e. a ramdisk)
    DEVICE_TYPE_OTHER,                          // Other device type (not defined by the kernel and may not be supported)
    DEVICE_TYPE_UNKNOWN,                        // Unknown device type
} DEVICE_TYPE;

typedef enum {
    IOCTL_GET_STATUS,                           // Get the status of the device
    IOCTL_RESET_DEVICE,                         // Reset the device
    IOCTL_FLUSH_CACHE,                          // Flush the device's cache
    // Others...
} IOCTL_CMD;

typedef struct Device device_t;

typedef struct {
    const char* name;                           // Name of the driver
    const char* description;                    // Description of the driver
    driver_id_t id;                             // Unique ID for the driver
    uint32_t version;                           // Version of the driver
    pcb_t* driverProcess;                       // Pointer to the PCB of the driver for this (switch to it when using the device)
    DRIVERSTATUS (*init)(void);                 // Initialize the driver
    DRIVERSTATUS (*deinit)(void);               // Shutdown and clean up the driver
    DRIVERSTATUS (*probe)(device_t* device);    // Probe the device to see if the driver supports it
} driver_t;

typedef struct {
    bool readonly : 1;                          // Whether the device is read-only
    bool writeonly : 1;                         // Whether the device is write-only
    bool removable : 1;                         // Whether the device is removable (i.e. USB drive)
    bool hotpluggable : 1;                      // Whether the device can be hot-plugged (i.e. USB)
    bool virtual : 1;                           // Whether the device is virtual (i.e. a software device, like a ramdisk)
    bool initialized : 1;                       // Whether the device has been initialized
    bool shared : 1;                            // Whether the device can be shared between multiple processes simultaneously (i.e. a network device or GPU)
    bool exclusive : 1;                         // Whether the device can only be used by one process at a time (i.e. a serial port)
} device_flags_t;

typedef struct Device {
    device_id_t id;                             // Device ID
    vendor_id_t vendorId;                       // Vendor ID
    DEVICE_STATUS status;                       // Status of the device
    device_flags_t flags;                       // Flags for the device (i.e. read-only, write-only, removable)
    const char* name;                           // Name of the device
    const char* description;                    // Description of the device

    const char* devName;                        // String that will be presented in /dev (i.e. kb0, sda, etc.)

    driver_t* driver;                           // Pointer to the driver for this device (determines whether this device can be used. NULL if no driver loaded)
    void* deviceInfo;                           // Device-specific information (likely a pointer to a struct containing device commands and other info)
    DEVICE_TYPE type;                           // Type of device

    // Note: OS MUST switch to the driver's PCB when using the device
    DRIVERSTATUS (*read)(struct Device* this, void* buffer, size_t size);       // Read from the device
    DRIVERSTATUS (*write)(struct Device* this, void* buffer, size_t size);      // Write to the device
    DRIVERSTATUS (*ioctl)(struct Device* this, int request, void* argp);        // Control the device (i.e. set options, get status)

    char last_error[64];                        // Last error message from the device in human-readable format

    mutex_t lock;                               // Mutex for the device (for thread safety)

    struct Device* parent;                      // Pointer to the parent device (if any)
    struct Device* next;                        // Pointer to the next sibling device (if any)
    struct Device* firstChild;                  // Pointer to the first child device (if any) (do I need this?)
} device_t;

/*
 * Block devices
 * These will go device->blkdev->partition->filesystem
 * Note: reading/writing sectors are done by the device's read/write functions. No need for redundant functions.
*/
typedef enum {
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
    FS_ISO9660,
    FS_EXT2,
    FS_EXT3,                                    // Does anyone actually use EXT3? I never even heard of it until recently...
    FS_EXT4,
    FS_NTFS,
    FS_OTHER,                                   // Filesystem not defined by kernel (make sure to design the driver interface to support this)
} FS_TYPE;

typedef unsigned long long lba;                 // Logical Block Addressing (LBA) offset on a disk
typedef unsigned long long uuid_t;              // Universally Unique Identifier (UUID) for a filesystem

// Resolve circular dependencies
typedef struct Partition partition_t;
typedef struct Filesystem filesystem_t;
typedef struct Block_Device blkdev_t;

// Block device structure (will be defined as, for example, /dev/sda)
typedef struct Block_Device {
    device_t* device;                           // Pointer to the device struct
    partition_t* firstPartition;                // Pointer to the first partition (if any)

    lba size;                                   // Size of the block device in sectors
    size_t sectorSize;                          // Size of each sector in bytes
    unsigned long long totalSize;               // Total size of the block device in bytes
    unsigned int numPartitions;                 // Number of partitions on the block device

    bool gpt;                                   // Whether the block device uses the GUID partition table
    bool mbr;                                   // Whether the block device uses the Master Boot Record
    bool lba48;                                 // Whether the block device supports LBA48 addressing
    bool lba28;                                 // Whether the block device supports LBA28 addressing
    bool chs;                                   // Whether the block device supports CHS addressing

    struct Block_Device* next;                  // Pointer to the next block device (if any - for ennumeration in VFS)
} blkdev_t;

// Partition structure (will not be located in /dev, is instead represented by a filesystem)
typedef struct Partition {
    lba start;                                  // Starting LBA of the partition
    lba end;                                    // Ending LBA of the partition
    lba size;                                   // Size of the partition in sectors
    const char* name;                           // Name of the partition
    const char* type;                           // Type of the partition in human-readable format
    device_t* device;                           // Pointer to the block device
    blkdev_t* blkdev;                           // Pointer to the block device info
    filesystem_t* filesystem;                   // Pointer to the filesystem (if any)
    struct Partition* next;                     // Pointer to the next partition (if any)
    struct Partition* previous;                 // Pointer to the previous partition (if any)
} partition_t;

// Filesystem structure (will be defined as, for example, /dev/sda1)
typedef struct Filesystem {
    FS_TYPE fs;                                 // Filesystem type
    lba start;                                  // Starting LBA of the filesystem
    lba size;                                   // Size of the filesystem in sectors
    partition_t* partition;                     // Pointer to the partition this filesystem is on
    blkdev_t* blkdev;                           // Pointer to the block device this filesystem is on
    driver_t* driver;                           // Pointer to the driver for this filesystem
    uuid_t uuid;                                // UUID of the filesystem
    void* fsInfo;                               // Filesystem-specific information (i.e. superblock)

    char* name;                                 // Name of the filesystem (i.e. ext4, ntfs, etc.)
    char* mountPoint;                           // Mount point of the filesystem (i.e. /, /home, /usr, /mnt, etc.)

    char* devName;                              // String that will be presented in /dev (i.e. sda1, sda2, etc.)

    DRIVERSTATUS (*mount)(filesystem_t* this, char* mountpoint);                           // Mount the filesystem
    DRIVERSTATUS (*unmount)(filesystem_t* this);                                           // Unmount the filesystem

    // File operations (should I have VFS files be returned?)
    DRIVERSTATUS (*read)(filesystem_t* this, void* buffer, size_t size, char* path);       // Read a file or directory from the filesystem (requires absolute paths, NULL path is root directory)
    DRIVERSTATUS (*write)(filesystem_t* this, void* buffer, size_t size, char* path);      // Write a file or directory to the filesystem (requires absolute paths, NULL path is root directory)
} filesystem_t;

/*
 * Character devices
 * These go device->chardev
 * It can also be chardev->serial->tty if needed
 * For framebuffer-based output on TTYs, the kernel will aquire the framebuffer and write to it based on the active TTY
*/

// Keyboard device structure
typedef struct Keyboard {
    // Luckily keyboards can be extremely simple
    void (*AddCallback)(KeyboardCallback callback);     // Install a keyboard handler
    void (*RemoveCallback)(KeyboardCallback callback);  // Remove a keyboard handler
} keyboard_t;

// Mouse device structure
typedef struct Mouse {
    // Mouse support is not high on my priority list and it is largely unimplemented
} mouse_t;

// TTY "device" structure
typedef struct tty {
    short ttyNum;                       // TTY number (for multiple TTYs)
    bool active;                        // Whether the TTY is active

    char* buffer;                       // Buffer for the tty to hold its characters (including scrollback - handling up to kernel code elsewhere)
    size_t size;                        // Size of the buffer

    unsigned short cursorX;             // Cursor X position
    unsigned short cursorY;             // Cursor Y position
    unsigned short width;               // Width of the TTY
    unsigned short height;              // Height of the TTY

    // Convenience functions (can still write to the buffer though)
    DRIVERSTATUS (*write)(struct tty* this, const char* text, size_t size);      // Write some text to the TTY
    DRIVERSTATUS (*read)(struct tty* this, char* buffer, size_t size);           // Read some text from the TTY
    DRIVERSTATUS (*clear)(struct tty* this);                                     // Clear the TTY
    //DRIVERSTATUS (*scroll)(struct tty* this, bool up);                           // Scroll the TTY

    char* devName;                      // String that will be presented in /dev (i.e. tty0, tty1, etc.)

    mutex_t lock;                       // Mutex for the TTY (for thread safety)

    struct tty* next;                   // Pointer to the next TTY (if any)
} tty_t;

// Serial device structure
typedef struct Serial {
    // Serial support is not high on my priority list either (I have no idea how serial works yet lol)
} serial_t;

/*
 * Graphics devices (like framebuffers)
 * These go device->gpu or device->framebuffer
 * It can also be device->pci->gpu or device->pcie->gpu if needed
*/

#define GRAPHICS_TYPE_FRAMEBUFFER 0
#define GRAPHICS_TYPE_GPU 1

// Framebuffer graphics device structure
typedef struct {
    int type;                                   // Type of graphics device (framebuffer or GPU)
    void* framebuffer;                          // Pointer to the framebuffer
    size_t width;                               // Width of the framebuffer in pixels
    size_t height;                              // Height of the framebuffer in pixels
    size_t bpp;                                 // Bits per pixel
    size_t size;                                // Size of the framebuffer in bytes
} framebuffer_t;

// Hardware-accelerated GPU device structure
typedef struct {
    int type;                                   // Type of graphics device (framebuffer or GPU)
    // GPU stuff...
} gpu_t;

// Will need more structures for the other device types...


/* 
 * Device Management and Registration
 * Will be a tree of devices
 * Example: root hub (if it exists)->block device->partition->filesystem
 *                                 |->block device->partition->filesystem
 *          |->char device->keyboard
 *          |->char device->tty
*/

typedef struct Device_Registry {
    device_t* firstDevice;                      // Pointer to the first device in the registry
    device_t* lastDevice;                       // Pointer to the last device in the registry
    size_t numDevices;                          // Number of devices in the registry
} device_registry_t;

extern device_registry_t* deviceRegistry;

// Device registration
int RegisterDevice(device_t* device);                                   // Add a device to the device registry
int UnregisterDevice(device_t* device);                                 // Remove a device from the device registry

// Driver registration
int RegisterDriver(driver_t* driver, device_t* device);                 // Replaces and does not save the driver for the device if one exists (kill its PCB?)
int UnregisterDriver(driver_t* driver, device_t* device);               // Does not restore any previous drivers, they must be loaded again

// Registry management
int CreateDeviceRegistry(void);                                         // Create the device registry
void DestroyDeviceRegistry(void);                                       // Destroy the device registry (say for system cleanup before shutdown)

// Device lookup
device_t* GetDeviceByID(device_id_t id);                                // Get a device by its ID
device_t* GetFirstDeviceByType(DEVICE_TYPE type);                       // Get the first device of a certain type
device_t* GetNextDeviceByType(device_t* previous, DEVICE_TYPE type);    // Get the next device of a certain type
device_t* GetDeviceFromVfs(char* path);                                 // Get a device from the VFS by its name (i.e. /dev/sda)

driver_t* CreateDriver(const char* name, const char* description, driver_id_t id, uint32_t version, DRIVERSTATUS (*init)(void), DRIVERSTATUS (*deinit)(void), DRIVERSTATUS (*probe)(device_t* device));  // Create a new driver

#endif // DEVICES_H