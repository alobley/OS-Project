#ifndef DRIVERS_H
#define DRIVERS_H

//#ifndef DEVICES_H

// An extension to system.h specifically for device drivers

#include <system.h>

#define DRIVER_API_VERSION 0            // The version of the driver API (specifically this file)

struct kmod;
struct mountpoint;
struct vfs_node;
struct device;

// Result of a driver operation
typedef int dresult_t;
#define DRIVER_INVALID_ARGUMENT -3
#define DRIVER_ACCESS_DENIED -2
#define DRIVER_FAILURE -1
#define DRIVER_SUCCESS 0
#define DRIVER_NOT_SUPPORTED 1
#define DRIVER_REGISTRY_FULL 2

typedef unsigned short modid_t;

// All of these functions should end with a SYS_YIELD, as they will be entered through an iret. Otherwise, the driver will crash.
typedef dresult_t (*init_handle_t)(void);
typedef dresult_t (*deinit_handle_t)(void);
typedef dresult_t (*read_handle_t)(device_id_t device, void* buf, size_t len, size_t offset);               // Offset is within the given buffer
typedef dresult_t (*write_handle_t)(device_id_t device, void* buf, size_t len, size_t offset);              // Offset is the offset of the device (drivers are free to ignore)
typedef dresult_t (*ioctl_handle_t)(int cmd, void* arg, device_id_t device);
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
        struct vfs_node* symlink;       // Pointer to the symlink (if symlink)
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
    _Bool read;                         // This file or directory has been read from the disk (if mounted). Drivers can set this flag if they want, but the kernel will likely overwrite it.
    struct mountpoint* mp;              // The mount point of this node (if any)
    char _reserved1[28];                // Reserved for use by the kernel
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

struct kmod {
    const char* name;               // Name of the module
    unsigned int class;             // Bitfield containing the class of device this module supports
    unsigned int type;              // Bitfield containing the type of device this module supports
    size_t driver_size;             // Size of the module in memory
    init_handle_t init;             // Initialization function
    deinit_handle_t deinit;         // De-initialization function
    probe_t probe;                  // Lets the kernel probe this module to support a specific device
    modid_t id;                     // This module's ID
    _Bool busy;                     // Whether this module is busy or not (set by the module to prevent the kernel from calling its functions while it's working)
    _Bool _reserved1;               // Reserved for use by the kernel
    char _reserved2[12];            // Reserved for use by the kernel
};

// Special struct specific to filesystem drivers (since they interact with the kernel)
struct fs_driver {
    struct kmod driver;
    driver_t driver;
    dresult_t (*sync)(device_id_t device);                          // Sync the filesystem to the disk
    dresult_t (*fsync)(char* path);                                 // Sync one file to the disk (path relative to the VFS's root)
    dresult_t (*delete)(char* path);                                // Delete a file from the filesystem (path relative to the VFS's root)
    dresult_t (*mount)(device_id_t device, char* path);             // The driver must mount the filesystem at the given path
    dresult_t (*unmount)(device_id_t device);                       // Unmount the directiories from the given filesystem device
};

struct mountpoint {
    struct device* blockDevice;                                      // The block device this filesystem is contained on
    struct device* fsDevice;                                         // The filesystem device used by this node
    struct fs_driver* fsDriver;                                      // The filesystem driver for this mount point
    struct vfs_node* mountRoot;                                      // The VFS node where the filesystem is mounted
};

struct device {
    const char* name;               // Name of the device
    unsigned int class;             // Bitfield for the class of device this is
    unsigned int type;              // Bitfield for the type of device this is
    void* driverData;               // Pointer to the driver-specific data (if any)
    device_ops_t ops;               // Per-device operations
    device_id_t id;                 // The device ID of this device (set by the kernel)
    device_id_t parent;             // The device ID of the parent device (if any - set by the driver upon the kernel probing a device)
    struct kmod* driver;            // The driver responsible for this device
    __uint8_t _reserved[16];        // Reserved for use by the kernel
    struct vfs_node* node;          // The VFS node of this driver
    pid_t caller;                   // The process currently using this device
};

