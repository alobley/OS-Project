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
#include <syscalls.h>

void kb_handle_install(struct Registers* regs){
    // EBX contains the pointer to the callback function
    device_t* keyboardDevice = GetDeviceFromVfs("/dev/kb0");
    if(keyboardDevice != NULL){
        keyboard_t* keyboardDeviceInfo = (keyboard_t*)keyboardDevice->deviceInfo;
        keyboardDeviceInfo->AddCallback((KeyboardCallback)regs->ebx);
        regs->eax = SYSCALL_SUCCESS;
    }else{
        regs->eax = SYSCALL_INVALID_ARGUMENT;
    }
}

void kb_handle_remove(struct Registers* regs){
    device_t* keyboardDevice = GetDeviceFromVfs("/dev/kb0");
    if(keyboardDevice != NULL){
        keyboard_t* keyboardDeviceInfo = (keyboard_t*)keyboardDevice->deviceInfo;
        keyboardDeviceInfo->RemoveCallback((KeyboardCallback)regs->ebx);
        regs->eax = SYSCALL_SUCCESS;
    }else{
        regs->eax = SYSCALL_INVALID_ARGUMENT;
    }
}

void sys_write(struct Registers* regs){
    // EBX contains the file descriptor
    // ECX contains the pointer to the data to write
    // EDX contains the number of bytes to write

    // EAX contains the result
    // Write data to a file descriptor

    file_list_t* processFiles = GetCurrentProcess()->fileList;
    if(processFiles == NULL){
        regs->eax = FILE_NOT_FOUND;
        return;
    }
    //printk("Process files: 0x%x\n", processFiles);
    file_context_t* context = FindFile(processFiles, regs->ebx);
    //printk("File context: 0x%x\n", context);
    if(context == NULL){
        regs->eax = FILE_NOT_FOUND;
        return;
    }

    //printk("Writing to file...\n");

    if(context->node->readOnly == true){
        regs->eax = FILE_READ_ONLY;
        return;
    }

    if(context->node->isDevice){
        // If the node is a device, call the device's read function
        char* fullPath = GetFullPath(context->node);
        if(fullPath == NULL){
            //printk("Error: failed to get full path\n");
            regs->eax = FILE_NOT_FOUND;
            return;
        }
        // Find the device in the VFS
        device_t* device = GetDeviceFromVfs(fullPath);
        hfree(fullPath);
        if(device != NULL){
            int result = device->write(device, (char*)regs->ecx, regs->edx);
            if(result < 0){
                //printk("Read incomplete!\n");
                regs->eax = FILE_INVALID_OFFSET;
                return;
            }
            //printk("Read %d bytes from device %s\n", regs->edx, device->devName);
            regs->eax = FILE_WRITE_SUCCESS;
            return;
        }
        // If the device is not found, return an error
        //printk("Error: device not found\n");
        regs->eax = FILE_NOT_FOUND;
        return;
    }

    // Write the data to the file's buffer
    for(uint32_t i = 0; i < regs->edx; i++){
        if(context->node->offset + i >= context->node->size){
            // Stop writing if we go past the end of the buffer
            regs->eax = FILE_INVALID_OFFSET;
            return;
        }

        if(IsTTY((tty_t*)context->node->data)){
            // Just in case the file is a TTY and not actually a file (must change to device read/write)
            //printk("Writing to TTY...\n");
            context->node->offset = ttyNode->offset;
            TTYWrite(GetActiveTTY(), ((const char*)regs->ecx + i), regs->edx);
            context->node->offset = ttyNode->offset;
            return;
        }else{
            ((char*)context->node->data)[context->node->offset++] = ((char*)regs->ecx)[i];
        }
    }

    regs->eax = FILE_WRITE_SUCCESS;
}

