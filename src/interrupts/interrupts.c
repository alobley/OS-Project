#include <interrupts.h>
#include <console.h>
#include <keyboard.h>
#include <multitasking.h>
#include <kernel.h>
#include <time.h>
#include <devices.h>

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

//extern void syscall_handler(struct Registers* regs);

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
    printf("EAX: 0x%x\n", regs->eax);
    printf("EBX: 0x%x\n", regs->ebx);
    printf("ECX: 0x%x\n", regs->ecx);
    printf("EDX: 0x%x\n", regs->edx);
    printf("ESI: 0x%x\n", regs->esi);
    printf("EDI: 0x%x\n", regs->edi);
    printf("EBP: 0x%x\n", regs->ebp);
    printf("ESP: 0x%x\n", regs->esp);
    printf("EIP: 0x%x\n", regs->eip);
    printf("CS: 0x%x\n", regs->cs);
    printf("DS: 0x%x\n", regs->ds);
    printf("ES: 0x%x\n", regs->es);
    printf("FS: 0x%x\n", regs->fs);
    printf("SS: 0x%x\n", regs->ss);
    printf("EFLAGS: 0x%x\n", regs->eflags);

    uint32_t cr0, cr2, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    printf("CR0: 0x%x ", cr0);
    printf("CR2: 0x%x ", cr2);
    printf("CR3: 0x%x ", cr3);
    printf("CR4: 0x%x\n", cr4);
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
HOT void syscall_handler(struct Registers *regs){
    int result;
    switch(regs->eax){
        case SYS_DBG: {
            // SYS_DBG
            printf("Syscall Debug!\n");
            regs->eax = 1;
            break;
        }
        case SYS_INSTALL_KBD_HANDLE:
            // SYS_INSTALL_KBD_HANDLE
            InstallKeyboardCallback((KeyboardCallback)regs->ebx);
            break;
        case SYS_REMOVE_KBD_HANDLE:
            // SYS_REMOVE_KBD_HANDLE
            RemoveKeyboardCallback((KeyboardCallback)regs->ebx);
            break;
        case SYS_WRITE:
            // SYS_WRITE
            // EBX contains the file descriptor
            // ECX contains the pointer to the data to write
            // EDX contains the number of bytes to write

            // Write data to the file descriptor
            switch(regs->ebx){
                case STDOUT_FILENO: {
                    // TODO: change STDOUT_FILENO to the actual file descriptor
                    WriteStringSize((char*)regs->ecx, regs->edx);
                    break;
                }
                default: {
                    printf("Unknown file descriptor: %d\n", regs->ebx);
                    break;
                }
            }
            break;
        case SYS_READ:
            // SYS_READ
            break;
        case SYS_EXIT:
            // SYS_EXIT
            // EBX contains the exit code
            // Exit a process
            break;
        case SYS_FORK:
            // SYS_FORK
            // Fork a process
            break;
        case SYS_EXEC:
            // SYS_EXEC
            // EBX contains the pointer to the path of the executable
            // ECX contains the pointer to the arguments
            // EDX contains the pointer to the environment variables
            // Execute a process (replaces current process)
            break;
        case SYS_WAIT:
            // SYS_WAIT
            // EBX contains the PID to wait for
            // Wait for a process to exit
            break;
        case SYS_GET_PID:
            // SYS_GET_PID
            regs->eax = (uint32_t)GetCurrentProcess()->pid;
            break;
        case SYS_GET_PCB:
            // SYS_GET_PCB
            // Gets own PCB when not priveliged, otherwise gets PCB of process with PID in ebx
            if(!CheckPrivelige()){
                regs->eax = (uint32_t)GetCurrentProcess();
            }else{
                // Don't have this yet, just do the same thing as above
                regs->eax = (uint32_t)GetCurrentProcess();
            }
            break;
        case SYS_OPEN:
            // SYS_OPEN
            break;
        case SYS_CLOSE:
            // SYS_CLOSE
            break;
        case SYS_SEEK:
            // SYS_SEEK
            break;
        case SYS_SLEEP:
            // SYS_SLEEP
            break;
        case SYS_GET_TIME:
            // SYS_GET_TIME
            regs->eax = (uint32_t)&currentTime;
            break;
        case SYS_KILL:
            // SYS_KILL
            break;
        case SYS_YIELD:
            // SYS_YIELD
            break;
        case SYS_MMAP:
            // SYS_MMAP
            break;
        case SYS_MUNMAP:
            // SYS_MUNMAP
            break;
        case SYS_BRK:
            // SYS_BRK
            break;
        case SYS_MPROTECT:
            // SYS_MPROTECT
            break;
        case SYS_REGDUMP:
            // SYS_REGDUMP
            // Dump the CPU's registers to the console for debugging reasons
            regdump(regs);
            break;
        


        // Priveliged system calls for drivers and kernel modules (privelige check required, will check PCB)
        // Note - these will always be the highest system calls.
        // Microkernel for now? Is it easier? What about speed?
        case SYS_MODULE_LOAD:
            // SYS_MODULE_LOAD
            if(!CheckPrivelige()){
                printf("Unpriveliged Application requesting system resources. Killing process.\n");
                // Log the error
                // Kill the process
                break;
            }
            // EBX contains a pointer to the driver struct
            // ECX contains a pointer to the device the driver is aquiring
            // ESI contains a pointer to the PCB (if the driver is a process, otherwise NULL)

            // Get the driver's PCB and set the proper flags
            if(regs->esi != 0){
                pcb_t* driverPCB = (pcb_t*)regs->esi;
                driverPCB->state = DRIVER;
                driverPCB->timeSlice = 0;
            }

            // Load the device driver
            result = RegisterDriver((driver_t*)regs->ebx, (device_t*)regs->ecx);
            regs->eax = result;
            if(result == DRIVER_SUCCESS){
                ((driver_t*)regs->ebx)->init();
            }else{
                printf("Failed to load driver: %d\n", result);
            }

            break;
        case SYS_MODULE_UNLOAD:
            // SYS_MODULE_UNLOAD
            // Remove a driver and delete its entry in the device registry
            break;
        case SYS_MODULE_QUERY:
            // SYS_MODULE_QUERY
            break;
        case SYS_REGISTER_DEVICE:
            // SYS_REGISTER_DEVICE
            // EBX contains a pointer to the device to register
            // ECX contains a pointer to the parent device (if any)
            if(!CheckPrivelige()){
                printf("Unpriveliged Application requesting system resources. Killing process.\n");
                // Log the error
                // Kill the process
                break;
            }
            result = RegisterDevice((device_t*)regs->ebx, (device_t*)regs->ecx);
            regs->eax = result;

            // Check the module type and load it
            // Do what is neccecary for the type of module (device, filesystem, etc.)
            break;
        case SYS_UNREGISTER_DEVICE:
            // SYS_UNREGISTER_DEVICE
            // EBX contains a value signaling what kind of device to unload
            // ECX contains a pointer to the struct containing the device data

            // Search for the device in the device list and unload it
            // The driver is responsible for its own memory management
            break;
        case SYS_GET_DEVICE:
            // SYS_GET_DEVICE
            // EBX contains the device ID to get
            regs->eax = (uint32_t)GetDeviceByID((device_id_t)regs->ebx);
            break;
        case SYS_GET_FIRST_DEVICE:
            // SYS_GET_FIRST_DEVICE
            // EBX contains the device type to get
            regs->eax = (uint32_t)GetFirstDevice((device_type_t)regs->ebx);
            break;
        case SYS_REQUEST_IRQ:
            // SYS_REQUEST_IRQ
            // This should be pretty simple. EBX contains the IRQ number to request, ECX contains a pointer to the handler function
            break;
        case SYS_RELEASE_IRQ:
            // SYS_RELEASE_IRQ
            // EBX contains the IRQ number to release
            
            // Disable the IRQ in the PIC
            break;
        case SYS_DRIVER_SET_STATUS:
            // SYS_DRIVER_SET_STATUS
            // Set the status of the device this driver is responsible for
            break;
        case SYS_DRIVER_MMAP:
            // SYS_DRIVER_MMAP
            // EBX contains the physical address to map
            // ECX contains the size of the region to map

            // Page a memory region for a driver to access
            // Make sure already mapped regions are not overwritten (palloc already has this functionality)
            break;
        case SYS_DRIVER_MUNMAP:
            // SYS_DRIVER_MUNMAP
            // EBX contains the physical address to unmap
            // ECX contains the size of the region to unmap

            // Unmap/Unpage a memory region for a driver to access
            break;
        case SYS_IO_PORT_READ:
            // SYS_IO_PORT_READ
            // EBX contains the port to read from
            // ECX contains the size of the read (1, 2, or 4 bytes)
            if(!CheckPrivelige()){
                printf("Unpriveliged Application requesting system resources. Killing process.\n");
                // Log the error
                // Kill the process
                break;
            }

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
                    printf("Unknown size for I/O port read: %d\n", regs->ecx);
                    regs->eax = 0;
                    break;
            }
            break;
        case SYS_IO_PORT_WRITE:
            // SYS_IO_PORT_WRITE
            // EBX contains the port to write to
            // ECX contains the size of the write (1, 2, or 4 bytes)
            // EDX contains the value to write
            if(!CheckPrivelige()){
                printf("Unpriveliged Application requesting system resources. Killing process.\n");
                // Log the error
                // Kill the process
                break;
            }

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
                    printf("Unknown size for I/O port write: %d\n", regs->ecx);
                    break;
            }
            break;
        case SYS_BLOCK_READ:
            // SYS_BLOCK_READ
            // Takes a pointer to a block device struct and reads from it
            // EBX contains the pointer to the block device struct
            // ECX contains the LBA to read from
            // EDX contains the number of sectors to read
            // ESI contains the pointer to the buffer to read into
            break;
        case SYS_BLOCK_WRITE:
            // SYS_BLOCK_WRITE
            break;
        case SYS_ENTER_V86_MODE:
            // SYS_ENTER_V86_MODE
            // Needed?
            regs->eflags |= 0x00020000;
            break;
        default: {
            printf("Unknown syscall: 0x%x\n", regs->eax);
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
    if (handlers[regs->int_no]) {
        handlers[regs->int_no](regs);
    }
}

extern void reboot();

static void ExceptionHandler(struct Registers *regs){
    printf("KERNEL PANIC: %s\n", exceptions[regs->int_no]);
    regdump(regs);
    cli
    hlt
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