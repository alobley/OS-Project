#ifndef VFS_H
#define VFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <users.h>
#include <time.h>

static const char* VFS_ROOT = "/";

// A structure that contains metadata for an entry in the virtual filesystem.
typedef struct VFS_Node {
    char* name;
    bool isDirectory;
    size_t size;                            // If file, the number of bytes. If directory, the number of children.
    union Pointer{
        struct VFS_Node** children;         // If directory (is an array of pointers to VFS_Node structs)
        void* data;                         // If file (data type to be determined elsewhere)
    } pointer;
    struct VFS_Node* parent;                // Parent directory
    unsigned int permissions;               // Permissions for the file or directory (the owner can do anything to it)
    uid owner;                              // The owner of the file or directory
    datetime_t created;                     // The time the file or directory was created
    datetime_t modified;                    // The time the file or directory was last modified
    datetime_t accessed;                    // The time the file or directory was last accessed
} vfs_node_t;

#endif // VFS_H