void sys_read(struct Registers* regs){
    // SYS_READ
    // EBX contains the file descriptor
    // ECX contains the pointer to the buffer to read into (found using the open syscall)
    // EDX contains the number of bytes to read

    // Read data from a file descriptor

    file_list_t* processFiles = GetCurrentProcess()->fileList;
    file_context_t* context = FindFile(processFiles, regs->ebx);
    if(context == NULL){
        regs->eax = FILE_NOT_FOUND;
        return;
    }

    if(context->node->writeOnly == true){
        regs->eax = FILE_WRITE_ONLY;
        return;
    }

    if(IsTTY((tty_t*)context->node->data)){
        // Just in case the file is a TTY and not actually a file
        context->node->offset = ttyNode->offset;
        int result = TTYRead((tty_t*)context->node->data, (char*)regs->ecx, regs->edx);
        //printk((char*)regs->ecx);
        context->node->offset = ttyNode->offset;
        //printk("Offset: %u\n", context->node->offset);
        //printk("Data located at offset in node buffer: %s\n", (char*)context->node->data + (context->node->offset - result));

        //strcpy((char*)regs->ecx, (char*)context->node->data + (context->node->offset - result));
        regs->eax = FILE_READ_SUCCESS;
        return;
    }

    if(context->node->isDevice){
        // If the node is a device, call the device's read function
        char* fullPath = GetFullPath(context->node);
        if(fullPath == NULL){
            //printk("Error: failed to get full path\n");
            regs->eax = FILE_NOT_FOUND;
            return;
        }
        // Find the device in the VFS
        device_t* device = GetDeviceFromVfs(fullPath);
        hfree(fullPath);
        if(device != NULL){
            int result = device->read(device, (char*)regs->ecx, regs->edx);
            if(result < 0){
                //printk("Read incomplete!\n");
                regs->eax = FILE_READ_INCOMPLETE;
                return;
            }
            //printk("Read %d bytes from device %s\n", regs->edx, device->devName);
            regs->eax = FILE_READ_SUCCESS;
            return;
        }
        // If the device is not found, return an error
        //printk("Error: device not found\n");
        regs->eax = FILE_NOT_FOUND;
        return;
    }

    //printk("File is not a device!\n");

    // Read the data from the file's buffer
    for(size_t i = 0; i < regs->edx; i++){
        if(context->node->offset + i >= context->node->size){
            // Stop reading if we go past the end of the buffer
            regs->eax = FILE_READ_INCOMPLETE;
            return;
        }
        ((char*)regs->ecx)[i] = ((char*)context->node->data)[context->node->offset++];
    }

    regs->eax = FILE_READ_SUCCESS;
}

void sys_istat(struct Registers* regs){
    // SYS_ISTAT
    // EBX contains a pointer to the directory
    // ECX is the number of the node to read (its position in the directory)
    // EDX is a pointer to the buffer to read into
    // Read node information from the given directory (or the current directory if none given)

    struct Node_Data* node = (struct Node_Data*)regs->edx;

    vfs_node_t* directory = NULL;

    if(regs->ebx == 0){
        // If EBX is 0, use the current working directory
        directory = VfsFindNode(GetCurrentProcess()->workingDirectory);
    }else{
        // If the start of the string is a slash, it is an absolute path. Otherwise, construct the path
        char* path = (char*)regs->ebx;
        if(*path == '/'){
            directory = VfsFindNode(path);
        }else{
            char* fullPath = JoinPath(GetCurrentProcess()->workingDirectory, path);
            if(fullPath == NULL){
                //printk("Error: failed to join path\n");
                regs->eax = SYSCALL_TASKING_FAILURE;
                return;
            }
            directory = VfsFindNode(fullPath);
            hfree(fullPath);
        }
    }

    if(directory == NULL){
        //printk("Error: failed to find directory\n");
        regs->eax = FILE_NOT_FOUND;
        return;
    }

    vfs_node_t* vfsNode = directory->firstChild;
    if(vfsNode == NULL){
        //printk("Error: directory is empty\n");
        regs->eax = FILE_NOT_FOUND;
        return;
    }
    // Get the node at the given index
    for(uint32_t i = 0; i < regs->ecx; i++){
        vfsNode = vfsNode->next;
        if(vfsNode == NULL){
            regs->eax = FILE_NOT_FOUND;
            return;
        }
    }

    node->size = vfsNode->size;
    if(vfsNode->isDirectory){
        node->type = NODE_TYPE_DIRECTORY;
    }else if(vfsNode->isDevice){
        node->type = NODE_TYPE_DEVICE;
    }else{
        node->type = NODE_TYPE_FILE;
    }

    node->permissions = vfsNode->permissions;
    if(vfsNode->lock.owner != NULL){
        node->owner = vfsNode->lock.owner->pid;
    }else{
        node->owner = ROOT_UID;
    }

    strcpy(node->name, vfsNode->name);

    regs->eax = FILE_EXISTS;
}






