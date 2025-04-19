#include <vfs.h>
#include <alloc.h>
#include <console.h>

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

vfs_node_t* VfsMakeNode(char* name, unsigned int flags, size_t size, unsigned int permissions, uid_t owner, void* data){
    vfs_node_t* node = (vfs_node_t*)halloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    node->name = strdup(name);
    if (node->name == NULL) {
        hfree(node);
        return NULL;
    }
    node->flags = flags;
    node->size = size;
    node->parent = NULL;
    node->next = NULL;
    node->permissions = permissions;
    node->owner = owner;
    node->created = currentTime;
    node->modified = currentTime;
    node->accessed = currentTime;
    node->data = data;                   // This is a union, so it's safe to do this
    
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
        if (!(current->flags & NODE_FLAG_DIRECTORY)) {
            // This would account for a kernel bug, since root should be a directory
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
    if (!(parent->flags & NODE_FLAG_DIRECTORY)) {
        return STANDARD_FAILURE;
    }

    // Check if child is already attached somewhere
    if (child->parent != NULL || child->next != NULL) {
        return STANDARD_FAILURE;  // Child is already in a list
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
    child->mountPoint = parent->mountPoint; // Inherit mount point if any
    return STANDARD_SUCCESS;
}

int VfsRemoveChild(vfs_node_t* parent, vfs_node_t* child){
    if(parent == NULL || child == NULL){
        return STANDARD_FAILURE;
    }
    if(parent->flags & NODE_FLAG_DIRECTORY){
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

vfs_node_t* VfsAddDevice(device_t* device, char* name, char* path, int permissions){
    vfs_node_t* node = VfsMakeNode(name, NODE_FLAG_DEVICE, 0, permissions, ROOT_UID, device);
    if(node == NULL){
        return NULL;
    }
    VfsAddChild(VfsFindNode(path), node);

    device->path = halloc(strlen(path) + strlen(name) + 2);
    if(device->path == NULL){
        hfree(node->name);
        hfree(node);
        return NULL;
    }
    memset(device->path, 0, strlen(path) + strlen(name) + 2);
    strcpy(device->path, path);
    strcat(device->path, "/");
    strcat(device->path, name);
    node->device = device;
    return node;
}

int VfsRemoveNode(vfs_node_t* node){
    if(node == NULL){
        return STANDARD_FAILURE;
    }
    if(node->flags & NODE_FLAG_DIRECTORY){
        while(node->firstChild != NULL){
            // This will be recursively called if there are children of children
            VfsRemoveChild(node, node->firstChild);
        }
    }
    if(node->parent != NULL){
        VfsRemoveChild(node->parent, node);
    }
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

// Set a bit in the FD bitmap and return its index
fd_t AllocateFileDescriptor(file_table_t* ft){
    for(size_t i = 0; i < sizeof(uint32_t) * 2048; i++){
        if(ft->bitmap[i] != 0xFFFFFFFF){
            for(size_t j = 0; j < 32; j++){
                if(!(ft->bitmap[i] & (1 << j))){
                    ft->bitmap[i] |= 1 << j;
                    return i * 32 + j;
                }
            }
        }
    }

    return -1;
}

void FreeFileDescriptor(fd_t file, file_table_t* table){
    table->bitmap[file / 32] &= ~(1 << (file % 32));
}

fd_t CreateFileContext(vfs_node_t* node, file_table_t* table, unsigned int flags){
    file_context_t* context = (file_context_t*)halloc(sizeof(file_context_t));
    if(context == NULL){
        return STANDARD_FAILURE;
    }
    memset(context, 0, sizeof(file_context_t));
    context->node = node;
    node->refCount++;
    context->fd = AllocateFileDescriptor(table);
    table->numOpenFiles++;
    if(table->arrSize < context->fd){
        table->openFiles = rehalloc(table->openFiles, table->numOpenFiles * sizeof(file_context_t*));
        if(table->openFiles == NULL){
            return EMERGENCY_NO_MEMORY;
        }
        table->arrSize++;
    }
    table->openFiles[context->fd] = context;
    return context->fd;
}

fd_t ReplaceFileContext(vfs_node_t* node, file_table_t* table, fd_t oldfd, fd_t newfd){
    if(newfd > DEFAULT_MAX_FDS || table->openFiles[newfd] != NULL || table->numOpenFiles < oldfd || table->openFiles[oldfd] == NULL){
        return STANDARD_FAILURE;
    }

    file_context_t* context = table->openFiles[oldfd];
    if(context == NULL){
        return STANDARD_FAILURE;
    }
    if(table->arrSize < newfd){
        table->openFiles = rehalloc(table->openFiles, table->numOpenFiles * sizeof(file_context_t*));
        if(table->openFiles == NULL){
            return EMERGENCY_NO_MEMORY;
        }
        table->arrSize++;
    }
    context->node = node;
    table->openFiles[oldfd] = NULL;
    context->fd = newfd;
    table->openFiles[newfd] = context;
    table->openFiles[context->fd] = context;
    return context->fd;
}

int DestroyFileContext(file_table_t* table, fd_t fd){
    table->numOpenFiles--;
    file_context_t* context = table->openFiles[fd];
    table->openFiles[fd] == NULL;
    FreeFileDescriptor(fd, table);
    context->node->refCount--;
    hfree(context);
    return STANDARD_SUCCESS;
}

void VfsDetachMountpoint(vfs_node_t* mountNode) {
    if (!mountNode || !mountNode->mountPoint) return;
    
    mountpoint_t* mp = mountNode->mountPoint;
    
    // Recursively detach mountpoint from all children
    vfs_node_t* child = mountNode->firstChild;
    while (child) {
        child->mountPoint = NULL;
        if(child->flags & NODE_FLAG_DIRECTORY) {
            VfsDetachMountpoint(child);
        }
        child = child->next;
    }
    
    // Don't free the mountpoint - let the filesystem driver do that
    mountNode->mountPoint = NULL;
}

// Create the bare minimum needed for a functional VFS on boot
int InitializeVfs(multiboot_info_t* mbootInfo) {
    // Initialize the virtual filesystem
    // Create the root directory
    mountpoint_t* rootMount = NULL;
    rootMount = (mountpoint_t*)halloc(sizeof(mountpoint_t));
    if(rootMount == NULL){
        return STANDARD_FAILURE;
    }
    memset(rootMount, 0, sizeof(mountpoint_t));
    rootMount->filesystem = NULL;
    rootMount->mountPath = (char*)halloc(2);
    if(rootMount->mountPath == NULL){
        return STANDARD_FAILURE;
    }
    strcpy(rootMount->mountPath, "/");
    rootMount->next = NULL;
    rootMount->mountPoint = NULL;

    root = VfsMakeNode("/", NODE_FLAG_DIRECTORY | NODE_FLAG_RESIZEABLE, 0, 0755, ROOT_UID, NULL);
    if(root == NULL){
        return STANDARD_FAILURE;
    }

    root->mountPoint = rootMount;

    int status = 0;

    // Make the /dev directory
    vfs_node_t* dev = VfsMakeNode("dev", NODE_FLAG_DIRECTORY | NODE_FLAG_RESIZEABLE, 0, 0755, ROOT_UID, NULL);
    if(dev == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, dev);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /initrd directory
    vfs_node_t* initrd = VfsMakeNode("initrd", NODE_FLAG_DIRECTORY | NODE_FLAG_RESIZEABLE, 0, 0755, ROOT_UID, NULL);
    if(initrd == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, initrd);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /mnt directory
    vfs_node_t* mnt = VfsMakeNode("mnt", NODE_FLAG_DIRECTORY | NODE_FLAG_RESIZEABLE, 0, 0755, ROOT_UID, NULL);
    if(mnt == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, mnt);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    vfs_node_t* rootmnt = VfsMakeNode("root", NODE_FLAG_DIRECTORY | NODE_FLAG_RESIZEABLE, 0, 0755, ROOT_UID, NULL);
    if(rootmnt == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, rootmnt);
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