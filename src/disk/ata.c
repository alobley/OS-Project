/* ATA.c
 * ATA driver for the kernel
 * This driver is responsible for initializing the ATA controller and handling ATA devices
 * This is currently meant to test the kernel's driver interface
 * This driver is a work in progress
*/
#include <kernel.h>
#include <devices.h>

// Subject to change
char* name = "pata";
char* description = "Primary ATA Controller";

// Placeholder values
device_id_t deviceID = 0xBEEF;
vendor_id_t vendorID = 0xBEEF;

device_t* controller = NULL;
driver_t* pataDriver = NULL;

driverstatus Ata_Init(){
    // Register the first drive
    int result = 0;
    // Do checks through I/O ports to see if there is a drive connected...
    // If not, unregister the devices and driver
    device_t* drive = CreateDevice(deviceID, vendorID, DEVICE_STATUS_IDLE, "pat0", "Primary ATA Drive", NULL, DEVICE_TYPE_BLOCK);
    if(drive == NULL){
        printf("Failed to create PATA drive\n");
        return DRIVER_FAILURE;
    }
    drive->parent = controller; 
    drive->driver = pataDriver;
    do_syscall(SYS_REGISTER_DEVICE, (uint32_t)drive, (uint32_t)controller, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != REGISTER_SUCCESS){
        printf("Failed to register PATA drive\n");
        return DRIVER_FAILURE;
    }
    return DRIVER_SUCCESS;
}

void InitializeAta(){
    controller = CreateDevice(deviceID, vendorID, DEVICE_STATUS_IDLE, name, description, NULL, DEVICE_TYPE_BLOCK);
    if(controller == NULL){
        printf("Failed to create PATA device\n");
        return;
    }

    pataDriver = CreateDriver(name, description, Ata_Init);
    if(pataDriver == NULL){
        printf("Failed to create PATA driver\n");
        return;
    }

    // Register the controller
    int result = 0;
    do_syscall(SYS_REGISTER_DEVICE, (uint32_t)controller, 0, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != REGISTER_SUCCESS){
        printf("Failed to register PATA device\n");
        return;
    }

    // Register the driver
    do_syscall(SYS_MODULE_LOAD, (uint32_t)pataDriver, (uint32_t)controller, 0, 0, 0);
    asm volatile("mov %%eax, %0" : "=r" (result));
    if(result != DRIVER_SUCCESS){
        printf("Failed to load PATA driver\n");
        return;
    }
}