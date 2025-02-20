#include <vfs.h>
#include <alloc.h>
#include <console.h>

vfs_node_t* root = NULL;

char* GetFullPath(vfs_node_t* node) {
    if (node == NULL) {
        return NULL;
    }
    
    if (node->parent == NULL) {
        // Root case: allocate and copy name
        char* path = (char*)halloc(strlen(node->name) + 1);
        if (path == NULL) {
            return NULL;
        }
        strcpy(path, node->name);
        return path;
    }
    
    // Get parent path recursively
    char* parentPath = GetFullPath(node->parent);
    if (parentPath == NULL) {
        return NULL;
    }
    
    // Calculate required size and allocate
    size_t parentLen = strlen(parentPath);
    size_t nodeLen = strlen(node->name);
    char* fullPath = (char*)halloc(parentLen + nodeLen + 2); // +2 for '/' and '\0'
    if (fullPath == NULL) {
        hfree(parentPath);
        return NULL;
    }
    
    // Construct full path
    strcpy(fullPath, parentPath);
    if(parentLen > 1){
        // If the parent path is not the root, add a slash
        strcat(fullPath, "/");
    }
    strcat(fullPath, node->name);
    
    hfree(parentPath);
    return fullPath;
}

vfs_node_t* VfsMakeNode(char* name, bool isDirectory, size_t size, unsigned int permissions, uid owner, void* pointer){
    vfs_node_t* node = (vfs_node_t*)halloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->name = name;
    node->isDirectory = isDirectory;
    node->size = size;
    node->parent = NULL;
    node->next = NULL;
    node->permissions = permissions;
    node->owner = owner;
    node->created = currentTime;
    node->modified = currentTime;
    node->accessed = currentTime;
    return node;
}

vfs_node_t* VfsFindNode(char* path) {
    if (!path || !root) {
        return NULL;
    }

    // Handle root path specially
    if (strcmp(path, "/") == 0) {
        return root;
    }

    // Create local copy of path for tokenization
    char* pathCopy = (char*)halloc(strlen(path) + 1);
    if (!pathCopy) {
        return NULL;
    }
    strcpy(pathCopy, path);

    vfs_node_t* current = root;
    char* token = strtok(pathCopy, "/");
    
    while (token != NULL) {
        if (!current->isDirectory) {
            hfree(pathCopy);
            return NULL;
        }

        // Find matching child node
        vfs_node_t* child = current->pointer.firstChild;
        bool found = false;

        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                current = child;
                found = true;
                break;
            }
            child = child->next;
        }

        if (!found) {
            hfree(pathCopy);
            return NULL;
        }

        token = strtok(NULL, "/");
    }

    hfree(pathCopy);
    return current;
}

int VfsAddChild(vfs_node_t* parent, vfs_node_t* child) {
    // Validate parameters
    if (parent == NULL || child == NULL) {
        return -1;
    }

    // Parent must be a directory
    if (!parent->isDirectory) {
        return -1;
    }

    // Check if child is already attached somewhere
    if (child->parent != NULL || child->next != NULL) {
        return -1;  // Child is already in a tree
    }

    // Clean child's connections to be safe
    child->parent = NULL;
    child->next = NULL;

    // Add child to parent
    if (parent->pointer.firstChild == NULL) {
        // First child case
        parent->pointer.firstChild = child;
    } else {
        // Find last child and append
        vfs_node_t* lastChild = parent->pointer.firstChild;
        while (lastChild->next != NULL) {
            lastChild = lastChild->next;
        }
        lastChild->next = child;
    }

    // Set up child's parent reference and increment size
    child->parent = parent;
    parent->size++;
    return 0;
}

int VfsRemoveChild(vfs_node_t* parent, vfs_node_t* child){
    if(parent == NULL || child == NULL){
        return -1;
    }
    if(parent->isDirectory){
        vfs_node_t* current = parent->pointer.firstChild;
        vfs_node_t* prev = NULL;
        while(current != NULL){
            if(current == child){
                if(prev == NULL){
                    parent->pointer.firstChild = current->next;
                }else{
                    prev->next = current->next;
                }
                parent->size--;
                return 0;
            }
            prev = current;
            current = current->next;
        }
    }
    return -1;
}

// This is gonna have to move
char* strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* result = (char*)halloc(len);
    if (result) {
        strcpy(result, str);
    }
    return result;
}

char* JoinPath(const char* base, const char* path) {
    // Handle absolute paths
    if (path[0] == '/') {
        return strdup(path);
    }
    
    // Allocate enough space for base + / + path + null terminator
    size_t len = strlen(base) + strlen(path) + 2;
    char* result = (char*)halloc(len);
    
    strcpy(result, base);
    // Add slash if base doesn't end with one
    if (base[strlen(base) - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, path);
    return result;
}

// Create the bare minimum needed for a functional VFS on boot
void vfs_init(multiboot_info_t* mbootInfo) {
    // Initialize the virtual filesystem
    // Create the root directory
    root = VfsMakeNode("/", true, 2, 0755, ROOT_UID, NULL);

    // Make the required directories for the initrd
    vfs_node_t* dev = VfsMakeNode("dev", true, 0, 0755, ROOT_UID, NULL);
    VfsAddChild(root, dev);
    vfs_node_t* initrd = VfsMakeNode("initrd", true, 0, 0755, ROOT_UID, NULL);
    VfsAddChild(root, initrd);

    // Make the ram0 file in the /dev directory
    vfs_node_t* rd = VfsMakeNode("ram0", false, 0, 0644, ROOT_UID, NULL);
    VfsAddChild(dev, rd);

    // Make the tty0 file in the /dev directory
    vfs_node_t* tty0 = VfsMakeNode("tty0", false, 0, 0644, ROOT_UID, NULL);
    VfsAddChild(dev, tty0);

    vfs_node_t* input = VfsMakeNode("input", true, 0, 0755, ROOT_UID, NULL);
    VfsAddChild(dev, input);

    vfs_node_t* keyboard = VfsMakeNode("kb0", false, 0, 0644, ROOT_UID, NULL);
    VfsAddChild(input, keyboard);

    vfs_node_t* ramdisk = VfsFindNode("/dev/ram0");
    // The file just exists to tell the kernel it's there, so it doesn't need data
    ramdisk->pointer.data = NULL;
    ramdisk->size = 1;

    // Get the initrd directory from the multiboot info
    multiboot_module_t* mod = (multiboot_module_t*)mbootInfo->mods_addr;
    // extract the data and convert it into VFS nodes...

    // Can now return and have the kernel load drivers and stuff
}

// Function that builds the final VFS...