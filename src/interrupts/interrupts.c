#include <interrupts.h>
#include <console.h>
#include <keyboard.h>
#include <multitasking.h>
#include <kernel.h>
#include <time.h>
#include <devices.h>
#include <tty.h>
#include <acpi.h>

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

static void regdump(struct Registers* regs){
    printk("EAX: 0x%x\n", regs->eax);
    printk("EBX: 0x%x\n", regs->ebx);
    printk("ECX: 0x%x\n", regs->ecx);
    printk("EDX: 0x%x\n", regs->edx);
    printk("ESI: 0x%x\n", regs->esi);
    printk("EDI: 0x%x\n", regs->edi);
    printk("EBP: 0x%x\n", regs->ebp);
    printk("ESP: 0x%x\n", regs->esp);
    printk("EIP: 0x%x\n", regs->eip);
    printk("CS: 0x%x\n", regs->cs);
    printk("DS: 0x%x\n", regs->ds);
    printk("ES: 0x%x\n", regs->es);
    printk("FS: 0x%x\n", regs->fs);
    printk("SS: 0x%x\n", regs->ss);
    printk("EFLAGS: 0x%x\n", regs->eflags);

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

bool CheckPrivelige(){
    if(GetCurrentProcess() == NULL){
        // Is part of the kernel
        return true;
    }
    return GetCurrentProcess()->owner == ROOT_UID;
}

// System call handler
// This is what is processed when you perform an ABI call (int 0x30). Work in progress.
// NOTE: The keyboard handler needs to be updated for multitasking in the future
HOT void syscall_handler(struct Registers *regs){
    int result;
    if(!CheckPrivelige() && regs->eax >= SYS_MODULE_LOAD){
        // Better to check once at the beginning rather than checking every time
        // Why not just return DRIVER_ACCESS_DENIED?
        printk("Unpriveliged Application requesting system resources. Killing process.\n");
        // Log the error
        
        // Kill the process
        SwitchProcess(true, regs);
        return;
    }
    switch(regs->eax){
        case SYS_DBG: {
            // SYS_DBG
            printk("Syscall Debug!\n");
            regs->eax = 0xFEEDBEEF;
            break;
        }
        case SYS_INSTALL_KBD_HANDLE: {
            // SYS_INSTALL_KBD_HANDLE
            // EBX contains the pointer to the callback function
            // Installs a keyboard callback on the first keyboard in the system. For more keyboards developers will have to open the keyboard device and install a callback.
            device_t* keyboardDevice = GetDeviceFromVfs("/dev/kb0");
            if(keyboardDevice != NULL){
                keyboard_t* keyboardDeviceInfo = (keyboard_t*)keyboardDevice->deviceInfo;
                keyboardDeviceInfo->AddCallback((KeyboardCallback)regs->ebx);
                regs->eax = SYSCALL_SUCCESS;
            }else{
                regs->eax = SYSCALL_INVALID_ARGUMENT;
            }
            break;
        }
        case SYS_REMOVE_KBD_HANDLE: {
            // SYS_REMOVE_KBD_HANDLE
            device_t* keyboardDevice = GetDeviceFromVfs("/dev/kb0");
            if(keyboardDevice != NULL){
                keyboard_t* keyboardDeviceInfo = (keyboard_t*)keyboardDevice->deviceInfo;
                keyboardDeviceInfo->RemoveCallback((KeyboardCallback)regs->ebx);
                regs->eax = STANDARD_SUCCESS;
            }else{
                regs->eax = STANDARD_FAILURE;
            }
            break;
        }
        case SYS_WRITE:{
            // SYS_WRITE
            // EBX contains the file descriptor
            // ECX contains the pointer to the data to write
            // EDX contains the number of bytes to write

            // EAX contains the result
            // Write data to a file descriptor

            file_list_t* processFiles = GetCurrentProcess()->fileList;
            file_context_t* context = FindFile(processFiles, regs->ebx);
            if(context == NULL){
                regs->eax = FILE_NOT_FOUND;
                break;
            }

            if(context->node->readOnly == true){
                regs->eax = FILE_READ_ONLY;
                break;
            }

            // Write the data to the file's buffer
            for(uint32_t i = 0; i < regs->edx; i++){
                if(context->node->offset + i >= context->node->size){
                    // Stop writing if we go past the end of the buffer
                    if(context->node->isResizeable == false){
                        regs->eax = FILE_NOT_RESIZEABLE;
                        break;
                    }else{
                        context->node->data = rehalloc(context->node->data, context->node->size * 2);
                        context->node->size *= 2;
                    }
                }

                if(IsTTY((tty_t*)context->node->data)){
                    // Just in case the file is a TTY and not actually a file
                    context->node->offset = ttyNode->offset;
                    TTYWrite(GetActiveTTY(), ((const char*)regs->ecx + i), regs->edx);
                    context->node->offset = ttyNode->offset;
                    break;
                }else{
                    ((char*)context->node->data)[context->node->offset++] = ((char*)regs->ecx)[i];
                }
            }

            regs->eax = FILE_WRITE_SUCCESS;
            break;
        }
        case SYS_READ:{
            // SYS_READ
            // EBX contains the file descriptor
            // ECX contains the pointer to the buffer to read into (found using the open syscall)
            // EDX contains the number of bytes to read

            // Read data from a file descriptor

            file_list_t* processFiles = GetCurrentProcess()->fileList;
            file_context_t* context = FindFile(processFiles, regs->ebx);
            if(context == NULL){
                regs->eax = FILE_NOT_FOUND;
                break;
            }

            if(context->node->writeOnly == true){
                regs->eax = FILE_WRITE_ONLY;
                break;
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
                break;
            }

            // Read the data from the file's buffer
            for(size_t i = 0; i < regs->edx; i++){
                if(context->node->offset + i >= context->node->size){
                    // Stop reading if we go past the end of the buffer
                    regs->eax = FILE_READ_INCOMPLETE;
                    break;
                }
                ((char*)regs->ecx)[i] = ((char*)context->node->data)[context->node->offset++];
            }

            regs->eax = FILE_READ_SUCCESS;
            break;
        }
        case SYS_EXIT:
            // SYS_EXIT
            // EBX contains the exit code
            // Exit a process

            // Tell thhe parent process that the child has exited
            // Perform a context switch and destroy the process

            break;
        case SYS_FORK:
            // SYS_FORK
            // Fork a process
            // Create a new PCB and copy everything about the process except the page directory
            // Allocate a new page directory for the new process
            // Switch to the new PCB
            // Return the PID of the new process to the parent
            // Return 0 to the child
            break;
        case SYS_EXEC:
            // SYS_EXEC
            // EBX contains the pointer to the path of the executable
            // ECX contains the pointer to the arguments
            // EDX contains the pointer to the environment variables
            // ESI contains the number of arguments
            // Execute a new process (replaces current process)
            // Load the next process
            // Keep the PCB of the caller but modify it and replace it with what the new process needs
            break;
        case SYS_WAIT_PID:
            // SYS_WAIT
            // EBX contains the PID to wait for
            // Wait for a process to exit
            // Set the calling process to waiting
            // When the child exits, return the exit code
            break;
        case SYS_GET_PID:
            // SYS_GET_PID
            // Get the PID of the current process
            regs->eax = (uint32_t)GetCurrentProcess()->pid;
            break;
        case SYS_GET_PPID:
            // SYS_GET_PPID
            // Get the PID of the parent process
            regs->eax = (uint32_t)GetCurrentProcess()->parent->pid;
            break;
        case SYS_GETCWD:
            // SYS_GETCWD
            // Gets own process PCB
            // EBX contains the buffer to write the PCB to.
            // ECX contains the length of the buffer (could be a security risk - a malicious application can simply make this too big. But this is a getter and not a setter.)

            // How can I get the length I need? Do I just have to deal with this?
            if(regs->ebx == 0 || regs->ecx == 0){
                regs->eax = STANDARD_FAILURE;
            }else{
                if(regs->ecx >= strlen(GetCurrentProcess()->workingDirectory)){
                    strcpy((char*)regs->ebx, GetCurrentProcess()->workingDirectory);
                    regs->eax = STANDARD_SUCCESS;
                }else{
                    // Buffer is too small
                    regs->eax = STANDARD_FAILURE;
                }
            }
            break;
        case SYS_CHDIR:
            // SYS_CHDIR
            // Change the current working directory
            // EBX contains the pointer to the new directory
            // Change the current working directory of the process

            pcb_t* currentProcess = GetCurrentProcess();
            char* dir = (char*)regs->ebx;
            if(dir == NULL || strlen(dir) == 0){
                // No directory specified
                regs->eax = STANDARD_FAILURE;
                break;
            }

            vfs_node_t* current = VfsFindNode(currentProcess->workingDirectory);
            if(current == NULL){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            if(!current->isDirectory){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            hfree(currentProcess->workingDirectory);
            if(strcmp(dir, "..") == 0 && current->parent != NULL){
                currentProcess->workingDirectory = GetFullPath(current->parent);
                regs->eax = STANDARD_SUCCESS;
                break;
            }else if(strcmp(dir, ".") == 0){
                regs->eax = STANDARD_SUCCESS;
                break;
            }
            char* fullPath = JoinPath(currentProcess->workingDirectory, dir);
            vfs_node_t* newDir = VfsFindNode(fullPath);
            if(newDir != NULL && newDir->isDirectory){
                currentProcess->workingDirectory = GetFullPath(newDir);
                hfree(fullPath);
                regs->eax = STANDARD_SUCCESS;
            }else{
                hfree(fullPath);
                regs->eax = STANDARD_FAILURE;
            }
            break;
        case SYS_OPEN: {
            // SYS_OPEN
            // Open a file from the VFS
            // EBX contains the pointer to the path of the file

            // EAX will contain the result
            // EBX will contain the file descriptor
            break;
        }
        case SYS_CLOSE: {
            // SYS_CLOSE
            // Close a file
            // EBX contains the file descriptor to close
            
            break;
        }
        case SYS_SEEK:
            // SYS_SEEK
            // EBX contains the file descriptor
            // ECX contains the offset to seek to
            // EDX contains the whence (0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END)
            // Seek to a position in a file
            // Will return the new offset or 0xFFFFFFFF (-1) if the seek failed
            file_context_t* context = FindFile(GetCurrentProcess()->fileList, regs->ebx);
            if(context == NULL){
                regs->eax = FILE_INVALID_OFFSET;
                break;
            }
            switch(regs->edx){
                case SEEK_SET:
                    if(regs->ecx > context->node->size){
                        regs->eax = FILE_INVALID_OFFSET;
                    }
                    context->node->offset = regs->ecx;
                    regs->eax = context->node->offset;
                    break;
                case SEEK_CUR:
                    if(context->node->offset += regs->ecx > context->node->size){
                        regs->eax = FILE_INVALID_OFFSET;
                    }else{
                        context->node->offset += regs->ecx;
                        context->node->offset;
                    }
                    break;
                case SEEK_END:
                    context->node->offset = context->node->size;
                    regs->eax = context->node->offset;
                    break;
            }
            break;
        case SYS_SLEEP:
            // SYS_SLEEP
            // EBX contains the low 32 bits of the amount of time to sleep in milliseconds
            // ECX contains the high 32 bits of the amount of time to sleep in milliseconds
            // Sleep for a certain amount of time

            // The sleep function will need to be implemented in the scheduler. Busy waiting can be fine because the scheduler will preemptively multitask to something else
            busysleep((uint64_t)regs->ebx | ((uint64_t)regs->ecx << 32));
            break;
        case SYS_GET_TIME:
            // SYS_GET_TIME
            // Get the time struct from the kernel
            // EBX contains the pointer to the time struct to copy the time into
            if(regs->ebx != 0){
                memcpy((datetime_t*)regs->ebx, &currentTime, sizeof(datetime_t));
                regs->eax = STANDARD_SUCCESS;
            }else{
                regs->eax = STANDARD_FAILURE;
            }
            break;
        case SYS_KILL:
            // SYS_KILL
            // EBX: contains the PID of the process to kill (must either be executed by root or the owner of the process)
            // Kill a process

            // Find the process by its PID
            // Kill the calling process
            break;
        case SYS_YIELD:
            // SYS_YIELD
            // Context switching (The kernel can also call this to preemptively multitask)

            // Yield the CPU
            SwitchProcess(false, regs);
            break;
        case SYS_PIPE:
            // Create a new file for reading and another one for writing but don't add it to the VFS or write it to the disk. It's specifically for inter-process communication.
            break;
        case SYS_REPDUP:
            // SYS_REPDUP
            // Replace and duplicate a file descriptor
            // EBX contains the old file descriptor
            // ECX contains the new file descriptor
            // Replace the old file descriptor with the new one
            break;
        case SYS_MMAP:
            // SYS_MMAP
            // Map memory pages to a given virtual address (should this and munmap be priveliged?)
            // EBX contains the pointer to the virtual address to map to
            // ECX contains the size of the memory to map
            // EDX contains the flags (read, write, execute)

            // Make sure to check that the memory to page is allowed to be used by the process, is not already paged, and is page-aligned

            // DON'T FORGET TO OR EAX WITH THE ALLOWED FLAGS - THIS IS A SECURITY CONCERN
            break;
        case SYS_MUNMAP:
            // SYS_MUNMAP
            // Unmap memory pages from a given virtual address - thus allowing the system more free memory to use
            // EBX contains the pointer to the virtual address to unmap from
            // ECX contains the size of the memory to unmap

            // Make sure the passed address to unmap is paged and page-aligned (pretty sure pfree already does that - need to check)
            break;
        case SYS_BRK:
            // SYS_BRK
            // Change the heap size
            // EBX contains the new size of the heap
            // Note: new heap allocations should be a multiple of PAGE_SIZE


            // Change the heap size of the process by paging some memory to the area
            break;
        case SYS_MPROTECT:
            // SYS_MPROTECT
            // Change the protection of memory pages
            // EBX contains the pointer to the virtual address to change the protection of
            // ECX contains the size of the memory to change the protection of
            // EDX contains the new protection flags (read, write, execute)

            // Make sure the passed address to change is paged and page-aligned and also is allowed to be changed by this process (don't want to change up the kernel's memory)
            break;
        case SYS_REGDUMP:
            // SYS_REGDUMP
            // Dump the CPU's registers to the console for debugging reasons
            regdump(regs);
            break;
        case SYS_SYSINFO:
            // SYS_SYSINFO
            // EBX contains the pointer to the sysinfo struct to copy the info into
            // Get system information
            if(regs->ebx == 0){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            struct sysinfo* info = (struct sysinfo*)regs->ebx;
            info->uptime = GetTicks() / 1000; // Convert to seconds
            info->totalMemory = totalMemSize;
            info->usedMemory = totalPages * PAGE_SIZE;
            info->freeMemory = (totalMemSize - (totalPages * PAGE_SIZE));
            info->numProcesses = 0; // TODO: Implement this
            memcpy(&info->kernelVersion, &kernelVersion, sizeof(version_t));
            memcpy(info->kernelRelease, kernelRelease, strlen(kernelRelease) + 1);
            info->acpiSupported = acpiInfo.exists;
            uint32_t eax = 0;
            uint32_t others[4] = {0};
            cpuid(eax, others[0], others[1], others[2]);
            memcpy(info->cpuID, others, sizeof(others));
            regs->eax = STANDARD_SUCCESS;
            break;
        case SYS_OPEN_DEVICE: {
            // SYS_OPEN_DEVICE
            // Open a device from the VFS
            // EBX contains the pointer to the path of the device
            // ECX contains a pointer to the buffer to copy into
            // Return the user device struct
            vfs_node_t* node = VfsFindNode((char*)regs->ebx);
            if(node == NULL || node->data == NULL || node->isDirectory){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            MutexLock(&node->lock);

            device_t* device = GetDeviceFromVfs((char*)regs->ebx);
            if(device == NULL){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            memcpy((user_device_t*)regs->ecx, device->userDevice, sizeof(user_device_t));
            regs->eax = STANDARD_SUCCESS;
            break;
        }
        case SYS_CLOSE_DEVICE: {
            // SYS_CLOSE_DEVICE
            // Close a device
            // EBX contains the path of the device to close
            vfs_node_t* node = VfsFindNode((char*)regs->ebx);
            if(node == NULL || node->data == NULL || node->isDirectory){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            MutexUnlock(&node->lock);
            regs->eax = STANDARD_SUCCESS;

            break;
        }

        // Do I need to also switch the registers to the driver's PCB? Is it fine to keep them at the kernel?
        case SYS_DEVICE_READ: {
            // SYS_DEVICE_READ
            // EBX contains device ID of the device to read from
            // ECX contains the pointer to the buffer to read into
            // EDX contains the size of the buffer
            device_t* device = GetDeviceByID(regs->ebx);
            if(device == NULL){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            MutexLock(&device->lock);                                                                       // Lock the device
            pcb_t* current = GetCurrentProcess();                                                           // Save the calling process
            SetCurrentProcess(device->driver->driverProcess);                                               // Switch to the driver
            regs->eax = device->read(device, (char*)regs->ecx, regs->edx);                                  // Call the device's read function
            SetCurrentProcess(current);                                                                     // Switch back to the caller
            MutexUnlock(&device->lock);                                                                     // Unlock the device
            break;
        }
        case SYS_DEVICE_WRITE: {
            // SYS_DEVICE_WRITE
            // EBX contains device ID of the device to write to
            // ECX contains the pointer to the buffer to write from
            // EDX contains the size of the buffer
            device_t* device = GetDeviceByID(regs->ebx);
            if(device == NULL){
                regs->eax = STANDARD_FAILURE;
                break;
            }
            MutexLock(&device->lock);                                                                       // Lock the device
            pcb_t* current = GetCurrentProcess();                                                           // Save the calling process
            SetCurrentProcess(device->driver->driverProcess);                                               // Switch to the driver
            regs->eax = device->write(device, (char*)regs->ecx, regs->edx);                                 // Call the device's write function
            SetCurrentProcess(current);                                                                     // Switch back to the caller
            MutexUnlock(&device->lock);  
            break;
        }
        case SYS_DEVICE_IOCTL: {
            // SYS_DEVICE_IOCTL
            // EBX contains the device ID of the device perform the ioctl on
            // ECX contains the command to perform
            // EDX contains the argument for the command
            break;
        }
        


        // Priveliged system calls for drivers and kernel modules (privelige check required, will check PCB)
        // Note - these will always be the highest system calls.
        // Microkernel for now? Is it easier? What about speed?
        case SYS_MODULE_LOAD:
            // EBX contains a pointer to the driver struct
            // ECX contains a pointer to the device the driver is aquiring

            if(regs->ebx == 0){
                // NULL pointer passed
                regs->eax = DRIVER_FAILURE;
                break;
            }

            // Get the driver's PCB and set the proper flags
            pcb_t* driverPCB = GetCurrentProcess();
            if(driverPCB != kernelPCB){
                driverPCB->state = DRIVER;
                driverPCB->timeSlice = 0; // No time slice for drivers
            }

            regs->eax = RegisterDriver((driver_t*)regs->ebx, (device_t*)regs->ecx);

            break;
        case SYS_MODULE_UNLOAD:
            // SYS_MODULE_UNLOAD
            // Remove a driver and delete its entry in the device registry
            // EBX contains a pointer to the driver struct
            // ECX contains a pointer to the device the driver is in charge of
            if((driver_t*)regs->ebx == NULL || ((driver_t*)regs->ebx)->driverProcess != GetCurrentProcess()){
                // NULL pointer passed or the driver is not the current process (a driver can only unload itself)
                regs->eax = DRIVER_FAILURE;
                break;
            }
            regs->eax = UnregisterDriver((driver_t*)regs->ebx, (device_t*)regs->ecx);
            break;
        case SYS_ADD_VFS_DEV:
            // SYS_ADD_VFS_DEV
            // Add a device entry to the VFS
            // EBX contains a pointer to the device to add to the VFS
            // ECX contains the device name
            // EDX contains the path to add the device to (i.e. /dev)
            if(strncmp((char*)regs->edx, "/dev", 4) == 0){
                // Add the device to the VFS
                int result = VfsAddDevice((device_t*)regs->ebx, (char*)regs->ecx, (char*)regs->edx);
                regs->eax = result;
            }else{
                printk("Error: a path outside of /dev is against the rules!\n");
                regs->eax = STANDARD_FAILURE;
            }
            break;
        case SYS_MODULE_QUERY:
            // SYS_MODULE_QUERY
            // Probe a kernel module
            // This is important for drivers that add new devices that can use other drivers. A disk driver, for example, may need a filesystem driver to read from the disk.
            // EBX contains a pointer to the driver struct
            // ECX contains a pointer to the device that we are querying

            if((driver_t*)regs->ebx == NULL){
                // NULL pointer passed
                regs->eax = STANDARD_FAILURE;
                break;
            }

            regs->eax = ((driver_t*)regs->ebx)->probe((device_t*)regs->ecx);
            break;
        case SYS_FIND_MODULE:
            // SYS_FIND_MODULE
            // Find a module by its supported devices
            // EBX contains the pointer to the device to find the driver for
            // ECX contains the type of device to find support for
            regs->eax = (uintptr_t)FindDriver((device_t*)regs->ebx, (DEVICE_TYPE)regs->ecx);

            break;
        case SYS_REGISTER_DEVICE:
            // SYS_REGISTER_DEVICE
            // EBX contains a pointer to the device to register
            regs->eax = RegisterDevice((device_t*)regs->ebx);

            // Check the module type and load it
            // Do what is neccecary for the type of module (device, filesystem, etc.)
            break;
        case SYS_UNREGISTER_DEVICE:
            // SYS_UNREGISTER_DEVICE
            // EBX contains a pointer to the struct containing the device data
            regs->eax = UnregisterDevice((device_t*)regs->ebx);

            // The driver is responsible for its own memory management
            break;
        case SYS_GET_DEVICE:
            // SYS_GET_DEVICE
            // EBX contains a pointer to the path of the device to get
            regs->eax = (uintptr_t)GetDeviceFromVfs((char*)regs->ebx);
            break;
        case SYS_REQUEST_IRQ:
            // SYS_REQUEST_IRQ
            // EBX contains the IRQ number to request
            // ECX contains a pointer to the handler function
            regs->eax = InstallIRQ(regs->ebx, (void(*)(struct Registers*))regs->ecx);
            break;
        case SYS_RELEASE_IRQ:
            // SYS_RELEASE_IRQ
            // EBX contains the IRQ number to release
            
            // Disable the IRQ in the PIC
            // Is there a way to check owned IRQs or should drivers just be absolutely certain they're setting the right IRQ?
            regs->eax = RemoveIRQ(regs->ebx);
            break;
        case SYS_DRIVER_MMAP:{
            // SYS_DRIVER_MMAP
            // EBX contains the physical address to map
            // ECX contains the virtual address to map to
            // EDX contains the flags (read, write, execute
            // ESI contains the size of the region to map

            int result = 0;
            for(size_t i = 0; i < (size_t)regs->esi; i += PAGE_SIZE){
                result = physpalloc((physaddr_t)(regs->ebx + i), (virtaddr_t)(regs->ecx + i), regs->edx);
                if(result != STANDARD_SUCCESS){
                    break;
                }
            }

            regs->eax = result;

            // Page a memory region for a driver to access
            // Make sure already mapped regions are not overwritten (palloc already has this functionality)
            break;
        }
        case SYS_DRIVER_MUNMAP:{
            // SYS_DRIVER_MUNMAP
            // EBX contains the virtual address to unmap
            // ECX contains the size of the region to unmap
            int result = 0;
            for(size_t i = 0; i < (size_t)regs->ecx; i += PAGE_SIZE){
                result = user_pfree((virtaddr_t)(regs->ebx + i));
                if(result != STANDARD_SUCCESS){
                    break;
                }
            }

            regs->eax = result;

            // Unmap/Unpage a memory region for a driver to access
            break;
        }
        case SYS_IO_PORT_READ:{
            // SYS_IO_PORT_READ
            // EBX contains the port to read from
            // ECX contains the size of the read (1, 2, or 4 bytes)

            switch(regs->ecx){
                case 8:
                    regs->eax = inb(regs->ebx);
                    break;
                case 16:
                    regs->eax = inw(regs->ebx);
                    break;
                case 32:
                    regs->eax = inl(regs->ebx);
                    break;
                default:
                    regs->eax = DRIVER_INVALID_ARGUMENT;
                    break;
            }
            break;
        }
        case SYS_IO_PORT_WRITE: {
            // SYS_IO_PORT_WRITE
            // EBX contains the port to write to
            // ECX contains the size of the write (1, 2, or 4 bytes)
            // EDX contains the value to write

            switch(regs->ecx){
                case 8:
                    outb(regs->ebx, regs->edx);
                    break;
                case 16:
                    outw(regs->ebx, regs->edx);
                    break;
                case 32:
                    outl(regs->ebx, regs->edx);
                    break;
                default:
                    regs->eax = DRIVER_INVALID_ARGUMENT;
                    return;
            }

            regs->eax = DRIVER_SUCCESS;
            break;
        }
        case SYS_ENTER_V86_MODE:
            // SYS_ENTER_V86_MODE
            // Needed?
            regs->eflags |= 0x00020000;
            break;
        case SYS_SHUTDOWN:{
            // SYS_SHUTDOWN
            // Shutdown the system
            AcpiShutdown();
            regs->eax = STANDARD_FAILURE;
            break;
        }
        case SYS_REBOOT:{
            // SYS_REBOOT
            // Reboot the system
            reboot_system();
            regs->eax = STANDARD_FAILURE;
            break;
        }
        default: {
            printk("Unknown syscall: 0x%x\n", regs->eax);
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

void ISRHandler(struct Registers *regs){
    sti
    if (handlers[regs->int_no]) {
        handlers[regs->int_no](regs);
    }
}

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

static void ExceptionHandler(struct Registers *regs){
    cli
    pcb_t* current = GetCurrentProcess();
    if(current == kernelPCB){
        // Exception was thrown by the kernel - cannot recover
        printk("KERNEL PANIC: %s\n", exceptions[regs->int_no]);
        regdump(regs);
        STOP
    }
    switch(regs->int_no){
        case PAGE_FAULT:{
            // Gracefully handle a page fault
            printk("Segmentation fault\n", current->pid);
            regdump(regs);
            SwitchProcess(true, regs);
            break;
        }
        case EXCEPTION_STACK_FAULT:{
            // Gracefully handle a stack fault (likely a stack overflow)
            printk("Stack fault\n");
            regdump(regs);
            SwitchProcess(true, regs);
            break;
        }
        // Other exceptions thrown by user applications...
        default:{
            printk("KERNEL PANIC: %s\n", exceptions[regs->int_no]);
            regdump(regs);
            STOP
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

    InstallISR(SYSCALL_INT, syscall_handler);
}