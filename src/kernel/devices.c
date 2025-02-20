#include <devices.h>
#include <alloc.h>

device_registry_t* deviceRoot = NULL;

driver_t* rootBusDriver = NULL;

char* rootBusName = "Motherboard";
char* rootBusDesc = "System root bus";
char* rootBusManufacturer = "Dedication OS developer";
char* rootBusFirmware = "IBM-compatible BIOS";

void InitializeDeviceRegistry(){
    rootBusDriver = (driver_t*)halloc(sizeof(driver_t));
    rootBusDriver->name = rootBusName;
    rootBusDriver->type = DEVICE_TYPE_BUS;
    rootBusDriver->deviceId = 0;
    rootBusDriver->vendorId = 0;
    rootBusDriver->pcb = NULL;
    
    deviceRoot = (device_registry_t*)halloc(sizeof(device_registry_t));
    deviceRoot->rootBus = (device_t*)halloc(sizeof(device_t));
    deviceRoot->rootBus->type = DEVICE_TYPE_BUS;
    deviceRoot->rootBus->status = DEVICE_STATUS_OK;
    deviceRoot->rootBus->deviceData = NULL;
    deviceRoot->rootBus->lock = MUTEX_INIT;
    deviceRoot->rootBus->Command = NULL;
    deviceRoot->rootBus->name = rootBusName;
    deviceRoot->rootBus->description = rootBusDesc;
    deviceRoot->rootBus->manufacturer = rootBusManufacturer;
    deviceRoot->rootBus->model = NULL;
    deviceRoot->rootBus->serial = NULL;
    deviceRoot->rootBus->firmware = rootBusFirmware;
    deviceRoot->rootBus->parent = NULL;
    deviceRoot->rootBus->children = NULL;
    deviceRoot->rootBus->next = NULL;
    deviceRoot->rootBus->driver = rootBusDriver;
    deviceRoot->rootBus->bus = NULL;
    deviceRoot->numDevices = 0;

    // Build device tree...
}