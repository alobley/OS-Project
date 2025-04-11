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

typedef struct VFS_mount mountpoint_t;
typedef struct Filesystem filesystem_t;

typedef struct VFS_Node {
    char* name;                         // Name of the file or directory
    bool isDirectory;                   // Whether the node is a file or directory
    bool isDevice;                      // Whether the node is a device
    bool readOnly;                      // Whether the file is read-only
    bool writeOnly;                     // Whether the file is write-only
    bool isResizeable;                  // Whether the file is resizeable (STDOUT, for example, is not)
    size_t size;                        // If file, the size in bytes (if loaded), if directory, the number of children it has
    size_t offset;                      // Current offset in the file (offset in data buffer)
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
    mountpoint_t* mountPoint;           // Pointer to the mount point (if any)
    mutex_t lock;                       // Mutex for thread safety
} vfs_node_t;

typedef struct VFS_mount {
    filesystem_t* filesystem;           // Pointer to the filesystem struct
    device_t* device;                   // Pointer to the device struct
    char* mountPath;                    // Path to the mount point of the filesystem (i.e. /, /home, /usr, /mnt, etc.)
    vfs_node_t* mountPoint;             // Pointer to the mount point in the VFS
    struct VFS_mount* next;             // Pointer to the next mount (if any)
} mountpoint_t;


// File context for a file descriptor (each process gets its own list of file contexts)
// NOTE: When reading an actual file from the filesystem, the file context will be created and the file's data will be read into memory
// If on a filesystem and in the VFS, the buffer will be released when the file is closed and the file is written to the disk.
// What about creating new files?
// STDIN, STDOUT, and STDERR will be created for every process
typedef struct FileContext {
    int fd;                             // File descriptor
    bool used;                          // Whether the file context is in use
    vfs_node_t* node;                   // Pointer to the VFS node
    uint32_t refCount;                  // Reference count (number of processes using this file context)
} file_context_t;

typedef struct FileListNode {
    file_context_t* context;            // Pointer to the file context
    struct FileListNode* next;        // Pointer to the next node in the list
} file_list_node_t;

typedef struct FileList{
    file_list_node_t* root;             // Pointer to the root of the list
    size_t size;                        // Size of the list (number of nodes)
} file_list_t;

extern vfs_node_t* root;

vfs_node_t* VfsFindNode(char* path);
vfs_node_t* VfsMakeNode(char* name, bool isDirectory, bool readOnly, bool isResizeable, bool writeOnly, size_t size, unsigned int permissions, uid owner, void* data);
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

file_list_t* CreateFileList();
file_context_t* CreateFileContext(vfs_node_t* node);
file_list_node_t* CreateListNode(file_context_t* context);
int AddFileToList(file_list_t* list, file_context_t* context);
void DestroyFileList(file_list_t* list);
file_context_t* FindFile(file_list_t* list, int fd);
int RemoveFileFromList(file_list_t* list, int fd);

void VfsDetachMountpoint(vfs_node_t* mountNode);

#endif