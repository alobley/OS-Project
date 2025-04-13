#include <vfs.h>
#include <alloc.h>
#include <console.h>

// TODO:
// - Add mutex checking in the VFS
// - Implement file reading and writing
// - Overhaul file descriptors and create a proper file descriptor list

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

vfs_node_t* VfsMakeNode(char* name, bool isDirectory, bool readOnly, bool writeOnly, bool isResizeable, size_t size, unsigned int permissions, uid owner, void* data){
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
    node->isResizeable = isResizeable;
    node->readOnly = readOnly;
    node->writeOnly = writeOnly;
    
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
    vfs_node_t* node = VfsMakeNode(name, false, false, false, false, 0, 0755, ROOT_UID, device);
    if(node == NULL){
        return STANDARD_FAILURE;
    }
    node->isDevice = true;
    VfsAddChild(VfsFindNode(path), node);

    device->path = halloc(strlen(path) + strlen(name) + 2);
    if(device->path == NULL){
        hfree(node->name);
        hfree(node);
        return STANDARD_FAILURE;
    }
    memset(device->path, 0, strlen(path) + strlen(name) + 2);
    strcpy(device->path, path);
    strcat(device->path, "/");
    strcat(device->path, name);
    return STANDARD_SUCCESS;
}

int VfsRemoveNode(vfs_node_t* node){
    if(node == NULL){
        return STANDARD_FAILURE;
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

file_context_t* CreateFileContext(vfs_node_t* node){
    file_context_t* context = (file_context_t*)halloc(sizeof(file_context_t));
    if(context == NULL){
        return NULL;
    }
    memset(context, 0, sizeof(file_context_t));
    context->node = node;
    context->refCount = 1; // Initially, one reference
    context->used = true;
    context->fd = 0;
    return context;
}

file_list_node_t* CreateListNode(file_context_t* context){
    file_list_node_t* node = (file_list_node_t*)halloc(sizeof(file_list_node_t));
    if(node == NULL){
        return NULL;
    }
    memset(node, 0, sizeof(file_list_node_t));
    node->context = context;
    node->next = NULL;
    return node;
}

file_list_t* CreateFileList(){
    file_list_t* list = (file_list_t*)halloc(sizeof(file_list_t));
    if(list == NULL){
        return NULL;
    }

    // Create stdin
    file_list_node_t* node = CreateListNode(CreateFileContext(VfsFindNode("/dev/stdin")));
    if(node == NULL){
        hfree(list);
        return NULL;
    }
    node->context->fd = STDIN_FILENO;
    node->context->used = true;
    node->context->refCount = 1;
    list->root = node;

    // Create stdout
    node->next = CreateListNode(CreateFileContext(VfsFindNode("/dev/stdout")));
    if(node->next == NULL){
        hfree(node);
        hfree(list);
        return NULL;
    }
    node = node->next;
    node->context->fd = STDOUT_FILENO;
    node->context->used = true;
    node->context->refCount = 1;

    // Create stderr
    node->next = CreateListNode(CreateFileContext(VfsFindNode("/dev/stderr")));
    if(node->next == NULL){
        hfree(node);
        hfree(list);
        return NULL;
    }
    node = node->next;
    node->context->fd = STDERR_FILENO;
    node->context->used = true;
    node->context->refCount = 1;
    node->next = NULL;

    list->size = 3;
    return list;
}

void VfsDetachMountpoint(vfs_node_t* mountNode) {
    if (!mountNode || !mountNode->mountPoint) return;
    
    mountpoint_t* mp = mountNode->mountPoint;
    
    // Recursively detach mountpoint from all children
    vfs_node_t* child = mountNode->firstChild;
    while (child) {
        child->mountPoint = NULL;
        if (child->isDirectory) {
            VfsDetachMountpoint(child);
        }
        child = child->next;
    }
    
    // Don't free the mountpoint - let the filesystem driver do that
    mountNode->mountPoint = NULL;
}

// Returns a file descriptor
int AddFileToList(file_list_t* list, file_context_t* context){
    if(list == NULL || context == NULL){
        return INVALID_FD;
    }
    
    int lastFd = INVALID_FD;
    // Before creating a node, try to find an unused node
    file_list_node_t* current = list->root;
    file_list_node_t* last = NULL; // Keep track of the last node
    
    while(current != NULL){
        lastFd = current->context->fd;
        if(current->context == NULL){
            current->context = context;
            current->context->used = true;
            current->context->refCount = 1;
            return current->context->fd;
        }
        last = current; // Update the last node
        current = current->next;
    }

    // If no unused node was found, create a new one
    file_list_node_t* newNode = CreateListNode(context);
    if(newNode == NULL){
        return INVALID_FD;
    }
    newNode->context->fd = lastFd + 1;
    newNode->context->used = true;
    newNode->context->refCount = 1;
    
    // Handle case where list is empty
    if(last == NULL) {
        list->root = newNode;
    } else {
        last->next = newNode;
    }
    
    list->size++;
    //printk("New file descriptor: %d\n", newNode->context->fd);
    return newNode->context->fd;
}

int RemoveFileFromList(file_list_t* list, int fd){
    if(list == NULL || fd < 0){
        return INVALID_FD;
    }
    file_list_node_t* current = list->root;
    file_list_node_t* prev = NULL;

    while(current != NULL){
        if(current->context != NULL && current->context->fd == fd && current->context->refCount == 1){
            if(prev == NULL){
                list->root = current->next;
            }else{
                prev->next = current->next;
            }
            hfree(current->context);
            hfree(current);
            list->size--;
            return STANDARD_SUCCESS;
        }else{
            if(current->context != NULL && current->context->fd == fd){
                current->context->refCount--;
                prev->next = current->next;
                if(current->context->refCount == 0){
                    hfree(current->context);
                    hfree(current);
                    list->size--;
                }
                hfree(current);
                return STANDARD_SUCCESS;
            }
        }
        prev = current;
        current = current->next;
    }
    return INVALID_FD;
}

void DestroyFileList(file_list_t* list){
    if(list == NULL){
        return;
    }
    file_list_node_t* current = list->root;
    while(current != NULL){
        file_list_node_t* next = current->next;
        hfree(current);
        current = next;
    }
    hfree(list);
}

file_context_t* FindFile(file_list_t* list, int fd){
    // Since the file descriptors are added in numerical order, we can just iterate through the list
    file_list_node_t* current = list->root;
    if(current == NULL){
        printk("Current was NULL!\n");
        return NULL;
    }
    // Check if the file descriptor is valid
    //printk("Searching for file descriptor %d\n", fd);
    for(size_t i = 0; i < list->size; i++){
        //printk("File descriptor %d\n", current->context->fd);
        if(current->context != NULL && current->context->fd == fd){
            return current->context;
        }
        current = current->next;
    }

    //printk("File descriptor not found!\n");
    return NULL;
}

// Function to search for a free file descriptor...

// Function to create a new file and read its data based on its mountpoint (if any)...

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

    root = VfsMakeNode("/", true, false, false, true, 2, 0755, ROOT_UID, NULL);
    if(root == NULL){
        return STANDARD_FAILURE;
    }

    root->mountPoint = rootMount;

    int status = 0;

    // Make the /dev directory
    vfs_node_t* dev = VfsMakeNode("dev", true, false, false, true, 0, 0755, ROOT_UID, NULL);
    if(dev == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, dev);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /initrd directory
    vfs_node_t* initrd = VfsMakeNode("initrd", true, false, false, true, 0, 0755, ROOT_UID, NULL);
    if(initrd == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, initrd);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    // Make the /mnt directory
    vfs_node_t* mnt = VfsMakeNode("mnt", true, false, false, true, 0, 0755, ROOT_UID, NULL);
    if(mnt == NULL){
        return STANDARD_FAILURE;
    }
    status = VfsAddChild(root, mnt);
    if(status != STANDARD_SUCCESS){
        return STANDARD_FAILURE;
    }

    vfs_node_t* rootmnt = VfsMakeNode("root", true, false, false, true, 0, 0755, ROOT_UID, NULL);
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