#include <drivers.h>
#include <kernel.h>

DRIVERSTATUS module_load(driver_t* this, device_t* device){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_MODULE_LOAD, (uintptr_t)this, (uintptr_t)device, 0, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS module_unload(driver_t* this, device_t* device){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_MODULE_UNLOAD, (uintptr_t)this, (uintptr_t)device, 0, 0, 0);
    getresult(result);
    return result;
}

int add_vfs_device(device_t* device, const char* name, const char* path){
    int result = 0;
    do_syscall(SYS_ADD_VFS_DEV, (uintptr_t)device, (uintptr_t)name, (uintptr_t)path, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS query_module(driver_t* this, device_t* device){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_MODULE_QUERY, (uintptr_t)this, (uintptr_t)device, 0, 0, 0);
    getresult(result);
    return result;
}

driver_t* find_module(device_t* device, DEVICE_TYPE type){
    driver_t* result = NULL;
    do_syscall(SYS_FIND_MODULE, (uintptr_t)device, (uintptr_t)type, 0, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS register_device(device_t* device){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_REGISTER_DEVICE, (uintptr_t)device, 0, 0, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS unregister_device(device_t* device){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_UNREGISTER_DEVICE, (uintptr_t)device, 0, 0, 0, 0);
    getresult(result);
    return result;
}

device_t* get_device(char* path){
    device_t* result = NULL;
    do_syscall(SYS_GET_DEVICE, (uintptr_t)path, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int request_irq(unsigned int irqno, void(*handler)(struct Registers* regs)){
    int result = 0;
    do_syscall(SYS_REQUEST_IRQ, irqno, handler, 0, 0, 0);
    getresult(result);
    return result;
}

int release_irq(unsigned int irqno){
    int result = 0;
    do_syscall(SYS_RELEASE_IRQ, irqno, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int page_physical_address(physaddr_t paddr, virtaddr_t vaddr, unsigned int flags, size_t size){
    int result = 0;
    do_syscall(SYS_DRIVER_MMAP, paddr, vaddr, flags, size, 0);
    getresult(result);
    return result;
}

int unpage_physical_address(virtaddr_t vaddr, size_t size){
    int result = 0;
    do_syscall(SYS_DRIVER_MUNMAP, vaddr, size, 0, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS io_port_read(unsigned short port, unsigned int size){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_IO_PORT_READ, (unsigned int)port, size, 0, 0, 0);
    getresult(result);
    return result;
}

DRIVERSTATUS io_port_write(unsigned short port, unsigned int size, unsigned int value){
    DRIVERSTATUS result = 0;
    do_syscall(SYS_IO_PORT_WRITE, (unsigned int)port, size, value, 0, 0);
    getresult(result);
    return result;
}