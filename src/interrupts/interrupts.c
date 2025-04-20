#include <interrupts.h>
#include <console.h>
#include <keyboard.h>
#include <multitasking.h>
#include <kernel.h>
#include <time.h>
#include <devices.h>
#include <tty.h>
#include <acpi.h>
#include <elf.h>
#include <gdt.h>

// A pointer to this struct will be returned to drivers upon request
struct kernelapi api = {
    .version = 1,
    .halloc = halloc,
    .hfree = hfree,
    .rehalloc = rehalloc,
    .mmap = physpalloc,
    .VfsFindNode = VfsFindNode,
    .VfsMakeNode = VfsMakeNode,
    .VfsRemoveNode = VfsRemoveNode,
    .printk = printk,
};

void teststub(){
    return;
}

// There appears to be an INTO instruction specifically meant for calling an overflow exception handler. That's pretty cool.

#define NUM_ISRS 49

extern void _isr0(struct Registers*);
extern void _isr1(struct Registers*);
extern void _isr2(struct Registers*);
extern void _isr3(struct Registers*);
extern void _isr4(struct Registers*);
extern void _isr5(struct Registers*);
extern void _isr6(struct Registers*);
extern void _isr7(struct Registers*);
extern void _isr8(struct Registers*);
extern void _isr9(struct Registers*);
extern void _isr10(struct Registers*);
extern void _isr11(struct Registers*);
extern void _isr12(struct Registers*);
extern void _isr13(struct Registers*);
extern void _isr14(struct Registers*);
extern void _isr15(struct Registers*);
extern void _isr16(struct Registers*);
extern void _isr17(struct Registers*);
extern void _isr18(struct Registers*);
extern void _isr19(struct Registers*);
extern void _isr20(struct Registers*);
extern void _isr21(struct Registers*);
extern void _isr22(struct Registers*);
extern void _isr23(struct Registers*);
extern void _isr24(struct Registers*);
extern void _isr25(struct Registers*);
extern void _isr26(struct Registers*);
extern void _isr27(struct Registers*);
extern void _isr28(struct Registers*);
extern void _isr29(struct Registers*);
extern void _isr30(struct Registers*);
extern void _isr31(struct Registers*);
extern void _isr32(struct Registers*);
extern void _isr33(struct Registers*);
extern void _isr34(struct Registers*);
extern void _isr35(struct Registers*);
extern void _isr36(struct Registers*);
extern void _isr37(struct Registers*);
extern void _isr38(struct Registers*);
extern void _isr39(struct Registers*);
extern void _isr40(struct Registers*);
extern void _isr41(struct Registers*);
extern void _isr42(struct Registers*);
extern void _isr43(struct Registers*);
extern void _isr44(struct Registers*);
extern void _isr45(struct Registers*);
extern void _isr46(struct Registers*);
extern void _isr47(struct Registers*);
extern void _isr48(struct Registers*);

NORET void reboot_system(){
    if(PS2ControllerExists()){
        // 8042 reset
        uint8_t good = 0x02;
        while (good & 0x02) good = inb(0x64);
        outb(0x64, 0xFE);
    }
    if(acpiInfo.exists){
        // ACPI reboot
        AcpiReboot();
    }

    uint32_t idt = 0;
    // If we get here, just triple fault
    lidt(idt)
    do_syscall(0, 0, 0, 0, 0, 0);

    STOP
    UNREACHABLE
}

extern pcb_t* kernelPCB;

static void regdump(struct Registers* regs){
    printk("EAX: 0x%x\n", regs->eax);
    printk("EBX: 0x%x\n", regs->ebx);
    printk("ECX: 0x%x\n", regs->ecx);
    printk("EDX: 0x%x\n", regs->edx);
    printk("ESI: 0x%x\n", regs->esi);
    printk("EDI: 0x%x\n", regs->edi);
    printk("EBP: 0x%x\n", regs->ebp);
    printk("ESP: 0x%x\n", regs->esp);
    printk("User ESP: 0x%x\n", regs->user_esp);
    printk("EIP: 0x%x\n", regs->eip);
    printk("CS: 0x%x\n", regs->cs);
    printk("DS: 0x%x\n", regs->ds);
    printk("ES: 0x%x\n", regs->es);
    printk("FS: 0x%x\n", regs->fs);
    printk("SS: 0x%x\n", regs->ss);
    printk("EFLAGS: 0x%x\n", regs->eflags);
    printk("Error code: 0b%b\n", regs->err_code);

    uint32_t cr0, cr2, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    printk("CR0: 0x%x ", cr0);
    printk("CR2: 0x%x ", cr2);
    printk("CR3: 0x%x ", cr3);
    printk("CR4: 0x%x\n", cr4);
} 

uid_t CheckPrivelige(){
    pcb_t* process = GetCurrentProcess();
    return process->user;
}

