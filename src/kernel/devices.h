#ifndef DEVICES_H
#define DEVICES_H

#include <multitasking.h>
#include <keyboard.h>
#include <stddef.h>
#include <system.h>                             // Since this header is for drivers, this include can stay
//#include <vfs.h>

#define DEVICE_MAGIC 0xDEDEDEDE

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
#define DRIVER_INVALID_STATE -8
#define DRIVER_PERMISSION_DENIED -9

typedef unsigned short driver_id_t;    // Unique ID for the driver (assigned by the kernel)

typedef enum {
    DEVICE_STATUS_IDLE,                         // Device is ready to be used
    DEVICE_STATUS_BUSY,                         // Device is currently in use
    DEVICE_STATUS_ERROR,                        // Device is in an error state
    DEVICE_STATUS_UNKNOWN,                      // Typically means no driver
} DEVICE_STATUS;

typedef enum {
    IOCTL_GET_STATUS,                           // Get the status of the device
    IOCTL_RESET_DEVICE,                         // Reset the device
    IOCTL_FLUSH_CACHE,                          // Flush the device's cache
    // Others...
} IOCTL_CMD;

typedef struct Device device_t;

typedef DRIVERSTATUS (*readhandle_t)(struct Device* this, void* buffer, size_t size);
typedef DRIVERSTATUS (*writehandle_t)(struct Device* this, void* buffer, size_t size);
typedef DRIVERSTATUS (*ioctlhandle_t)(struct Device* this, IOCTL_CMD request, void* argp);

typedef DRIVERSTATUS (*driver_init_t)(void);
typedef DRIVERSTATUS (*driver_deinit_t)(void);
typedef DRIVERSTATUS (*driver_probe_t)(device_t* device);

typedef struct Driver {
    const char* name;                           // Name of the driver
    const char* description;                    // Description of the driver
    driver_id_t id;                             // Unique ID for the driver
    uint32_t version;                           // Version of the driver
    pcb_t* driverProcess;                       // Pointer to the PCB of the driver for this (switch to it when using the device)

    DEVICE_TYPE supportedType;                  // Type of device this driver supports
    driver_init_t init;                         // Initialize the driver
    driver_deinit_t deinit;                     // Shutdown and clean up the driver
    driver_probe_t probe;                       // Probe the device to see if the driver supports it
    DEVICE_TYPE type;                           // Type of device this driver supports
    struct Driver* next;                        // Pointer to the next driver (for driver registry)
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
    readhandle_t read;       // Read from the device
    writehandle_t write;      // Write to the device
    ioctlhandle_t ioctl;        // Control the device (i.e. set options, get status)

    char last_error[64];                        // Last error message from the device in human-readable format

    mutex_t lock;                               // Mutex for the device (for thread safety)

    struct Device* parent;                      // Pointer to the parent device (if any)
    struct Device* next;                        // Pointer to the next sibling device (if any)
    struct Device* firstChild;                  // Pointer to the first child device (if any) (do I need this?)

    user_device_t* userDevice;                  // Pointer to the userland device struct
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

// Universally Unique Identifier (UUID) for a filesystem or GPT partition
typedef struct PACKED UUID {
    uint8_t data[16];                           // 128-bit identifier
} PACKED uuid_t;

// Resolve circular dependencies
typedef struct Partition partition_t;
typedef struct Filesystem filesystem_t;
typedef struct Block_Device blkdev_t;
typedef struct VFS_mount mountpoint_t;

// Block device structure (will be defined as, for example, /dev/sda)
typedef struct Block_Device {
    user_device_t* device;                      // Pointer to the device struct
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
    bool slave;                                 // Whether the block device is a slave device (if applicable)

    uint16_t basePort;                          // Base port of the device (if applicable)

    struct Block_Device* next;                  // Pointer to the next block device (if any - for ennumeration in VFS)
} blkdev_t;

// Partition structure (will not be located in /dev, is instead represented by a filesystem)
typedef struct Partition {
    lba start;                                  // Starting LBA of the partition
    lba end;                                    // Ending LBA of the partition
    lba size;                                   // Size of the partition in sectors
    //const char* name;                         // Name of the partition
    const char* type;                           // Type of the partition in human-readable format
    device_t* device;                           // Pointer to the block device
    blkdev_t* blkdev;                           // Pointer to the block device info
    uint8_t fsID;                               // MBR filesystem ID
    uuid_t uuid;                                // UUID of the partition (if GPT, otherwise 0)
    //filesystem_t* filesystem;                 // Pointer to the filesystem (if any)
    struct Partition* next;                     // Pointer to the next partition (if any)
    struct Partition* previous;                 // Pointer to the previous partition (if any)
} partition_t;

// Filesystem structure (will be defined as, for example, /dev/sda1)
// This will have to be private and based on partitions and mountpoints
typedef struct Filesystem {
    FS_TYPE fs;                                 // Filesystem type
    partition_t* partition;                     // Pointer to the partition this filesystem is on
    
    void* fsInfo;                               // Filesystem-specific information (i.e. superblock)

    const char* name;                           // Name of the filesystem (i.e. ext4, ntfs, etc.)
    char* volumeName;                           // Name of the volume
    mountpoint_t* mountPoint;                   // Mount point of the filesystem (i.e. /, /home, /usr, /mnt, etc.)

    char* devName;                              // String that will be presented in /dev (i.e. sda1, sda2, etc.)

    device_t* device;                           // Pointer to the device struct

    // Mount/unmount should check the device for locks
    DRIVERSTATUS (*mount)(device_t* this, char* mountpoint);                           // Mount the filesystem
    DRIVERSTATUS (*unmount)(device_t* this);                                           // Unmount the filesystem
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

    vfs_node_t* node;                   // Pointer to the VFS node for this TTY

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

    driver_t* firstDriver;                      // Pointer to the first driver in the registry
    driver_t* lastDriver;                       // Pointer to the last driver in the registry
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

device_t* CreateDevice(const char* name, const char* devName, const char* description, void* data, driver_t* driver, DEVICE_TYPE type, vendor_id_t vendorId, device_flags_t flags, readhandle_t read, writehandle_t write, ioctlhandle_t ioctl, device_t* parent);  // Create a new device
driver_t* CreateDriver(const char* name, const char* description, uint32_t version, DEVICE_TYPE type, driver_init_t init, driver_deinit_t deinit, driver_probe_t probe);  // Create a new driver

driver_t* FindDriver(device_t* device, DEVICE_TYPE type /*if not the specified device's type*/);                                  // Find a compatible driver for a device (if any)

#endif // DEVICES_H