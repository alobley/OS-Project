#ifndef VFS_H
#define VFS_H

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <multitasking.h>
#include <users.h>
#include <system.h>

#define VFS_ROOT "/"

typedef struct Mount_Point mountpoint_t;

// File descriptor for invalid file
#define INVALID_FD -1

typedef struct VFS_Node {
    char* name;                                 // Name of the file or directory
    unsigned int flags;                         // Flags for this node
    size_t size;                                // If file, the size in bytes (if loaded), if directory, the number of children it has
    union {
        struct VFS_Node* firstChild;            // Pointer to the first child (if directory)
        device_t* device;                       // Pointer to the device struct (if device)
        void* data;                             // Pointer to the file data (if file)
    };
    struct VFS_Node* parent;                    // Pointer to the parent node (if it exists)
    struct VFS_Node* next;                      // Pointer to the next sibling node (if it exists)
    unsigned int permissions;                   // Permissions for the file or directory    
    uid_t owner;                                // Owner of the file or directory
    datetime_t created;                         // Date and time the file or directory was created
    datetime_t modified;                        // Date and time the file or directory was last modified
    datetime_t accessed;                        // Date and time the file or directory was last accessed
    device_id_t device;                         // The device ID of the block device this node's filesystem resides on
    bool read;                                  // This directory has been read from the disk (if mounted).
    mountpoint_t* mountPoint;                   // Pointer to the mount point (if any)
    mutex_t lock;                               // Mutex for synchronization
    size_t refCount;                            // Reference count (number of processes using this file context). If zero, the data can be freed so as not to waste RAM.
    queue_t waitingFor;                         // Processes waiting on an update to this node
} vfs_node_t;

#define NODE_FLAG_DIRECTORY (1 << 0)            // Whether the node is a file or directory
#define NODE_FLAG_DEVICE (1 << 1)               // Whether the node is a device
#define NODE_FLAG_RO (1 << 2)                   // Whether the node is read-only
#define NODE_FLAG_WO (1 << 3)                   // Whether the node is write-only
#define NODE_FLAG_RESIZEABLE (1 << 4)           // Whether the node is resizeable
#define NODE_FLAG_TEMP (1 << 5)                 // This node is temporary and will be destroyed with the owned process
#define NODE_FLAG_IPC (1 << 6)                  // This node is being used for IPC
#define NODE_FLAG_MOUNTED (1 << 7)              // This VFS node has been mounted
#define NODE_FLAG_NOTREAD (1 << 8)              // A node has not been read into memory if it is mounted (be it file data or a directory)
#define NODE_RESERVED (1 << 9)

#define PIPE_BUFFER_SIZE 4080
typedef struct IPC_Pipe {
    pid_t requester;                            // The requesting process of this pipe
    pid_t responder;                            // The responding process to this pipe
    uint32_t type;                              // The type of pipe this is
    size_t length;                              // How much data is actually in the pipe (max 4080 bytes)
    void* address;                              // The virtual address of this pipe (primarily for kernel use)
    uint8_t data[PIPE_BUFFER_SIZE];             // The data in the pipe
} pipe_t;

typedef struct Pipe_Queue_Node {
    pipe_t* message;
    struct Pipe_Queue_Node* next;
    struct Pipe_Queue_Node* prev;
} pqnode;

typedef struct Pipe_Queue {
    pqnode* head;
    pqnode* tail;
    size_t count;
} pqueue_t;

#define PIPE_TYPE_STREAM 1
#define PIPE_TYPE_MESSAGE 2
#define PIPE_TYPE_FIFO 3
#define PIPE_TYPE_EVENT 4
#define PIPE_TYPE_SHAREDMEM 5
#define PIPE_TYPE_SOCKETPAIR 6

#define PIPE_SENT 0
#define PIPE_RECIEVED 0
#define PIPE_DEST_FULL -1

// File context for a file descriptor (each process gets its own file table)
typedef struct File_Context {
    int fd;                             // File descriptor
    vfs_node_t* node;                   // Pointer to the VFS node VFS nodes don't have to be linked to the VFS node tree. Temp files, for example.
    size_t offset;                      // Current offset in the file (offset in data buffer or the current direct to read)
    unsigned int flags;                 // The local flags
} file_context_t;

// The aforementioned file table
typedef struct File_Table {
    uint32_t* bitmap;                   // Bitmap for tracking open files
    file_context_t** openFiles;         // Array of open files
    size_t numOpenFiles;                // The number of currently open files
    size_t arrSize;                     // The size of the array
    int max_fds;                        // The maximum number of file descriptors this file can have
} file_table_t;

#define DEFAULT_MAX_FDS 65535

extern vfs_node_t* root;

vfs_node_t* VfsFindNode(char* path);
vfs_node_t* VfsMakeNode(char* name, unsigned int flags, size_t size, unsigned int permissions, uid_t owner, void* data);
int VfsRemoveNode(vfs_node_t* node);
vfs_node_t* VfsAddDevice(device_t* device, char* name, char* path, int permissions);
int VfsAddChild(vfs_node_t* parent, vfs_node_t* child);
int InitializeVfs(multiboot_info_t* mbootInfo);
char* GetFullPath(vfs_node_t* node);
char* JoinPath(const char* base, const char* path);

mountpoint_t* GetMountedFsFromPath(char* path);
mountpoint_t* GetMountedFsFromNode(vfs_node_t* node);

// After calling a mount function in a driver, this function copies the read nodes to kernel memory
int MountFS(vfs_node_t* rootDir, char* mountPath);

fd_t CreateFileContext(vfs_node_t* node, file_table_t* table, unsigned int flags);
int DestroyFileContext(file_table_t* table, fd_t fd);
fd_t AllocateFileDescriptor(file_table_t* ft);
void FreeFileDescriptor(fd_t file, file_table_t* table);

void VfsDetachMountpoint(vfs_node_t* mountNode);

int Chroot(vfs_node_t* newRoot);
int ChangeNodeOwner(vfs_node_t* node, uid_t newOwner);
int ChangeNodePerms(vfs_node_t* node, unsigned int perms);

#endif