/// @brief The Dedication OS system call handler. This function is called upon int 0x30.
/// @param regs The CPU state of the calling program
/// @return Depends on the system call, can be sizes, pointers, file descriptors, or success/error codes
static HOT void syscall_handler(struct Registers *regs){
    pcb_t* currentProcess = GetCurrentProcess();
    if(currentProcess != NULL){
        currentProcess->inSyscall = true;
    }
    switch(regs->eax){
        case SYS_DBG:{
            printk("Syscall debug!\n");
            regs->eax = 0xDEADBEEF;
            break;
        }

        // Filesystem operations
        case SYS_WRITE:{
            // SYS_WRITE - write data to a file descriptor
            // EBX = fd
            // ECX = Pointer to the data buffer
            // EDX = Size of the data buffer in bytes

            // Returns: (ssize_t) bytes written

            // TODO: change this to page remapping or something so the kernel doesn't have to relay data (that's super inefficient and a security risk)

            if((regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(currentProcess->fileTable->openFiles[regs->ebx] != NULL && currentProcess->fileTable->openFiles[regs->ebx]->node->data != NULL){
                file_table_t* table = currentProcess->fileTable;
                file_context_t* context = table->openFiles[regs->ebx];
                vfs_node_t* node = context->node;

                if((node->flags & NODE_FLAG_RO) || (!(node->permissions & S_IROTH) && !(currentProcess->user == node->owner))  || (context->flags & O_RDONLY)){
                    regs->eax = SYSCALL_ACCESS_DENIED;
                    return;
                }

                if(node == NULL){
                    // We somehow got a NULL file (they are read upon the first SYS_OPEN or contain device data)
                    regs->eax = SYSCALL_FAILURE;
                    return;
                }

                // Lock the node (resizing can clobber pointers and device use should be done one process at a time)
                while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                }

                if(node->data == NULL && (node->flags & NODE_FLAG_RESIZEABLE)){
                    // This node is zero-sized, we need to allocate it
                    node->data = halloc(regs->edx);
                    if(node->data == NULL){
                        // Failed to allocate memory
                        MUnlock(&node->lock);
                        regs->eax = SYSCALL_OUT_OF_MEMORY;
                        return;
                    }
                    memset(node->data, 0, regs->edx);
                    context->offset = 0;
                    node->size = regs->edx;
                }else if(node->data == NULL){
                    // This node is zero-sized and not resizeable, we can't write to it
                    MUnlock(&node->lock);
                    regs->eax = SYSCALL_ACCESS_DENIED;
                    return;
                }

                if(node->flags & NODE_FLAG_DEVICE){
                    device_t* device = node->device;
                    driver_t* driver = device->driver;

                    while(MLock(&device->lock, currentProcess) == SEM_LOCKED){
                        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                    }

                    currentProcess->using = device;

                    regs->eax = device->ops.write(device->id, (void*)regs->ecx, regs->edx, context->offset);
                    if((int32_t)regs->eax < 0){
                        // Error writing to the device
                        regs->eax = SYSCALL_FAILURE;
                    }

                    MUnlock(&device->lock);
                    MUnlock(&node->lock);

                    currentProcess->using = NULL;

                    break;
                }else{
                    if(context->offset + regs->edx <= node->size && !(node->flags & NODE_FLAG_RO)){
                        // The remaining space in the file is large enough to write the new data and the file is not read only
                        memcpy(node->data + context->offset, (void*)regs->ecx, regs->edx);
                        context->offset += regs->edx;
                        regs->eax = regs->edx;
                    }else if((node->flags & NODE_FLAG_RESIZEABLE) && !(node->flags & NODE_FLAG_RO)){
                        // The size to write goes beyond the bounds of the file but it's resizeable
                        node->data = rehalloc(node->data, context->offset + regs->edx);
                        memcpy(node->data + context->offset, (void*)regs->ecx, regs->edx);
                        node->size = context->offset + regs->edx;
                        context->offset += regs->edx;
                        regs->eax = regs->edx;
                    }else if(!(node->flags & NODE_FLAG_RO)){
                        // Not resizeable and too small, just write as much as we can
                        memcpy(node->data + context->offset, (void*)regs->ecx, node->size - context->offset);
                        regs->eax = node->size - context->offset;
                    }else{
                        // File is read-only
                        regs->eax = SYSCALL_ACCESS_DENIED;
                    }
                }

                // Unlock the node
                MUnlock(&node->lock);

                // Update polls for this fd...
            }else{
                // Invalid fd or unopened file somehow got aquired
                regs->eax = SYSCALL_INVALID_ARGUMENT;
            }
            break;
        }

        case SYS_READ:{
            // SYS_READ - read data from a file descriptor
            // EBX = fd
            // ECX = pointer to the data buffer
            // EDX = size of the data buffer in bytes

            // Returns: (ssize_t) bytes read - negative on error

            // TODO: change this to page remapping or something so the kernel doesn't have to relay data

            if((regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(currentProcess->fileTable->openFiles[regs->ebx] != NULL && currentProcess->fileTable->openFiles[regs->ebx]->node->data != NULL){
                file_table_t* table = currentProcess->fileTable;
                file_context_t* context = table->openFiles[regs->ebx];
                vfs_node_t* node = context->node;

                if(node == NULL || node->data == NULL){
                    regs->eax = SYSCALL_FAILURE;
                    return;
                }

                if((node->flags & NODE_FLAG_WO) || (!(node->permissions & S_IROTH) && !(currentProcess->user == node->owner)) || (context->flags & O_WRONLY)){
                    // Node is write-only or caller doesn't have the right permissions
                    regs->eax = SYSCALL_ACCESS_DENIED;
                    return;
                }

                while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                }

                if(node->flags & NODE_FLAG_DEVICE){
                    device_t* device = node->device;
                    driver_t* driver = device->driver;

                    while(MLock(&device->lock, currentProcess) == SEM_LOCKED){
                        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                    }

                    currentProcess->using = device;

                    regs->eax = device->ops.read(device->id, (void*)regs->ecx, regs->edx, context->offset);
                    if((int32_t)regs->eax < 0){
                        // Error reading from the device
                        regs->eax = SYSCALL_FAILURE;
                    }
                    MUnlock(&device->lock);
                    MUnlock(&node->lock);

                    currentProcess->using = NULL;

                    break;
                }

                if(context->offset + regs->edx <= node->size && !(node->flags & NODE_FLAG_WO)){
                    // The remaining space in the file is large enough to read the given amount of data and the file is not write only
                    memcpy((void*)regs->ecx, node->data + context->offset, regs->edx);
                    context->offset += regs->edx;
                    regs->eax = regs->edx;
                }else if(!(node->flags & NODE_FLAG_WO)){
                    // Read until the end of the file
                    memcpy((void*)regs->ecx, node->data + context->offset, node->size - context->offset);
                    context->offset = node->size;
                    regs->eax = node->size - context->offset;
                }else{
                    // File is write-only or is not large enough for the read
                    regs->eax = SYSCALL_ACCESS_DENIED;
                }

                MUnlock(&node->lock);
            }else{
                regs->eax = SYSCALL_INVALID_ARGUMENT;
            }

            break;
        }

        case SYS_OPEN:{
            // SYS_OPEN - Open a file and return its file descriptor
            // EBX = Path
            // ECX = Flags
            // EDX = Mode (if opening new)

            // Returns file descriptor on success, otherwise failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            bool nodeMade = false;
            vfs_node_t* node = VfsFindNode((char*)regs->ebx);
            if(node != NULL && node->data == NULL && (regs->ecx | O_CREAT)){
                // Just start with a 100 byte buffer and make it resizeable
                void* data = halloc(100);
                if(data == NULL){
                    regs->eax = SYSCALL_OUT_OF_MEMORY;
                    return;
                }
                node = VfsMakeNode(strrchr((char*)regs->ebx, '/') + 1, regs->ecx | NODE_FLAG_RESIZEABLE, 100, regs->edx, currentProcess->user, data);
                if(node == NULL){
                    hfree(data);
                    regs->eax = SYSCALL_OUT_OF_MEMORY;
                    return;
                }
                *strrchr((char*)regs->ebx, '/') = '\0';
                VfsAddChild(VfsFindNode((char*)regs->ebx), node);
                nodeMade = true;
            }else if(node == NULL){
                // Not opening new and file not found
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(node->owner != currentProcess->effectiveUser && (node->permissions & (S_IROTH | S_IWOTH | S_IXOTH)) == 0){
                // Invalid permissions!
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((node->flags & regs->ecx) == 0){
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            // Only allow the supported flags
            regs->ecx &= node->flags;

            fd_t result = CreateFileContext(node, currentProcess->fileTable, regs->ecx);
            if(result == EMERGENCY_NO_MEMORY){
                // Process was compromized on a no memory error
                currentProcess->state = ZOMBIE;
                ContextSwitch(regs, GetNextProcess(currentProcess));          // Switch to next process
                DestroyProcess(currentProcess);                 // Cannibalize the caller on memory error
                if(nodeMade){
                    VfsRemoveNode(node);
                }
                return;
            }else if(result != STANDARD_SUCCESS){
                regs->eax = SYSCALL_OUT_OF_MEMORY;
                if(nodeMade){
                    VfsRemoveNode(node);
                }
                return;
            }

            node->refCount++;
            regs->eax = result;

            if(node->data == NULL && node->mountPoint != NULL && !nodeMade && !(node->flags & NODE_FLAG_DIRECTORY) && !(node->flags & NODE_FLAG_DEVICE) && node->read == false){
                // Switch to the driver and load the data
                device_t* device = node->mountPoint->fsDevice;
                driver_t* driver = &node->mountPoint->fsDriver->driver;

                currentProcess->using = device;

                while(MLock(&device->lock, currentProcess) == SEM_LOCKED){
                    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                }
                
                device->ops.read(device->id, node->data, node->size, 0);
                node->read = true;

                MUnlock(&device->lock);

                currentProcess->using = NULL;
            }

            MUnlock(&node->lock);

            return;
        }

        case SYS_CLOSE:{
            // SYS_CLOSE - Close a file descriptor
            // EBX = fd

            file_table_t* table = currentProcess->fileTable;

            if(table == NULL || regs->ebx > currentProcess->fileTable->arrSize){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = table->openFiles[regs->ebx]->node;
            if(node == NULL){
                // Invalid file descriptor
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(node->flags & NODE_FLAG_DEVICE){
                currentProcess->using = NULL;
            }

            // Luckily this was was extremely simple - nearly all the required logic is in this function
            DestroyFileContext(table, regs->ebx);
            
            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SEEK:{
            // SYS_SEEK - Seek to a position in a file's buffer
            // EBX = fd
            // ECX = Offset to seek to
            // EDX = Whence (0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END)

            // Returns: (ssize_t) new offset or negative value on failure

            if(currentProcess->fileTable->arrSize < regs->ebx){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            file_context_t* context = currentProcess->fileTable->openFiles[regs->ebx];
            if(context == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if((context->node->flags & NODE_FLAG_DEVICE) || (context->node->flags & NODE_FLAG_DIRECTORY)){
                // Invalid read
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(regs->edx == SEEK_SET){
                if(regs->ecx > context->node->size){
                    regs->eax = SYSCALL_INVALID_ARGUMENT;
                    return;
                }
                context->offset = regs->ecx;
                regs->eax = context->offset;
            }else if(regs->edx == SEEK_CUR){
                if(context->offset + (ssize_t)regs->ecx > context->node->size){
                    regs->eax = SYSCALL_INVALID_ARGUMENT;
                    return;
                }
                context->offset += (ssize_t)regs->ecx;
                regs->eax = context->offset;
            }else if(regs->edx == SEEK_END){
                // Offset to seek must be negative here
                if((ssize_t)context->node->size + (ssize_t)regs->ecx < 0 || (ssize_t)regs->ecx > 0){
                    // Too far back or positive seek value
                    regs->eax = SYSCALL_INVALID_ARGUMENT;
                    return;
                }
                context->offset = context->node->size;
                context->offset += (ssize_t)regs->ecx;
                regs->eax = context->offset;
            }else{
                regs->eax = SYSCALL_INVALID_ARGUMENT;
            }
            break;
        }

        case SYS_FSYNC:{
            // SYS_FSYNC - Write a file's data to disk
            // EBX = fd

            if(currentProcess->fileTable->arrSize < regs->ebx){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(currentProcess->fileTable->openFiles[regs->ebx] == NULL || currentProcess->fileTable->openFiles[regs->ebx]->node == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            file_context_t* context = currentProcess->fileTable->openFiles[regs->ebx];
            if(context == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            fs_driver_t* driver = context->node->mountPoint->fsDriver;
            if(driver == NULL || driver->fsync == NULL){
                // No driver or fsync function, can't sync
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            char* path = GetFullPath(context->node);
            if(path == NULL){
                // Failed to get the full path
                regs->eax = SYSCALL_FAILURE;
                return;
            }
            regs->eax = driver->fsync(path);
            hfree(path);
            break;
        }

        case SYS_SYNC:{
            // SYS_SYNC - Write the buffers of all files (if in a mounted filesystem) to the disk
            // No arguments(?)

            // Will need to search for mountpoints. Maybe I need a mountpoint registry? Or maybe I can find all filesystem drivers and fsync them?

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_TRUNCATE:{
            // SYS_TRUNCATE - set the size of a file to something specific
            // EBX = fd
            // ECX = size

            // Returns: (ssize_t) The new size of the file, or error if negative

            file_table_t* table = currentProcess->fileTable;
            if(regs->ebx > table->arrSize || table->openFiles[regs->ebx] == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            file_context_t* context = table->openFiles[regs->ebx];
            vfs_node_t* node = context->node;

            while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
            }

            // Check the node's permissions to make sure that writing and resizing is allowed
            if((node->flags & NODE_FLAG_RO) || (!(node->permissions & S_IROTH) && !(currentProcess->user == node->owner))  || (context->flags & O_RDONLY) || !(node->flags & NODE_FLAG_RESIZEABLE)){
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(node == NULL || node->data == NULL){
                // This is likely the result of a kernel bug
                regs->eax = SYSCALL_FAILURE;
                MUnlock(&node->lock);
                return;
            }

            if((node->flags & NODE_FLAG_DIRECTORY) || (node->flags & NODE_FLAG_DEVICE)){
                // Can't truncate devices or directories!
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                MUnlock(&node->lock);
                return;
            }

            // We can now resize the node
            if(regs->ecx > 0){
                void* newData = halloc(regs->ecx);
                if(newData == NULL){
                    regs->eax = SYSCALL_OUT_OF_MEMORY;
                    MUnlock(&node->lock);
                    return;
                }
                memset(newData, 0, regs->ecx);
                size_t copySize = (node->size < regs->ecx) ? node->size : regs->ecx;
                memcpy(newData, node->data, copySize);
                hfree(node->data);
                node->data = newData;
            }else{
                hfree(node->data);
                node->data = halloc(1);             // To prevent attempts to read the file from disk
                if(node->data == NULL){
                    // Basically impossible, but I need to cover everything.
                    MUnlock(&node->lock);
                    regs->eax = SYSCALL_FAILURE;
                    return;
                }
            }

            if(context->offset > regs->ecx){
                context->offset = regs->ecx;
            }
            
            node->size = regs->ecx;

            MUnlock(&node->lock);

            regs->eax = regs->ecx;
            break;
        }

        case SYS_STAT:{
            // SYS_STAT - Get file metadata from a given path
            // EBX = Path
            // ECX = struct stat*

            // This should be implemented ASAP

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_FSTAT:{
            // SYS_FSTAT - Get file metadata from a given fd
            // EBX = fd
            // ECX = struct stat*

            // This one shouldn't be too bad to implement, it's important anyway

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_GETDENTS:{
            // SYS_GETDENTS - Get directory entries from a given fd
            // EBX = fd
            // ECX = struct dirent*
            // EDX = size of the buffer

            // Returns: (ssize_t) bytes read - negative on error

            // I need to implement this one too ASAP

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_CHMOD:{
            // SYS_CHMOD - Change the permissions of a file
            // EBX = Path
            // ECX = mode

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_CHOWN:{
            // SYS_CHOWN - Change the owner of a file
            // EBX = Path
            // ECX = uid_t

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SYMLINK:{
            // SYS_SYMLINK - Create a symbolic link
            // EBX = Path
            // ECX = Target

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_UNLINK:{
            // SYS_UNLINK - Remove a file
            // EBX = Path

            // Returns: Success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }
            vfs_node_t* node = VfsFindNode((char*)regs->ebx);

            if(node == NULL || (!(node->permissions & S_IWOTH) && node->owner != currentProcess->user && currentProcess->user != ROOT_UID)){
                // File not found or invalid permissions
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
            }

            if(node->mountPoint != NULL){
                // This node has a mountpoint, so delete the original node
                if(node->mountPoint->fsDriver->delete != NULL){
                    node->mountPoint->fsDriver->delete((char*)regs->ebx);
                }
            }

            VfsRemoveNode(node);

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_MKDIR:{
            // SYS_MKDIR - Create a directory
            // EBX = Path
            // ECX = Mode

            // Returns: Success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsMakeNode(strrchr((char*)regs->ebx, '/') + 1, NODE_FLAG_DIRECTORY, 0, regs->ecx, currentProcess->user, NULL);
            if(node == NULL){
                regs->eax = SYSCALL_OUT_OF_MEMORY;
                return;
            }
            *strrchr((char*)regs->ebx, '/') = '\0';
            VfsAddChild(VfsFindNode((char*)regs->ebx), node);

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_RMDIR:{
            // SYS_RMDIR - Remove a directory
            // EBX = Path

            // Returns: Success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsFindNode((char*)regs->ebx);

            if(node == NULL || (!(node->permissions & S_IWOTH) && node->owner != currentProcess->user && currentProcess->user != ROOT_UID)){
                // File not found or invalid permissions
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                // After this, it will be destroyed, so no reason to unlock it
                do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
            }

            if(node->mountPoint != NULL){
                // This node has a mountpoint, so delete the original node
                if(node->mountPoint->fsDriver->delete != NULL){
                    node->mountPoint->fsDriver->delete((char*)regs->ebx);
                }
            }

            VfsRemoveNode(node);

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_RENAME:{
            // SYS_RENAME - Rename a file
            // EBX = Path
            // ECX = New name

            // Returns: Success or failure


            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsFindNode((char*)regs->ebx);

            if(node == NULL || (!(node->permissions & S_IWOTH) && node->owner != currentProcess->user && currentProcess->user != ROOT_UID)){
                // File not found or invalid permissions
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
            }

            if(node->mountPoint != NULL){
                // This node has a mountpoint, so delete the original node
                if(node->mountPoint->fsDriver->delete != NULL){
                    node->mountPoint->fsDriver->delete((char*)regs->ebx);
                }
            }

            char* newname = strdup((char*)regs->ecx);
            if(newname == NULL){
                // Failed to allocate memory
                MUnlock(&node->lock);
                regs->eax = SYSCALL_OUT_OF_MEMORY;
                return;
            }

            hfree(node->name);
            node->name = newname;

            MUnlock(&node->lock);

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_GETCWD:{
            // SYS_GETCWD - Get the current working directory
            // EBX = Path buffer
            // ECX = Size of the buffer

            // Returns: Success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(currentProcess->workingDirectory == NULL){
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            char* fullPath = GetFullPath(currentProcess->workingDirectory);
            if(fullPath == NULL){
                // Failed to get the full path
                regs->eax = SYSCALL_FAILURE;
                return;
            }
            if(strlen(fullPath) == 0){
                // Empty path
                hfree(fullPath);
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            size_t bytesToCopy = 0;
            if(strlen(fullPath) > regs->ecx){
                bytesToCopy = regs->ecx;
            }else{
                bytesToCopy = strlen(fullPath);
            }
            memcpy((char*)regs->ebx, fullPath, bytesToCopy);
            ((char*)regs->ebx)[bytesToCopy] = '\0';             // Null terminate the string
            hfree(fullPath);
            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_CHDIR:{
            // SYS_CHDIR - Change the current working directory
            // EBX = Path
            // ECX = Length of the string

            // Returns: Success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            char* dir = (char*)regs->ebx;
            if(strlen(dir) > regs->ecx || strlen(dir) == 0){
                // Invalid length
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* current = currentProcess->workingDirectory;
            if(current == NULL || !(current->flags & NODE_FLAG_DIRECTORY)){
                // No or invalid current working directory
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            if(strcmp(dir, "..") == 0 && current->parent != NULL){
                currentProcess->workingDirectory = current->parent;
                regs->eax = SYSCALL_SUCCESS;
                break;
            }else if(strcmp(dir, ".") == 0){
                regs->eax = SYSCALL_SUCCESS;
                break;
            }

            char* currentPath = GetFullPath(current);
            if(currentPath == NULL){
                // Failed to get the full path
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            char* newPath = JoinPath(currentPath, dir);
            if(newPath == NULL){
                // Failed to join the path
                hfree(currentPath);
                regs->eax = SYSCALL_FAILURE;
                return;
            }
            vfs_node_t* newDir = VfsFindNode(newPath);
            if(newDir == NULL || !(newDir->flags & NODE_FLAG_DIRECTORY)){
                // Invalid directory
                hfree(currentPath);
                hfree(newPath);
                regs->eax = SYSCALL_INVALID_ARGUMENT;
            }else{
                hfree(newPath);
                hfree(currentPath);
                currentProcess->workingDirectory = newDir;
                regs->eax = SYSCALL_SUCCESS;
            }

            break;
        }

        case SYS_MOUNT:{
            // SYS_MOUNT - Mount a filesystem
            // EBX = Mount path
            // ECX = Device path

            // Returns: Success or failure (you'll know if it worked)

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END || strlen((char*)regs->ebx) == 0) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }
            if((regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END || strlen((char*)regs->ecx) == 0) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsFindNode((char*)regs->ebx);
            if(node == NULL || !(node->flags & NODE_FLAG_DIRECTORY)){
                // Invalid directory
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            device_t* partition = VfsFindNode((char*)regs->ecx)->device;
            if(partition == NULL){
                // Invalid device
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            fs_driver_t* driver = FindFsDriver(partition);
            if(driver == NULL || driver->mount == NULL){
                // No driver found
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            if(driver->mount(partition->id, (char*)regs->ebx) != 0){
                // Failed to mount the filesystem
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_UMOUNT:{
            // SYS_UMOUNT - Unmount a filesystem
            // EBX = Mount path

            // Returns: success or failure

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END || strlen((char*)regs->ebx) == 0) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsFindNode((char*)regs->ebx);
            if(node == NULL || !(node->flags & NODE_FLAG_DIRECTORY)){
                // Invalid directory
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            fs_driver_t* driver = FindFsDriver(node->device);
            if(driver == NULL || driver->unmount == NULL){
                // No driver found
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            if(driver->unmount(node->mountPoint->fsDevice->id, (char*)regs->ebx) != 0){
                // Failed to unmount the filesystem
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_DUP:{
            // SYS_DUP - Duplicate a file descriptor
            // EBX = fd

            // Returns: New fd on success, otherwise failure

            if(currentProcess->fileTable->arrSize < regs->ebx){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            file_context_t* context = currentProcess->fileTable->openFiles[regs->ebx];
            if(context == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            fd_t result = CreateFileContext(context->node, currentProcess->fileTable, context->flags);
            if(result == EMERGENCY_NO_MEMORY){
                // Process was compromized on a no memory error
                ContextSwitch(regs, GetNextProcess(currentProcess));          // Switch to next process
                DestroyProcess(currentProcess);                 // Destroy the running process
                return;
            }else if(result != STANDARD_SUCCESS){
                regs->eax = SYSCALL_OUT_OF_MEMORY;
                return;
            }

            regs->eax = result;
            break;
        }

        case SYS_DUP2:{
            // SYS_DUP2 - Duplicate a file descriptor to a specific fd
            // EBX = fd
            // ECX = new fd

            // Returns: New fd on success, otherwise failure

            if(currentProcess->fileTable->arrSize < regs->ebx || currentProcess->fileTable->arrSize < regs->ecx){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            file_context_t* context = currentProcess->fileTable->openFiles[regs->ebx];
            if(context == NULL){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = context->node;
            if(node == NULL){
                // Invalid file descriptor
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            ReplaceFileContext(node, currentProcess->fileTable, regs->ebx, regs->ecx);

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_FCNTL:{
            // SYS_FCNTL - Manipulate a file descriptor
            // EBX = fd
            // ECX = Flags

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_IOCTL:{
            // SYS_IOCTL - Perform an IOCTL operation on a file descriptor (devices only)
            // EBX = fd
            // ECX = Command
            // EDX = Argument

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_POLL:{
            // SYS_POLL - Wait for a set of file descriptors to be ready
            // EBX = fd
            // ECX = Timeout

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        // Process management
        case SYS_EXIT:{
            // SYS_EXIT - Exit the current process
            // EBX = Exit code

            // Returns: Nothing

            if(currentProcess == kernelPCB){
                // Kernel process cannot exit
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            currentProcess->exitStatus = regs->ebx;
            currentProcess->state = ZOMBIE;

            while(currentProcess->waiting != NULL){
                // Send to all waiting processes that the process has finished
                currentProcess->waiting->this->context.eax = currentProcess->exitStatus;
                currentProcess->waiting->this->state = RUNNING;
                struct Process_List* next = currentProcess->waiting->next;
                hfree(currentProcess->waiting);
                currentProcess->waiting = next;
            }

            UnscheduleProcess(currentProcess);                      // Remove the process from the scheduler
            RemoveProcessFromQueue(currentProcess, &currentProcess->waitingFor->waitQueue);     // Remove the process from the waiting queue

            DestroyProcess(currentProcess);
            ContextSwitch(regs, GetNextProcess(currentProcess));                  // Switch to next process
            break;
        }

        case SYS_FORK:{
            // SYS_FORK - Fork the current process
            // No arguments(?)

            // Returns: Child PID on success, otherwise failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot fork
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            pcb_t* child = DuplicateProcess(currentProcess);
            if(child == NULL){
                regs->eax = SYSCALL_OUT_OF_MEMORY;
                return;
            }
            ScheduleProcess(child);                         // Add the child to the scheduler

            child->context.eax = 0;                         // Set the child's return value to 0

            regs->eax = child->pid;
            break;
        }

        case SYS_EXEC:{
            // SYS_EXEC - Execute a new program
            // EBX = Path
            // ECX = Arguments

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot exec
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            char* path = (char*)regs->ebx;
            if(strlen(path) == 0){
                // Invalid length
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            vfs_node_t* node = VfsFindNode(path);
            if(node == NULL || (!(node->permissions & S_IXOTH) && currentProcess->user != ROOT_UID && currentProcess->user != node->owner) || (node->flags & NODE_FLAG_DIRECTORY) || (node->flags & NODE_FLAG_DEVICE)){
                // Invalid executable
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(node->owner != currentProcess->effectiveUser && (node->permissions & S_IROTH) == 0){
                // Invalid permissions!
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            char* name = strrchr(path, '/');
            if(name == NULL){
                name = path;
            }else{
                *name = '\0';
                name++;
            }
            if(ReplaceProcess(name, node->data, currentProcess, regs) != STANDARD_SUCCESS){
                // Failed to replace the process
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            // The process is now replaced, so we don't need to do anything else. It's already scheduled too because the PCB's address didn't change, and the registers were handled in the function.

            break;
        }

        case SYS_EXECVE:{
            // SYS_EXECVE - Execute a new program with a specific environment
            // EBX = Path
            // ECX = Arguments
            // EDX = Environment

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_WAIT_PID:{
            // SYS_WAIT_PID - Wait for a process to finish
            // EBX = PID

            // Returns: Exit code of the process, or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot wait
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 || regs->ebx > 65535){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            pcb_t* child = GetProcessByPID(regs->ebx);
            if(child == NULL){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            // Wait for the child to finish
            currentProcess->state = WAITING;
            do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);

            // Everything else is handled in SYS_EXIT

            break;
        }

        case SYS_GET_PID:{
            // SYS_GET_PID - Get the PID of the current process
            // No arguments

            // Luckily this is extremely simple - just return the PID of the current process
            regs->eax = currentProcess->pid;
            break;
        }

        case SYS_GET_PPID:{
            // SYS_GET_PPID - Get the PID of the parent process
            // No arguments

            // Returns: Parent PID

            if(currentProcess->parent == NULL){
                regs->eax = 0;
            }else{
                regs->eax = currentProcess->parent->pid;
            }
            break;
        }

        case SYS_KILL:{
            // SYS_KILL - Kill a process
            // EBX = PID

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot kill
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 || regs->ebx > 65535){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            pcb_t* target = GetProcessByPID(regs->ebx);
            if(target == NULL){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(target == currentProcess){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(target->parent != currentProcess){
                // You cannot kill a process that is not your child
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(target->state == ZOMBIE){
                // Process is already dead
                regs->eax = SYSCALL_SUCCESS;
                return;
            }

            if(InsertSignals(target, SIGTERM) != STANDARD_SUCCESS){
                // Failed to insert the signal
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_YIELD:{
            // SYS_YIELD - Yield the current process
            // No arguments

            if(currentProcess == kernelPCB){
                // Kernel process cannot yield
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            ContextSwitch(regs, GetNextProcess(currentProcess));          // Switch to next process
            break;
        }

        case SYS_GETRLIMIT:{
            // SYS_GETRLIMIT - Get the resource limits of the current process
            // EBX = Resource limit
            // ECX = struct rlimit*

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SETRLIMIT:{
            // SYS_SETRLIMIT - Set the resource limits of the current process
            // EBX = Resource limit
            // ECX = struct rlimit*

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }
        
        case SYS_GETUID:{
            // SYS_GETUID - Get the UID of the current process
            // No arguments

            // Returns: UID

            regs->eax = currentProcess->user;
            break;
        }

        case SYS_GETGID:{
            // SYS_GETGID - Get the GID of the current process
            // No arguments

            // Returns: GID

            regs->eax = currentProcess->group;
            break;
        }


        // Inter-Process Communication
        case SYS_PIPE:{
            // SYS_PIPE - Create a pipe
            // EBX = Pipe

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SHMGET:{
            // SYS_SHMGET - Get a shared memory segment
            // EBX = Key
            // ECX = Size

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SHMAT:{
            // SYS_SHMAT - Attach a shared memory segment
            // EBX = Key
            // ECX = Address

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SHMDT:{
            // SYS_SHMDT - Detach a shared memory segment
            // EBX = Key

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_MSGGET:{
            // SYS_MSGGET - Get a message queue
            // EBX = Key

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_MSGSND:{
            // SYS_MSGSND - Send a message to a queue
            // EBX = Key
            // ECX = Message

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_MSGRCV:{
            // SYS_MSGRCV - Receive a message from a queue
            // EBX = Key
            // ECX = Message

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SETSID:{
            // SYS_SETSID - Set the session ID of the current process
            // No arguments

            // Returns: nothing

            // Let's keep this simple
            regs->eax = currentProcess->pid;
            break;
        }

        case SYS_GETSID:{
            // SYS_GETSID - Get the session ID of the current process
            // No arguments

            // Returns: the session ID of the current process

            regs->eax = currentProcess->sessionId;
            break;
        }

        
        // Time management
        case SYS_SLEEP:{
            // SYS_SLEEP - Sleep for a given amount of time
            // EBX = Time in milliseconds (low 32)
            // ECX = Time in milliseconds (high 32)

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot sleep
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 && regs->ecx == 0){
                // Invalid time
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            uint64_t time = ((uint64_t)regs->ecx << 32) | (uint64_t)regs->ebx;
            if(time == 0){
                // Invalid time
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            currentProcess->state = SLEEPING;
            currentProcess->sleepUntil = GetTicks() + time;
            do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);

            regs->eax = SYSCALL_SUCCESS;

            break;
        }

        case SYS_GET_TIME:{
            // SYS_GET_TIME - Get the current time
            // EBX = pointer to the datetime_t struct

            // Returns: the time

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx >= USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            datetime_t* time = (datetime_t*)regs->ebx;
            memcpy(time, &currentTime, sizeof(datetime_t));

            regs->eax = SYSCALL_SUCCESS;
            break;
        }


        // Networking
        case SYS_SOCKET:{
            // SYS_SOCKET - Create a socket
            // EBX = Domain
            // ECX = Type
            // EDX = Protocol

            // Returns: Socket descriptor

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_BIND:{
            // SYS_BIND - Bind a socket to an address
            // EBX = Socket descriptor
            // ECX = Address

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_LISTEN:{
            // SYS_LISTEN - Listen for incoming connections
            // EBX = Socket descriptor
            // ECX = Backlog

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_ACCEPT:{
            // SYS_ACCEPT - Accept an incoming connection
            // EBX = Socket descriptor
            // ECX = Address

            // Returns: Socket descriptor

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_CONNECT:{
            // SYS_CONNECT - Connect to a remote socket
            // EBX = Socket descriptor
            // ECX = Address

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SENDTO:{
            // SYS_SEND - Send data to a socket
            // EBX = Socket descriptor
            // ECX = Buffer
            // EDX = Length

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_RECVFROM:{
            // SYS_RECV - Receive data from a socket
            // EBX = Socket descriptor
            // ECX = Buffer
            // EDX = Length

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }
        
        case SYS_SETSOCKOPT:{
            // SYS_SETSOCKOPT - Set socket options
            // EBX = Socket descriptor
            // ECX = Level
            // EDX = Option name
            // EAX = Option value

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_GETSOCKOPT:{
            // SYS_GETSOCKOPT - Get socket options
            // EBX = Socket descriptor
            // ECX = Level
            // EDX = Option name
            // EAX = Option value

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }


        // Memory management
        case SYS_MMAP:{
            // SYS_MMAP - Map a file into memory
            // EBX = File descriptor
            // ECX = Address
            // EDX = Length

            // Returns: Address of the mapped file

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_MUNMAP:{
            // SYS_MUNMAP - Unmap a file from memory
            // EBX = Address
            // ECX = Length

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_BRK:{
            // SYS_BRK - Set the program break
            // EBX = Address

            // Returns: Success or failure

            // I need some code for extending process heaps here, preferrably soon. Malloc is helpful.

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_MPROTECT:{
            // SYS_MPROTECT - Set memory protection
            // EBX = Address
            // ECX = Length
            // EDX = Protection flags

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SENDSIG:{
            // SYS_SENDSIG - Send a signal to a process
            // EBX = PID
            // ECX = Signal

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot send signals
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 || regs->ebx > 65535){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            pcb_t* target = GetProcessByPID(regs->ebx);
            if(target == NULL){
                // Invalid PID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(InsertSignals(target, regs->ecx) != STANDARD_SUCCESS){
                // Failed to insert the signal
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SIGACTION:{
            // SYS_SIGACTION - Set the action for a signal
            // EBX = Signal (expects the bitmask and only one bit)
            // ECX = Action

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot set signal actions
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0){
                // Invalid signal
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if((regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            currentProcess->signalInfo.handlers[__builtin_ctz(regs->ebx)] = (void*)regs->ecx;

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SIGPROCMASK:{
            // SYS_SIGPROCMASK - Set the signal mask
            // EBX = Signal mask (bitfield)

            // Returns: Success or failure

            if(currentProcess == kernelPCB){
                // Kernel process cannot set signal masks
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0){
                // Invalid signal mask
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            currentProcess->signalInfo.masked = regs->ebx;

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_ALARM:{
            // SYS_ALARM - Set an alarm that a process will have to handle
            // EBX = Time in seconds

            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }


        // Priveliged/Driver system calls
        case SYS_MODULE_LOAD:{
            // SYS_MODULE_LOAD - Load a kernel module
            // EBX = Path

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only priveliged processes can load modules
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            char* path = (char*)regs->ebx;
            if(strlen(path) == 0){
                // Invalid length
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            if(LoadModule(path) != STANDARD_SUCCESS){
                // Failed to load the module
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_MODULE_UNLOAD:{
            // SYS_MODULE_UNLOAD - Unload a kernel module
            // EBX = Module ID

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only priveliged processes can unload modules
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(UnloadModule(regs->ebx) != STANDARD_SUCCESS){
                // Failed to unload the module
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_MODULE_QUERY:{
            // SYS_MODULE_QUERY - Query a kernel module
            // EBX = Path

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only priveliged processes can query modules
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_REGISTER_DEVICE:{
            // SYS_REGISTER_DEVICE - Register a device driver
            // EBX = Pointer to device struct
            // ECX = pointer to device name
            // EDX = permissions

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can register devices
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(RegisterDevice((device_t*)regs->ebx, (char*)regs->ecx, regs->edx) != STANDARD_SUCCESS){
                // Failed to register the device
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_UNREGISTER_DEVICE:{
            // SYS_UNREGISTER_DEVICE - Unregister a device driver
            // EBX = Pointer to device struct

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can unregister devices
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(UnregisterDevice((device_t*)regs->ebx) != STANDARD_SUCCESS){
                // Failed to unregister the device
                regs->eax = SYSCALL_FAILURE;
                return;
            }

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_ACQUIRE_DEVICE:{
            // SYS_AQUIRE_DEVICE - Aquire an existing device to modify
            // EBX = Path to device in the VFS

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can aquire devices
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_REQUEST_IRQ:{
            // SYS_REQUEST_IRQ - Request an IRQ
            // EBX = IRQ number
            // ECX = Handler

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can request IRQs
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_RELEASE_IRQ:{
            // SYS_RELEASE_IRQ - Release an IRQ
            // EBX = IRQ number

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can release IRQs
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_GET_API:{
            // SYS_GET_API - Get the kernel API struct for pointers to kernel functions

            // Returns: Success or failure

            if(currentProcess->context.cs != GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE)){
                // Only drivers and the kernel can get the API version
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = (uint32_t)&api;
            break;
        }

        case SYS_SETUID:{
            // SYS_SETUID - Set the UID of the current process
            // EBX = UID

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can set the UID
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 || regs->ebx > 65536){
                // Invalid UID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            currentProcess->user = regs->ebx;

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SETGID:{
            // SYS_SETGID - Set the GID of the current process
            // EBX = GID

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can set the GID
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if(regs->ebx == 0 || regs->ebx > 65535){
                // Invalid GID
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            currentProcess->group = regs->ebx;

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SETGROUPS:{
            // SYS_SETGROUPS - Set the groups of the current process
            // EBX = Groups

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can set the groups
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_GETGROUPS:{
            // SYS_GETGROUPS - Get the groups of the current process
            // EBX = Groups

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can get the groups
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SET_TIME:{
            // SYS_SET_TIME - Set the current time
            // EBX = pointer to the datetime_t struct

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can get the groups
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx >= USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            datetime_t* time = (datetime_t*)regs->ebx;
            memcpy(&currentTime, time, sizeof(datetime_t));

            regs->eax = SYSCALL_SUCCESS;
            break;
        }

        case SYS_SHUTDOWN:{
            // SYS_SHUTDOWN - Shutdown the system

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can shutdown the system
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            AcpiShutdown();

            break;
        }

        case SYS_REBOOT:{
            // SYS_REBOOT - Reboot the system

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can reboot the system
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            reboot_system();

            break;
        }

        case SYS_UNAME:{
            // SYS_UNAME - Get system information
            // EBX contains the pointer to the uname struct to copy the info into

            if(currentProcess->user != ROOT_UID){
                // Only root can get the groups
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx >= USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            struct uname* info = (struct uname*)regs->ebx;
            info->uptime = GetTicks() / 1000; // Convert to seconds
            info->totalMemory = totalMemSize;
            info->usedMemory = mappedPages * PAGE_SIZE;
            info->freeMemory = (totalMemSize - (mappedPages * PAGE_SIZE));
            info->numProcesses = numProcesses;
            memcpy(&info->kernelVersion, &kernelVersion, sizeof(version_t));
            memcpy(info->kernelRelease, kernelRelease, strlen(kernelRelease) + 1);
            info->acpiSupported = acpiInfo.exists;
            uint32_t eax = 0;
            uint32_t others[4] = {0};
            cpuid(eax, others[0], others[1], others[2]);
            memcpy(info->cpuOEM, others, sizeof(others));
            regs->eax = STANDARD_SUCCESS;
            break;
        }

        case SYS_CHROOT:{
            // SYS_CHROOT - Change the root directory of the current process
            // EBX = Path
            
            // Returns: Success or failure

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_KLOG_READ:{
            // SYS_KLOG_READ - Read the kernel log
            // EBX = Pointer to buffer
            // ECX = Size of buffer

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can read the kernel log
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx >= USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            // Printk doesn't use a buffer yet unfortunately
            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_KLOG_FLUSH:{
            // SYS_KLOG_FLUSH - Flush the kernel log
            // EBX = Pointer to buffer
            // ECX = Size of buffer

            // Returns: Success or failure

            if(currentProcess->user != ROOT_UID){
                // Only root can flush the kernel log
                regs->eax = SYSCALL_ACCESS_DENIED;
                return;
            }

            if((regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx >= USER_MEM_END) && currentProcess != kernelPCB){
                // Invalid address
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
            }

            // Printk doesn't use a buffer yet unfortunately
            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        default:{
            // Invalid system call!
            regs->eax = SYSCALL_NOT_FOUND;
            break;
        }
    }
    if(currentProcess != NULL){
        currentProcess->inSyscall = false;
    }
}

static void (*stubs[NUM_ISRS])(struct Registers*) = {
    _isr0,
    _isr1,
    _isr2,
    _isr3,
    _isr4,
    _isr5,
    _isr6,
    _isr7,
    _isr8,
    _isr9,
    _isr10,
    _isr11,
    _isr12,
    _isr13,
    _isr14,
    _isr15,
    _isr16,
    _isr17,
    _isr18,
    _isr19,
    _isr20,
    _isr21,
    _isr22,
    _isr23,
    _isr24,
    _isr25,
    _isr26,
    _isr27,
    _isr28,
    _isr29,
    _isr30,
    _isr31,
    _isr32,
    _isr33,
    _isr34,
    _isr35,
    _isr36,
    _isr37,
    _isr38,
    _isr39,
    _isr40,
    _isr41,
    _isr42,
    _isr43,
    _isr44,
    _isr45,
    _isr46,
    _isr47,
    _isr48
};

enum exception {
    EXCEPTION_DIVIDE_BY_ZERO,
    EXCEPTION_DEBUG,
    EXCEPTION_NMI,
    EXCEPTION_BREAKPOINT,
    EXCEPTION_OVERFLOW,
    EXCEPTION_OOB,
    EXCEPTION_INVALID_OPCODE,
    EXCEPTION_NO_COPROCESSOR,
    EXCEPTION_DOUBLE_FAULT,
    EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN,
    EXCEPTION_BAD_TSS,
    EXCEPTION_SEGMENT_NOT_PRESENT,
    EXCEPTION_STACK_FAULT,
    EXCEPTION_GENERAL_PROTECTION_FAULT,
    EXCEPTION_PAGE_FAULT,
    EXCEPTION_UNRECOGNIZED_INTERRUPT,
    EXCEPTION_COPROCESSOR_FAULT,
    EXCEPTION_ALIGNMENT_CHECK,
    EXCEPTION_MACHINE_CHECK
};

static const char *exceptions[32] = {
    "Divide by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "OOB",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unrecognized interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED"
};

static struct {
    size_t index;
    void (*stub)(struct Registers*);
} isrs[NUM_ISRS];

static void (*handlers[NUM_ISRS])(struct Registers*) = { 0 };

void InstallISR(size_t i, void (*handler)(struct Registers*)){
    handlers[i] = handler;
}

extern uint8_t intstack_base;
extern uint8_t intstack;

void ISRHandler(struct Registers *regs){
    tss.esp0 = regs->esp;                                   // Help reduce stack corruption upon a nested interrupt
    if (handlers[regs->int_no]) {
        handlers[regs->int_no](regs);
    }
    tss.esp0 = (uint32_t)&intstack;
    tss.ss0 = GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_DATA);
}

struct IDTEntry{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t __ignored;
    uint8_t type;
    uint16_t offset_high;
} PACKED;

struct IDTPointer {
    uint16_t limit;
    uintptr_t base;
} PACKED;

static struct {
    struct IDTEntry entries[256];
    struct IDTPointer pointer;
} idt;

extern void LoadIDT();

void SetIDT(uint8_t index, void(*base)(struct Registers*), uint16_t selector, uint8_t flags){
    idt.entries[index] = (struct IDTEntry){
        .offset_low = ((uintptr_t) base) & 0xFFFF,
        .offset_high = (((uintptr_t) base) >> 16) & 0xFFFF,
        .selector = selector,
        .type = flags,
        .__ignored = 0
    };
}

void InitIDT(){
    idt.pointer.limit = sizeof(idt.entries) - 1;
    idt.pointer.base = (uintptr_t) &idt.entries[0];
    memset(&idt.entries[0], 0, sizeof(idt.entries));
    LoadIDT((uintptr_t) &idt.pointer);
}

static void ExceptionHandler(struct Registers *regs){
    cli
    // Uncomment this when debugging system calls
    printk("KERNEL PANIC: %s\n", exceptions[regs->int_no]);
    regdump(regs);
    STOP

    volatile pcb_t* current = GetCurrentProcess();
    if(current == kernelPCB){
        // Exception was thrown by the kernel - cannot recover
        printk("KERNEL PANIC: %s\n", exceptions[regs->int_no]);
        //printk("Current PCB address: 0x%x\n", current);
        //printk("Kernel PCB address: 0x%x\n", kernelPCB);
        regdump(regs);
        STOP
    }
    switch(regs->int_no){
        case PAGE_FAULT:{
            // Gracefully handle a page fault
            sti
            //exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        case EXCEPTION_STACK_FAULT:{
            // Gracefully handle a stack fault (likely a stack overflow)
            sti
            //exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        case EXCEPTION_GENERAL_PROTECTION_FAULT:{
            // Gracefully handle a general protection fault
            sti
            //exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        // Other exceptions thrown by user applications...
        default:{
            sti
            //exit(SYSCALL_FAULT_DETECTED);
            break;
        }
    }
    return;
}

void InitISR(){
    for(size_t i = 0; i < NUM_ISRS; i++){
        isrs[i].index = i;
        isrs[i].stub = stubs[i];
        SetIDT(isrs[i].index, isrs[i].stub, 0x08, 0x8E);
    }

    for(size_t i = 0; i < 32; i++){
        InstallISR(i, ExceptionHandler);
    }

    //SetIDT(SYSCALL_INT, syscall_handler, 0x08, 0x8E);
    InstallISR(SYSCALL_INT, syscall_handler);
    idt.entries[SYSCALL_INT].selector = GDT_RING0_SEGMENT_POINTER(GDT_KERNEL_CODE);
    idt.entries[SYSCALL_INT].type = 0xEE;
}