void sys_exec(struct Registers* regs){
    // SYS_EXEC
    // EBX contains the pointer to the path of the executable
    // ECX contains the pointer to the arguments
    // EDX contains the pointer to the environment variables
    // ESI contains the number of arguments
    // Execute a new process (replaces current process)
    // Load the next process
    // Keep the PCB of the caller but modify it and replace it with what the new process needs

    // This is an early implementation, so right now it creates a new process and switches to it, then switches back to the caller when SYS_EXIT is called

    //printk("User ESP: 0x%x\n", regs->user_esp);
    //printk("Kernel ESP: 0x%x\n", regs->esp);

    if(strlen((char*)regs->ebx) == 0){
        regs->eax = SYSCALL_TASKING_FAILURE;
        return;
    }

    char* path = (char*)regs->ebx;

    volatile pcb_t* currentProcess = GetCurrentProcess();
    if(currentProcess == NULL){
        //printk("Error: failed to get current process\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        return;
    }

    memcpy(currentProcess->regs, regs, sizeof(struct Registers));

    vfs_node_t* current = VfsFindNode(currentProcess->workingDirectory);
    if(current == NULL){
        //printk("Error: current directory does not exist\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        return;
    }

    vfs_node_t* file = NULL;

    char* fullPath = JoinPath(currentProcess->workingDirectory, path);
    if(fullPath == NULL){
        //printk("Error: failed to join path\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        return;
    }

    if(*path != '/'){
        file = VfsFindNode(fullPath);
        if(file == NULL){
            //printk("Error: file %s does not exist\n", path);
            hfree(fullPath);
            regs->eax = SYSCALL_TASKING_FAILURE;
            return;
        }
    }else{
        file = VfsFindNode(path);
        if(file == NULL){
            regs->eax = SYSCALL_TASKING_FAILURE;
            hfree(fullPath);
            return;
        }
    }

    if(file->mountPoint == NULL){
        //printk("Error: file %s doesn't seem to be mounted\n", path);
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(fullPath);
        return;
    }

    if(file->mountPoint->device == NULL){
        //printk("Error: file at %s seems to have an invalid filesystem device\n", path);
        //printk("Mountpoint address: 0x%x\n", file->mountPoint);
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(fullPath);
        return;
    }

    //STOP

    MutexLock(&file->mountPoint->device->lock);
    if(file->mountPoint->device->read(file->mountPoint->device, fullPath, file->size) != DRIVER_SUCCESS){
        //printk("Error: failed to read file %s\n", path);
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(fullPath);
        return;
    }
    MutexUnlock(&file->mountPoint->device->lock);
    
    //hfree(fullPath);

    // Get and parse the ELF header
    Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)file->data;

    //printk("ELF Magic: 0x%x\n", elfHeader->magic);
    if(elfHeader->magic != ELF_MAGIC){
        //printk("Error: ELF magic number invalid!\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(file->data);
        return;
    }

    if(elfHeader->type != ELF_TYPE_EXEC && elfHeader->type != ELF_TYPE_SHARED){
        //printk("Error: ELF type invalid!\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(file->data);
        return;
    }

    if(elfHeader->instructionSet != ISA_X86_32){
        //printk("Error: Invalid ISA!\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        hfree(file->data);
        return;
    }

    volatile pcb_t* newProcess = CreateProcess(NULL, strdup(file->name), strdup(currentProcess->workingDirectory), currentProcess->owner, true, false, true, NORMAL, PROCESS_DEFAULT_TIME_SLICE, currentProcess);
    if(newProcess == NULL){
        //printk("Error: failed to create new process\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        return;
    }

    newProcess->executablePath = fullPath;
    hfree(fullPath);

    // TODO: Allocate a new page directory for the new process

    // Copy the ELF data to the new process's memory
    Elf32_Phdr* programHeader = (Elf32_Phdr*)((uintptr_t)elfHeader + elfHeader->programHeaderOffset);
    virtaddr_t entryPoint = (virtaddr_t)elfHeader->entry;
    if(entryPoint < USER_MEM_START || entryPoint > USER_MEM_END){
        //printk("Error: entry point is outside of user memory range!\n");
        regs->eax = SYSCALL_TASKING_FAILURE;
        DestroyProcess(newProcess);
        hfree(file->data);
        return;
    }

    virtaddr_t stackBase = 0;
    newProcess->EntryPoint = (void*)entryPoint;
    for(uint16_t i = 0; i < elfHeader->programHeaderEntryCount; i++){
        if(programHeader[i].type == PT_LOAD || programHeader[i].type == PT_DYNAMIC){
            virtaddr_t alignedAddress = programHeader[i].virtualAddress & 0xFFFFF000;
            size_t offset = programHeader[i].virtualAddress & 0x00000FFF;
            for(size_t j = 0; j < programHeader[i].memorySize; j += PAGE_SIZE){
                //printk("Allocating virtual address 0x%x\n", programHeader[i].virtualAddress + i);
                // Align the address to page
                //printk("Paging virtual address 0x%x\n", alignedAddress + j);
                //printk("Real address: 0x%x\n", programHeader[i].virtualAddress + j);
                page_result_t result = user_palloc(alignedAddress + j, PAGE_PRESENT | PAGE_RW | PAGE_USER);
                if(result != PAGE_OK){
                    // Only if the page is not aquired, since it may re-page the same area. If that's the case, just copy the data
                    //printk("Error: failed to page memory\n");
                    regs->eax = SYSCALL_TASKING_FAILURE;
                    return;
                }
            }
            memset((void*)(programHeader[i].virtualAddress), 0, programHeader[i].memorySize);
            memcpy((void*)(programHeader[i].virtualAddress), (void*)((uintptr_t)elfHeader + programHeader[i].offset), programHeader[i].fileSize);
            newProcess->memSize += programHeader[i].memorySize;
            stackBase = programHeader[i].virtualAddress + programHeader[i].memorySize;              // Ensure the stack is at the end of the memory
        }
    }

    stackBase += PAGE_SIZE; // Add a page of padding to the stack base
    newProcess->stackBase = (stackBase & 0xFFFFF000) + PAGE_SIZE; // Align the stack base to the next page
    newProcess->stackTop = newProcess->stackBase + (PAGE_SIZE * 2); // Set the stack top to the next page

    // Page the stack
    for(size_t i = 0; i < 2; i++){
        //printk("Allocating virtual address 0x%x\n", newProcess->stackBase + (i * PAGE_SIZE));
        page_result_t result = palloc(newProcess->stackBase + (i * PAGE_SIZE), PAGE_PRESENT | PAGE_RW | PAGE_USER);
        //printk("Result: %d\n", result);
        if(result != PAGE_OK){
            //printk("Error: failed to page stack\n");
            regs->eax = SYSCALL_TASKING_FAILURE;
            return;
        }
    }

    SetCurrentProcess(newProcess);

    // Be very nice and (definitely not conspicuously) clear the registers for the new process :)
    regs->eax = 0;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;

    // Enter ring 3 and execute the program (iret method)
    regs->eflags.iopl = 0;
    regs->eflags.interruptEnable = 1;
    regs->cs = GDT_RING3_SEGMENT_POINTER(GDT_USER_CODE);
    regs->ds = GDT_RING3_SEGMENT_POINTER(GDT_USER_DATA);
    regs->es = GDT_RING3_SEGMENT_POINTER(GDT_USER_DATA);
    regs->fs = GDT_RING3_SEGMENT_POINTER(GDT_USER_DATA);
    regs->gs = GDT_RING3_SEGMENT_POINTER(GDT_USER_DATA);
    regs->ss = GDT_RING3_SEGMENT_POINTER(GDT_USER_DATA);

    regs->eip = (uintptr_t)newProcess->EntryPoint;
    //printk("Stack top: 0x%x\n", newProcess->stackTop);
    //printk("Stack base: 0x%x\n", newProcess->stackBase);
    regs->user_esp = newProcess->stackTop;
    regs->ebp = newProcess->stackBase;

    memcpy(newProcess->regs, regs, sizeof(struct Registers));
}