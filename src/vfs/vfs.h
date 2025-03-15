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

typedef struct VFS_Node {
    char* name;                         // Name of the file or directory
    bool isDirectory;                   // Whether the node is a file or directory
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
} vfs_node_t;

extern vfs_node_t* root;

vfs_node_t* VfsFindNode(char* path);
vfs_node_t* VfsMakeNode(char* name, bool isDirectory, size_t size, unsigned int permissions, uid owner, void* data);
int VfsRemoveNode(vfs_node_t* node);
int VfsAddDevice(device_t* device, char* name, char* path);
int VfsAddChild(vfs_node_t* parent, vfs_node_t* child);
void InitializeVfs(multiboot_info_t* mbootInfo);
char* GetFullPath(vfs_node_t* node);
char* JoinPath(const char* base, const char* path);

#endif