typedef enum PAGE_RESULT {
    // Errors (negative values)
    FAULT_FATAL              = -11,  // Page fault occurred in the kernel  or critical code and therefore is unrecoverable. The OS should halt execution.
    FAULT_NONFATAL           = -10,  // The page fault could not be handled and was likely in user mode. The OS should kill the offending process
    ADDRESS_NOT_ALIGNED      = -9,   // A given address wasn't aligned
    PAGE_NOT_PRESENT         = -8,   // A requested page did not have the present flag
    PAGE_NOT_MAPPED          = -7,   // A requested page had no mapping
    PROTECTION_VIOLATION     = -6,   // Protection flags were violated (e.g., write to read-only)
    NO_MORE_MEMORY           = -5,   // System ran out of memory
    TABLE_FULL               = -4,   // Page table is full
    NO_FRAME_FOUND           = -3,   // Could not find a frame for the page
    GENERIC_ERROR            = -1,   // Internal/unknown error

    // Success and info (zero or positive)
    PAGE_OK                  = 0,    // Successful operation
    PAGE_ALREADY_MAPPED      = 1,    // Page was already mapped
    CANNOT_SWAP              = 2,    // Page marked NOSWAP cannot be swapped
    NOTHING_TO_DO            = 3,    // No operation needed
    FAULT_HANDLED            = 4     // A page fault was handled gracefully
} page_result_t;

#define PAGE_PRESENT (1 << 0)                               // This page is present in physical memory
#define PAGE_RW (1 << 1)                                    // This page is writable and readable
#define PAGE_USER (1 << 2)                                  // This page is accessible by user programs
#define PAGE_WRITE_THROUGH (1 << 3)                         // This page writes through the cache
#define PAGE_CACHE_DISABLE (1 << 4)                         // This page ignores the cache
#define PAGE_ACCESSED (1 << 5)                              // The page was read during a virtual address translation
#define PAGE_DIRTY (1 << 6)                                 // This page has had data written to it. Set by the CPU.
#define PAGE_PAT (1 << 7)                                   // The page attribute table (PAT) is enabled for this page
#define PAGE_GLOBAL (1 << 8)                                // The page is never invalidated, even if CR3 changes
#define PAGE_NOSWAP (1 << 9)                                // Available for the OS to use. Disables swapping by the OS.
#define PAGE_NOREMAP (1 << 10)                              // Available for the OS to use. Determines whether the page's frame can be reused as regular memory.
#define PAGE_SHARED (1 << 11)                               // Available for the OS to use. This page is shared between multiple processes.

typedef unsigned int physaddr_t;
typedef unsigned int virtaddr_t;

struct kernelapi {
    unsigned int version;                                                           // The version of the kernel API
    void* (*halloc)(size_t size);                                                   // Allocate memory from the kernel heap
    void (*hfree)(void* ptr);                                                       // Free memory from the kernel heap  
    void* (*rehalloc)(void* ptr, size_t size);                                      // Reallocate memory from the kernel heap
    page_result_t (*mmap)(virtaddr_t virt, physaddr_t phys, unsigned int flags);    // This "mmap" is actually physpalloc
    struct vfs_node* (*VfsFindNode)(char* path);                                    // Find a VFS node by its path
    struct vfs_node* (*VfsMakeNode)(char* name, unsigned int flags, size_t size, unsigned int permissions, uid_t owner, void* data); // Create a VFS node (does not add it to the VFS tree)
    int (*VfsRemoveNode)(struct vfs_node* node);                                    // Remove a VFS node
    int (*VfsAddChild)(struct vfs_node* parent, struct vfs_node* child);            // Add a child to a VFS node
    struct device* (*GetDevice)(device_id_t device);                                // Get a device by its ID   

    // IN/OUT and string functions should be implemented in the driver (the kernel's versions are static inlines)
    void (*printk)(const char* str, ...);
};

//#endif

#endif  // DRIVERS_H