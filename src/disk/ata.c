#include <kernel.h>
#include <devices.h>

// Subject to change
char* name = "pat0";
char* description = "Primary ATA Controller";

// Placeholder values
device_id_t deviceID = 0xBEEF;
vendor_id_t vendorID = 0xBEEF;

driverstatus Ata_Init(){
    return DRIVER_SUCCESS;
}

void InitializeAta(){
    device_t* pata = CreateDevice(deviceID, vendorID, DEVICE_STATUS_IDLE, name, description, NULL, DEVICE_TYPE_BLOCK);
    if(pata == NULL){
        printf("Failed to create PATA device\n");
        return;
    }

    driver_t* pataDriver = CreateDriver(name, description, Ata_Init);
    if(pataDriver == NULL){
        printf("Failed to create PATA driver\n");
        return;
    }

    pata->driver = pataDriver;
    pataDriver->device = pata;

    // Register the device
    int result = 0;
    do_syscall(SYS_REGISTER_DEVICE, (uint32_t)pata, 0, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != REGISTER_SUCCESS){
        printf("Failed to register PATA device\n");
        return;
    }

    //STOP

    // Register the driver
    do_syscall(SYS_MODULE_LOAD, (uint32_t)pataDriver, (uint32_t)pata, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != DRIVER_SUCCESS){
        printf("Failed to load PATA driver\n");
        return;
    }

    //STOP
}