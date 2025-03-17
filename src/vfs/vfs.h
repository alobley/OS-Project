#ifndef VFS_H
#define VFS_H

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <multitasking.h>
#include <users.h>
#include <kernel.h>
#include <devices.h>

#define VFS_ROOT "/"

// File descriptor for invalid file
#define INVALID_FD -1

typedef struct VFS_Node {
    char* name;                         // Name of the file or directory
    bool isDirectory;                   // Whether the node is a file or directory
    bool readOnly;                      // Whether the file is read-only
    bool writeOnly;                     // Whether the file is write-only
    size_t size;                        // If file, the size in bytes (if loaded), if directory, the number of children it has
    union {
        struct VFS_Node* firstChild;    // Pointer to the first child (if directory)
        void* data;                     // Pointer to the file data (if file)
    };
    struct VFS_Node* parent;            // Pointer to the parent node (if it exists)
    struct VFS_Node* next;              // Pointer to the next sibling node (if it exists)
    unsigned int permissions;           // Permissions for the file or directory    
    uid owner;                          // Owner of the file or directory
    datetime_t created;                 // Date and time the file or directory was created
    datetime_t modified;                // Date and time the file or directory was last modified
    datetime_t accessed;                // Date and time the file or directory was last accessed
    mutex_t lock;                       // Mutex for thread safety

    int fd;                             // File descriptor for the node (if file)
} vfs_node_t;

typedef struct VFS_mount{
    filesystem_t* filesystem;           // Pointer to the filesystem struct
    char* mountPath;                    // Path to the mount point of the filesystem (i.e. /, /home, /usr, /mnt, etc.)
    vfs_node_t* mountPoint;             // Pointer to the mount point in the VFS
    struct VFS_mount* next;             // Pointer to the next mount (if any)
} mountpoint_t;

extern vfs_node_t* root;

vfs_node_t* VfsFindNode(char* path);
vfs_node_t* VfsMakeNode(char* name, bool isDirectory, bool readOnly, bool writeOnly, size_t size, unsigned int permissions, uid owner, void* data);
vfs_node_t* VfsGetNodeFromFd(int fd);
int VfsRemoveNode(vfs_node_t* node);
int VfsAddDevice(device_t* device, char* name, char* path);
int VfsAddChild(vfs_node_t* parent, vfs_node_t* child);
int InitializeVfs(multiboot_info_t* mbootInfo);
char* GetFullPath(vfs_node_t* node);
char* JoinPath(const char* base, const char* path);

int mountfs(filesystem_t* fs, mountpoint_t* mountPoint);
mountpoint_t* GetMountedFsFromPath(char* path);
mountpoint_t* GetMountedFsFromNode(vfs_node_t* node);

// Read a file from a mounted filesystem
void* VfsReadFile(vfs_node_t* node, size_t* size);

#endif