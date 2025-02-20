#include <devices.h>
#include <alloc.h>
#include <vfs.h>

device_registry_t* deviceRoot = NULL;

driver_t* rootBusDriver = NULL;

char* rootBusName = "Motherboard";
char* rootBusDesc = "System root bus";
char* rootBusManufacturer = "Dedication OS developer";
char* rootBusFirmware = "IBM-compatible BIOS";

device_t* CreateDevice(device_type_t type, device_status_t status, void* deviceData, driver_t* driver, char* name, char* description, char* manufacturer, 
                       char* model, char* serial, char* firmware, uint32_t deviceId, bool isBus, driver_status (*Command)(device_t* self, device_command_t command, void* data, 
                       size_t bufferLen))
{
    device_t* device = (device_t*)halloc(sizeof(device_t));
    device->type = type;
    device->status = status;
    device->deviceData = deviceData;
    device->lock = MUTEX_INIT;
    device->Command = Command;
    device->name = name;
    device->description = description;
    device->manufacturer = manufacturer;
    device->model = model;
    device->serial = serial;
    device->firmware = firmware;
    device->parent = NULL;
    device->children = NULL;
    device->next = NULL;
    device->driver = driver;
    device->id = deviceId;
    device->numDevices = 0;
    device->isBus = isBus;
    return device;
}

driver_t* CreateDriver(char* name, device_type_t type, uint16_t deviceId, uint16_t vendorId, void* pcb){
    driver_t* driver = (driver_t*)halloc(sizeof(driver_t));
    driver->name = name;
    driver->type = type;
    driver->deviceId = deviceId;
    driver->vendorId = vendorId;
    driver->pcb = pcb;
    return driver;
}

// TODO: Add the ability for non-bus devices to have child (virtual) devices
int AddDevice(device_t* device, device_type_t type){
    if(device == NULL){
        return -1;
    }
    device_t* current = deviceRoot->rootBus;
    while(current->next != NULL){
        // Locate the bus
        if(current->type != type){
            current = current->next;
        }
    }

    if(current != NULL && current->type != type){
        // If we are adding a bus, add it to the root bus
        current->next = device;
        device->parent = deviceRoot->rootBus;
        deviceRoot->rootBus->numDevices++;
    }else if(current != NULL && current->type == type){
        // If we are adding a device, add it to the bus
        if(current->children == NULL){
            // No children, add a new child
            current->children = device;
            device->parent = current;
        }else{
            // There are children, append the new one
            current = current->children;
            while(current->next != NULL){
                current = current->next;
            }
        }
        current->next = device;
        device->parent = current->parent;
        current->parent->numDevices++;
    }else{
        return -1;
    }

    deviceRoot->numDevices++;
    return 0;
}

void InitializeDeviceRegistry(){
    rootBusDriver = CreateDriver(rootBusName, DEVICE_TYPE_MOTHERBOARD, 0, 0, NULL);
    device_t* motherboard = CreateDevice(DEVICE_TYPE_MOTHERBOARD, DEVICE_STATUS_OK, NULL, rootBusDriver, rootBusName, rootBusDesc, rootBusManufacturer, 
                                        NULL, NULL, rootBusFirmware, 0, true, NULL);
    deviceRoot = (device_registry_t*)halloc(sizeof(device_registry_t));
    deviceRoot->rootBus = motherboard;
    deviceRoot->numDevices = 1;

    VfsAddDevice(motherboard, "rootdev", "/dev");

    // TODO: Move the following code to driver implementations
    driver_t* keyboardDriver = CreateDriver("PS/2 Keyboard Driver", DEVICE_TYPE_INPUT_DEVICE, 0, 0, NULL);
    device_t* keyboard = CreateDevice(DEVICE_TYPE_INPUT_DEVICE, DEVICE_STATUS_OK, NULL, keyboardDriver, "Keyboard", "Standard PS/2 keyboard", "Dedication OS developer",
    "Standard", " ", " ", 0, false, NULL /*TODO: change the keyboard driver to allow command processing*/);
    AddDevice(keyboard, DEVICE_TYPE_INPUT_DEVICE);
    VfsAddDevice(keyboard, "kb0", "/dev/input");

    driver_t* ramfsDriver = CreateDriver("RamFS Driver", DEVICE_TYPE_FS_DEVICE, 0, 0, NULL);
    device_t* ramfs = CreateDevice(DEVICE_TYPE_FS_DEVICE, DEVICE_STATUS_OK, NULL, ramfsDriver, "RamFS", "RamFS filesystem", "Dedication OS developer", NULL, NULL,
    NULL, 0, false, NULL);
    AddDevice(ramfs, DEVICE_TYPE_FS_DEVICE);
    VfsAddDevice(ramfs, "ram0", "/dev");

    /*
    // Dump the info of the keyboard driver/device for debugging
    printf("Keyboard driver info:\n");
    printf("Name: %s\n", keyboardDriver->name);
    printf("Type: %d\n", keyboardDriver->type);
    printf("Device ID: %d\n", keyboardDriver->deviceId);
    printf("Vendor ID: %d\n", keyboardDriver->vendorId);
    printf("Device type: %d\n", keyboard->type);
    printf("Device status: %d\n", keyboard->status);
    printf("Device name: %s\n", keyboard->name);
    printf("Device description: %s\n", keyboard->description);
    printf("Device manufacturer: %s\n", keyboard->manufacturer);
    printf("Device model: %s\n", keyboard->model);
    printf("Device serial: %s\n", keyboard->serial);
    printf("Device firmware: %s\n", keyboard->firmware);
    printf("Device ID: %d\n", keyboard->id);
    printf("Parent bus devices: %d\n", keyboard->parent->numDevices);
    STOP
    */
}