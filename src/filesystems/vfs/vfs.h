#ifndef VFS_H
#define VFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <users.h>
#include <time.h>
#include <multiboot.h>

#define VFS_ROOT "/"

// A structure that contains metadata for an entry in the virtual filesystem.
typedef struct VFS_Node {
    char* name;
    bool isDirectory;
    size_t size;                            // If file, the number of bytes. If directory, the number of children.
    union Pointer{
        struct VFS_Node* firstChild;        // If directory
        void* data;                         // If file (data type to be determined elsewhere)
    } pointer;
    struct VFS_Node* parent;                // Parent directory
    struct VFS_Node* next;                  // Next sibling
    unsigned int permissions;               // Permissions for the file or directory (the owner can do anything to it)
    uid owner;                              // The owner of the file or directory
    datetime_t created;                     // The time the file or directory was created
    datetime_t modified;                    // The time the file or directory was last modified
    datetime_t accessed;                    // The time the file or directory was last accessed
} vfs_node_t;

extern vfs_node_t* root;

vfs_node_t* VfsFindNode(char* path);
vfs_node_t* VfsMakeNode(char* name, bool isDirectory, size_t size, unsigned int permissions, uid owner, void* pointer);
int VfsAddChild(vfs_node_t* parent, vfs_node_t* child);
void vfs_init(multiboot_info_t* mbootInfo);
char* GetFullPath(vfs_node_t* node);

#endif // VFS_H