#ifndef DRIVERS_H
#define DRIVERS_H

// An extension to system.h specifically for device drivers

#include <system.h>

#define DRIVER_API_VERSION 0

// Result of a driver operation
typedef int dresult_t;
#define DRIVER_ACCESS_DENIED -2
#define DRIVER_FAILURE -1
#define DRIVER_SUCCESS 0
#define DRIVER_NOT_SUPPORTED 1
#define DRIVER_INVALID_ARGUMENT 2

struct device {
    const char* name;               // Name of the device
    unsigned int class;             // Bitfield for the class of device this is
    unsigned int type;              // Bitfield for the type of device this is
    void* driver_data;              // Allows the driver to attach its own context
    __uint8_t _reserved1[4];        // Reserved for use by the kernel
    device_ops_t ops;               // Per-device operations
    device_id_t id;                 // The device ID of this device (set by the kernel)
    device_id_t parent;             // The device ID of the parent device (if any - set by the driver upon the kernel probing a device)
    __uint8_t _reserved2[28];       // Reserved for use by the kernel (allows the easy use of memcpy)
};

// All of these functions should end with a SYS_YIELD, as they will be entered through an iret. Otherwise, the driver will crash.
typedef dresult_t (*init_handle_t)(void);
typedef dresult_t (*deinit_handle_t)(void);
typedef dresult_t (*read_handle_t)(void* buf, size_t len);
typedef dresult_t (*write_handle_t)(void* buf, size_t len);
typedef dresult_t (*ioctl_handle_t)(int cmd, void* arg);
typedef dresult_t (*irq_handle_t)(int num);
typedef dresult_t (*probe_t)(device_id_t device, unsigned int deviceClass, unsigned int deviceType);

// Per-device operations
typedef struct Device_Ops {
    read_handle_t read;             // Read function
    write_handle_t write;           // Write function
    ioctl_handle_t ioctl;           // IOCTL function
    irq_handle_t irqHandle;         // Interrupt handling function (if IRQs are installed)
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
    // More...
};

struct vfs_node {
    char* name;                         // Name of the file or directory
    unsigned int flags;                 // Flags for this node
    size_t size;                        // If file, the size in bytes (if loaded), if directory, the number of children it has
    union {
        struct vfs_node* firstChild;    // Pointer to the first child (if directory)
        // No need for device pointer, the kernel handles /dev.
        void* data;                     // Pointer to the file data (if file)
    };
    struct vfs_node* parent;            // Pointer to the parent node (if it exists)
    struct vfs_node* next;              // Pointer to the next sibling node (if it exists)
    unsigned int permissions;           // Permissions for the file or directory    
    uid_t owner;                        // Owner of the file or directory
    datetime_t created;                 // Date and time the file or directory was created
    datetime_t modified;                // Date and time the file or directory was last modified
    datetime_t accessed;                // Date and time the file or directory was last accessed
    device_id_t device;                 // The device ID of the block device this node's filesystem resides on (drivers are HIGHLY encouraged to use this for bookkeeping)
    _Bool read;                         // This file or directory has been read from the disk (if mounted). The kernel expects drivers to ignore this flag.
    __uint8_t _reserved[32];            // Reserved for use by the kernel - expected to be zero. Data here will be ignored or overwritten.
};
#define NODE_FLAG_DIRECTORY (1 << 0)            // Whether the node is a file or directory
#define NODE_FLAG_DEVICE (1 << 1)               // Whether the node is a device
#define NODE_FLAG_RO (1 << 2)                   // Whether the node is read-only
#define NODE_FLAG_WO (1 << 3)                   // Whether the node is write-only
#define NODE_FLAG_RESIZEABLE (1 << 4)           // Whether the node is resizeable
#define NODE_FLAG_TEMP (1 << 5)                 // This node is temporary and will be destroyed with the owned process
#define NODE_FLAG_IPC (1 << 6)                  // This node is being used for IPC
#define NODE_FLAG_MOUNTED (1 << 7)              // This VFS node has been mounted
#define NODE_FLAG_NOTREAD (1 << 8)              // A node has not been read into memory if it is mounted (be it file data or a directory)

struct driver {
    const char* name;                   // Name of the driver
    unsigned int class;                 // Bitfield containing the class of device this driver supports
    unsigned int type;                  // Bitfield containing the type of driver this is
    char _ignored1[4];                  // Ignored. Used by the kernel
    init_handle_t init;                 // Initialization function
    deinit_handle_t deinit;             // De-initialization function
    probe_t probe;                      // Lets the kernel probe this driver to support a specific device
    _Bool _ignored2;                    // Ignored. Used by the kernel.
    char _ignored3[4];                  // Ignored. Used by the kernel.
};

struct fs_driver {
    struct driver driver;
    struct vfs_node* (*readdir)(device_id_t device);
    struct vfs_node* (*mount)(device_id_t device);
    dresult_t (*unmount)(struct vfs_node* node);
};

#endif  // DRIVERS_H