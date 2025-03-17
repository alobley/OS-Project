#include <vfs.h>
#include <alloc.h>

// TODO:
// - Add mutex checking in the VFS
// - Implement file reading and writing
// - Overhaul file descriptors and create a proper file descriptor table

// File descriptor counter (linear should be fine, who has 4 billion files?)
int currentfd = 2;              // 0 and 1 are stdin and stdout while 2 is stderr (should stderr have a file descriptor?)

vfs_node_t* root = NULL;

// This is gonna have to move
char* strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* result = (char*)halloc(len);
    if (result) {
        strcpy(result, str);
    }
    return result;
}

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

vfs_node_t* VfsMakeNode(char* name, bool isDirectory, bool readOnly, bool writeOnly, size_t size, unsigned int permissions, uid owner, void* data){
    vfs_node_t* node = (vfs_node_t*)halloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->name = strdup(name);
    if (node->name == NULL) {
        hfree(node);
        return NULL;
    }
    node->isDirectory = isDirectory;
    node->size = size;
    node->parent = NULL;
    node->next = NULL;
    node->permissions = permissions;
    node->owner = owner;
    node->created = currentTime;
    node->modified = currentTime;
    node->accessed = currentTime;
    node->lock = MUTEX_INIT;
    node->data = data;                   // This is a union, so it's safe to do this
    
    if(isDirectory) {
        node->fd = INVALID_FD;           // For directories, this will be invalid
    } else {
        node->fd = currentfd++;          // For files, this will be the next available file descriptor
    }
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
        vfs_node_t* child = current->firstChild;
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
        return STANDARD_FAILURE;
    }

    // Parent must be a directory
    if (!parent->isDirectory) {
        return STANDARD_FAILURE;
    }

    // Check if child is already attached somewhere
    if (child->parent != NULL || child->next != NULL) {
        return STANDARD_FAILURE;  // Child is already in a tree
    }

    // Clean child's connections to be safe
    child->parent = NULL;
    child->next = NULL;

    // Add child to parent
    if (parent->firstChild == NULL) {
        // First child case
        parent->firstChild = child;
    } else {
        // Find last child and append
        vfs_node_t* lastChild = parent->firstChild;
        while (lastChild->next != NULL) {
            lastChild = lastChild->next;
        }
        lastChild->next = child;
    }

    // Set up child's parent reference and increment size
    child->parent = parent;
    parent->size++;
    return STANDARD_SUCCESS;
}

int VfsRemoveChild(vfs_node_t* parent, vfs_node_t* child){
    if(parent == NULL || child == NULL){
        return STANDARD_FAILURE;
    }
    if(parent->isDirectory){
        vfs_node_t* current = parent->firstChild;
        vfs_node_t* prev = NULL;
        while(current != NULL){
            if(current == child){
                if(prev == NULL){
                    parent->firstChild = current->next;
                }else{
                    prev->next = current->next;
                }
                parent->size--;
                hfree(current->name);
                hfree(current);
                return STANDARD_SUCCESS;
            }
            prev = current;
            current = current->next;
        }
    }
    return STANDARD_FAILURE;
}

int VfsAddDevice(device_t* device, char* name, char* path){
    vfs_node_t* node = VfsMakeNode(name, false, true, true, 0, 0755, ROOT_UID, device);
    if(node == NULL){
        return STANDARD_FAILURE;
    }
    VfsAddChild(VfsFindNode(path), node);
    return STANDARD_SUCCESS;
}

int VfsRemoveNode(vfs_node_t* node){
    if(node == NULL){
        return STANDARD_FAILURE;
    }
    if(node->fd != INVALID_FD && node->fd == currentfd - 1){
        // Might as well reclaim file descriptors whenever we can
        currentfd--;
    }
    if(node->parent != NULL){
        VfsRemoveChild(node->parent, node);
    }
    if(node->isDirectory){
        while(node->firstChild != NULL){
            VfsRemoveChild(node, node->firstChild);
        }
    }
    hfree(node->name);
    hfree(node);
    return STANDARD_SUCCESS;
}

char* JoinPath(const char* base, const char* path) {
    if(base == NULL || path == NULL){
        return NULL;
    }

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

vfs_node_t* VfsGetNodeFromFd(int fd){
    vfs_node_t* current = root;
    // Recursively search for the node with the matching file descriptor from the root directory and all its child directories
    while(current != NULL){
        if(current->fd == fd){
            return current;
        }
        // Check children
        if(current->isDirectory){
            vfs_node_t* child = current->firstChild;
            while(child != NULL){
                if(child->fd == fd){
                    return child;
                }
                child = child->next;
            }
            current = current->next;
        }
    }
    return NULL;
}

// Create the bare minimum needed for a functional VFS on boot
int InitializeVfs(multiboot_info_t* mbootInfo) {
    // Initialize the virtual filesystem
    // Create the root directory
    root = VfsMakeNode("/", true, false, false, 2, 0755, ROOT_UID, NULL);
    if(root == NULL){
        return STANDARD_FAILURE;
    }

    int status = 0;

    // Make the /dev directory
    vfs_node_t* dev = VfsMakeNode("dev", true, false, false, 0, 0755, ROOT_UID, NULL);
    if(dev == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, dev);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /initrd directory
    vfs_node_t* initrd = VfsMakeNode("initrd", true, false, false, 0, 0755, ROOT_UID, NULL);
    if(initrd == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, initrd);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /mnt directory
    vfs_node_t* mnt = VfsMakeNode("mnt", true, false, false, 0, 0755, ROOT_UID, NULL);
    if(mnt == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, mnt);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // More directories?

    // TODO: Search for devices and add them to the VFS dynamically

    // Get the initrd directory from the multiboot info
    multiboot_module_t* mod = (multiboot_module_t*)mbootInfo->mods_addr;
    // extract the data and convert it into VFS nodes...

    // Can now return and have the kernel load drivers and stuff

    return STANDARD_SUCCESS;
}