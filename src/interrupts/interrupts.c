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

// An array of PCBs that are responsible for interrupts
pcb_t* interruptOwners[256];

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
    currentProcess->inSyscall = true;
    switch(regs->eax){
        case SYS_DBG:{
            printk("Syscall debug!\n");
            regs->eax = 0xDEADBEEF;
            break;
        }

        case SYS_WRITE:{
            // SYS_WRITE - write data to a file descriptor
            // EBX = fd
            // ECX = Pointer to the data buffer
            // EDX = Size of the data buffer in bytes

            // Returns: (ssize_t) bytes written

            // TODO: change this to page remapping or something so the kernel doesn't have to relay data (that's super inefficient and a security risk)

            if(regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END){
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

                if(node == NULL || node->data == NULL){
                    // We somehow got a NULL file (they are read upon the first SYS_OPEN or contain device data)
                    regs->eax = SYSCALL_FAILURE;
                    return;
                }

                // Lock the node (resizing can clobber pointers and device use should be done one process at a time)
                while(MLock(&node->lock, currentProcess) == SEM_LOCKED){
                    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                }

                if(node->flags & NODE_FLAG_DEVICE){
                    device_t* device = node->device;

                    // Lock the device's mutex as well - although the VFS node's mutex was probably enough.
                    while(MLock(&device->lock, currentProcess) == SEM_LOCKED){
                        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                    }

                    void* ptr = halloc(regs->edx);
                    if(ptr == NULL){
                        regs->eax = SYSCALL_OUT_OF_MEMORY;
                        MUnlock(&node->lock);
                        return;
                    }
                    memset(ptr, 0, regs->edx);
                    memcpy(ptr, (void*)regs->ecx, regs->edx);

                    if(device == NULL || device->driver == NULL || device->ops.write == NULL){
                        regs->eax = SYSCALL_INVALID_ARGUMENT;
                        hfree(ptr);
                        MUnlock(&node->lock);
                        MUnlock(&device->lock);
                        return;
                    }

                    if(device->kernel_buffer != NULL){
                        // Just in case the last use of this was not freed
                        hfree(device->kernel_buffer);
                        device->kernel_buffer = NULL;
                    }

                    // The driver is busy and somehow we made it here
                    if(device->driver->owner->using != NULL){
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        MUnlock(&device->lock);
                        MUnlock(&node->lock);
                        regs->eax = SYSCALL_ACCESS_DENIED;
                        return;
                    }

                    device->kernel_buffer = ptr;                                    // Set a pointer for deallocation when the driver yields
                    device->driver->owner->using = device;

                    if(device->driver->in_kernel){
                        // The driver is part of the kernel, do a simple function call
                        regs->eax = device->ops.write(ptr, regs->edx);
                        memcpy((void*)regs->ecx, ptr, regs->edx);
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        device->driver->owner->using = NULL;
                        MUnlock(&device->lock);
                        MUnlock(&node->lock);
                        return;
                    }

                    size_t size = regs->edx;
                    device_id_t deviceID = device->id;
                    pcb_t* caller = currentProcess;
                    device->caller = caller;
                    currentProcess->state = WAITING;
                    
                    // Switch to the driver for the device - will return when the original process is switched back to.
                    ContextSwitch(regs, device->driver->owner);

                    currentProcess = GetCurrentProcess();

                    // Allow for easy re-retrieval later
                    currentProcess->using = device;

                    // Map the buffer into the driver's memory - destroying other kernel buffer regions is set to true 
                    // because this is where the region from previous uses is freed.
                    int result = AddMemoryRegion(currentProcess, regs->edx, PAGE_SIZE, MEMREGION_KERNELSHARE, NULL, NULL, true);
                    if(result == STANDARD_FAILURE){
                        ContextSwitch(regs, caller);                    // Switch back to the caller
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        device->driver->owner->using = NULL;
                        caller->state = RUNNING;
                        device->caller = NULL;
                        regs->eax = SYSCALL_FAILURE;
                        return;
                    }else if(result == EMERGENCY_NO_MEMORY){
                        ContextSwitch(regs, caller);                    // Return to the caller
                        UnregisterDriver(device->driver);               // Remove the offending driver
                        DestroyProcess(currentProcess);                 // The driver should not lead to a result like this
                        hfree(ptr);
                        device->caller = NULL;
                        device->kernel_buffer = NULL;
                        device->driver->owner->using = NULL;
                        caller->state = RUNNING;

                        regs->eax = SYSCALL_OUT_OF_MEMORY;
                        return;
                    }

                    void* userptr = currentProcess->regions[currentProcess->numRegions - 1]->start;
                    memcpy(ptr, userptr, regs->edx);

                    // We don't need the kernel buffer anymore
                    hfree(ptr);

                    // Reset the driver's stack (the stack will just grow larger and larger every time if I don't)
                    regs->user_esp = (uintptr_t)currentProcess->regions[REGION_USER_STACK_INDEX]->end;

                    // Push the arguments in reverse order onto the user-mode stack - write(ptr, size)
                    UserPush(regs->user_esp, size);
                    UserPush(regs->user_esp, userptr);
                    regs->eip = device->ops.write;

                    // The driver will be expected to put a return code or the number of bytes read into EAX before yielding again (the C API will abstract this)

                    // The process can be retrieved again from the owner of the device's mutex lock, and the PCB of the driver contains a pointer to the device being used

                    break;
                }

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

                // Unlock the node
                MUnlock(&node->lock);
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

            if(regs->ecx == 0 || regs->ecx < USER_MEM_START || regs->ecx > USER_MEM_END){
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

                    while(MLock(device->lock, currentProcess) == SEM_LOCKED){
                        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                    }

                    // The kernel buffer that will be copied to
                    void* ptr = halloc(regs->edx);
                    if(ptr == NULL){
                        regs->eax = SYSCALL_OUT_OF_MEMORY;
                        return;
                    }
                    memset(ptr, 0, regs->edx);
                    memcpy(ptr, (void*)regs->ecx, regs->edx);

                    if(device == NULL || device->driver == NULL || device->ops.read == NULL || (node->flags & NODE_FLAG_RO)){
                        regs->eax = SYSCALL_INVALID_ARGUMENT;
                        MUnlock(&node->lock);
                        MUnlock(&device->lock);
                        return;
                    }

                    if(device->kernel_buffer != NULL){
                        // Just in case the last use of this was not freed
                        hfree(device->kernel_buffer);
                        device->kernel_buffer = NULL;
                    }

                    // The driver is busy and we somehow made it here
                    if(device->driver->owner->using != NULL){
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        MUnlock(&device->lock);
                        MUnlock(&node->lock);
                        regs->eax = SYSCALL_ACCESS_DENIED;
                        return;
                    }

                    device->kernel_buffer = ptr;                                    // Set a pointer for deallocation when the driver yields

                    if(device->driver->in_kernel){
                        // The driver is part of the kernel, do a simple function call
                        regs->eax = device->ops.read(ptr, regs->edx);
                        memcpy((void*)regs->ecx, ptr, regs->edx);
                        hfree(ptr);
                        device->driver->owner->using = NULL;
                        device->kernel_buffer = NULL;
                        MUnlock(&device->lock);
                        MUnlock(&node->lock);
                        return;
                    }

                    size_t size = regs->edx;
                    device_id_t deviceID = device->id;
                    pcb_t* caller = currentProcess;
                    currentProcess->state = WAITING;
                    device->caller = caller;
                    
                    // Switch to the driver for the device - will return when the original process is switched back to.
                    ContextSwitch(regs, device->driver->owner);

                    currentProcess = GetCurrentProcess();

                    // Allow for easy re-retrieval later
                    currentProcess->using = device;

                    // Map the buffer into the driver's memory - destroying other kernel buffer regions is set to true 
                    // because this is where the region from previous uses is freed.
                    int result = AddMemoryRegion(currentProcess, regs->edx, PAGE_SIZE, MEMREGION_KERNELSHARE, NULL, NULL, true);
                    if(result == STANDARD_FAILURE){
                        MUnlock(&node->lock);
                        MUnlock(&device->lock);
                        ContextSwitch(regs, caller);                    // Switch back to the caller
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        device->driver->owner->using = NULL;
                        device->caller = NULL;
                        caller->state = RUNNING;
                        regs->eax = SYSCALL_FAILURE;
                        return;
                    }else if(result == EMERGENCY_NO_MEMORY){
                        ContextSwitch(regs, caller);                    // Return to the caller
                        UnregisterDriver(device->driver);               // Remove the offending driver
                        DestroyProcess(currentProcess);                 // The driver should not lead to a result like this
                        MUnlock(&node->lock);
                        MUnlock(&device->lock);
                        hfree(ptr);
                        device->kernel_buffer = NULL;
                        device->driver->owner->using = NULL;
                        device->caller = NULL;
                        caller->state = RUNNING;
                        regs->eax = SYSCALL_OUT_OF_MEMORY;
                        return;
                    }

                    void* userptr = currentProcess->regions[currentProcess->numRegions - 1]->start;
                    memcpy(ptr, userptr, regs->edx);

                    // We don't need the kernel buffer anymore
                    hfree(ptr);

                    // Push the arguments in reverse order onto the user-mode stack - read(ptr, size)
                    regs->user_esp = (uintptr_t)currentProcess->regions[REGION_USER_STACK_INDEX]->end;
                    UserPush(regs->user_esp, size);
                    UserPush(regs->user_esp, userptr);
                    regs->eip = device->ops.read;

                    // The driver will be expected to put a return code into EAX before yielding again (the C API will abstract this)

                    // The process can be retrieved again from the owner of the device's mutex lock, and the PCB of the driver contains a pointer to the device being used

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

            if(regs->ebx == 0 || regs->ebx < USER_MEM_START || regs->ebx > USER_MEM_END){
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
                ContextSwitch(regs, GetNextProcess());          // Switch to next process
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

            if(node->data == NULL && node->mountPoint != NULL && !nodeMade && !(node->flags & NODE_FLAG_DIRECTORY) && !(node->flags & NODE_FLAG_DEVICE)){
                // Switch to the driver and load the data
                device_t* device = node->mountPoint->fsDevice;
                driver_t* driver = node->mountPoint->fsDriver;

                while(MLock(&device->lock, currentProcess) == SEM_LOCKED){
                    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
                }
                pcb_t* caller = currentProcess;
                caller->state = WAITING;
                device->caller = caller;

                if(driver->owner->using != NULL){
                    // Driver is busy and we somehow made it here
                    regs->eax = SYSCALL_FAILURE;
                    caller->state = RUNNING;
                    return;
                }

                // The driver is expected to have accurately assigned the file size
                device->kernel_buffer = halloc(node->size);
                if(device->kernel_buffer == NULL){
                    regs->eax = SYSCALL_OUT_OF_MEMORY;
                    caller->state = RUNNING;
                    return;
                }

                if(device->driver->in_kernel){
                    // The driver is part of the kernel, do a simple function call
                    regs->eax = device->ops.read(device->kernel_buffer, regs->edx);
                    memcpy((void*)regs->ecx, device->kernel_buffer, regs->edx);
                    hfree(device->kernel_buffer);
                    device->driver->owner->using = NULL;
                    device->kernel_buffer = NULL;
                    device->caller = NULL;
                    MUnlock(&device->lock);
                    caller->state = RUNNING;
                    return;
                }

                ContextSwitch(regs, node->mountPoint->fsDriver->owner);
                currentProcess = GetCurrentProcess();

                // Map the buffer into the driver's memory - destroying other kernel buffer regions is set to true 
                // because this is where the region from previous uses is freed.
                int result = AddMemoryRegion(currentProcess, regs->edx, PAGE_SIZE, MEMREGION_KERNELSHARE, NULL, NULL, true);
                if(result == STANDARD_FAILURE){
                    ContextSwitch(regs, caller);                    // Switch back to the caller
                    device->caller = NULL;
                    MUnlock(&device->lock);
                    regs->eax = SYSCALL_FAILURE;
                    caller->state = RUNNING;
                    return;
                }else if(result == EMERGENCY_NO_MEMORY){
                    ContextSwitch(regs, caller);                    // Return to the caller
                    device->caller = NULL;
                    MUnlock(&device->lock);
                    UnregisterDriver(device->driver);               // Remove the offending driver
                    DestroyProcess(currentProcess);                 // The driver should not lead to a result like this
                    caller->state = RUNNING;
                    regs->eax = SYSCALL_OUT_OF_MEMORY;
                    return;
                }

                device->driver->owner->using = device;

                // Reset the driver's stack (the stack will just grow larger and larger every time if I don't)
                regs->user_esp = (uintptr_t)currentProcess->regions[REGION_USER_STACK_INDEX]->end;

                // Push the values onto the driver's stack
                UserPush(regs->user_esp, node->size);
                UserPush(regs->user_esp, currentProcess->regions[currentProcess->numRegions - 1]->start);
                regs->eip = device->ops.read;
            }

            return;
        }

        case SYS_CLOSE:{
            // SYS_CLOSE - Close a file descriptor
            // EBX = fd

            file_table_t* table = currentProcess->fileTable;

            if(table == NULL || regs->ebx > table->arrSize){
                regs->eax = SYSCALL_INVALID_ARGUMENT;
                return;
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
                if(context->node->size + (ssize_t)regs->ecx < 0 || (ssize_t)regs->ecx > 0){
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

            regs->eax = SYSCALL_NOT_IMPLEMENTED;
            break;
        }

        case SYS_SYNC:{
            // SYS_SYNC - Write the buffers of all files to the disk
            // No arguments(?)

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

        // And 94 more... oh man.

        default:{
            // Invalid system call!
            regs->eax = SYSCALL_NOT_FOUND;
            break;
        }
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
            exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        case EXCEPTION_STACK_FAULT:{
            // Gracefully handle a stack fault (likely a stack overflow)
            sti
            exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        case EXCEPTION_GENERAL_PROTECTION_FAULT:{
            // Gracefully handle a general protection fault
            sti
            exit(SYSCALL_FAULT_DETECTED);
            break;
        }
        // Other exceptions thrown by user applications...
        default:{
            sti
            exit(SYSCALL_FAULT_DETECTED);
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