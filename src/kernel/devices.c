#include <devices.h>
#include <alloc.h>
#include <console.h>
#include <vfs.h>

device_registry_t* deviceRegistry;


int InitDeviceRegistry(){
    deviceRegistry = (device_registry_t*)halloc(sizeof(device_registry_t));
    if(deviceRegistry == NULL){
        return REGISTER_OUT_OF_MEMORY;
    }
    deviceRegistry->firstDevice = NULL;
    deviceRegistry->lastDevice = NULL;
    deviceRegistry->numDevices = 0;
    return REGISTER_SUCCESS;
}

int DestroyDeviceRegistry(){
    if(deviceRegistry == NULL){
        return REGISTER_FAILURE;
    }
    device_t* current = deviceRegistry->firstDevice;
    while(current != NULL){
        device_t* next = current->next;
        DestroyDevice(current);
        current = next;
    }
    hfree(deviceRegistry);
    deviceRegistry = NULL;
    return REGISTER_SUCCESS;
}

driver_t* CreateDriver(char* name, char* description, driverstatus (*init)()){
    driver_t* driver = (driver_t*)halloc(sizeof(driver_t));
    if(driver == NULL){
        return NULL;
    }
    driver->name = NULL;
    driver->description = NULL;
    driver->init = NULL;
    driver->device = NULL;
    return driver;
}

int DestroyDriver(driver_t* driver){
    if(driver == NULL){
        return -1;
    }
    hfree(driver->name);
    hfree(driver->description);
    hfree(driver);
    return 0;
}

device_t* CreateDevice(device_id_t deviceID, vendor_id_t vendorID, device_status_t status, char* name, char* description, void* deviceInfo, device_type_t type){
    device_t* device = (device_t*)halloc(sizeof(device_t));
    if(device == NULL){
        return NULL;
    }
    device->deviceID = deviceID;
    device->vendorID = vendorID;
    device->status = status;
    device->name = name;
    device->description = description;
    device->deviceInfo = deviceInfo;
    device->driver = NULL;
    device->type = type;
    device->next = NULL;
    device->prev = NULL;
    return device;
}

int DestroyDevice(device_t* device){
    if(device == NULL){
        return -1;
    }
    hfree(device->name);
    hfree(device->description);
    hfree(device);
    return 0;
}

int RegisterDevice(device_t* device, device_t* parent){
    if(deviceRegistry == NULL){
        return REGISTER_FAILURE;
    }
    if(device == NULL){
        return REGISTER_FAILURE;
    }
    if(deviceRegistry->firstDevice == NULL){
        if(parent != NULL){
            // Registering a child of an unregistered device is not allowed
            return REGISTER_FAILURE;
        }
        deviceRegistry->firstDevice = device;
        deviceRegistry->lastDevice = device;
    }else{
        if(parent != NULL){
            parent->firstChild = device;
        }else{
            deviceRegistry->lastDevice->next = device;
            device->prev = deviceRegistry->lastDevice;
            deviceRegistry->lastDevice = device;
        }
    }
    deviceRegistry->numDevices++;

    // Add device to the VFS
    VfsAddDevice(device, device->name, "/dev");
    return REGISTER_SUCCESS;
}

int UnregisterDevice(device_t* device){
    if(deviceRegistry == NULL){
        return REGISTER_FAILURE;
    }
    if(device == NULL){
        return REGISTER_DEVICE_NOT_FOUND;
    }
    if(deviceRegistry->firstDevice == device){
        deviceRegistry->firstDevice = device->next;
    }
    if(deviceRegistry->lastDevice == device){
        deviceRegistry->lastDevice = device->prev;
    }
    if(device->prev != NULL){
        device->prev->next = device->next;
    }
    if(device->next != NULL){
        device->next->prev = device->prev;
    }
    deviceRegistry->numDevices--;
    VfsRemoveNode(VfsFindNode(JoinPath("/dev", device->name)));
    return REGISTER_SUCCESS;
}

int RegisterDriver(driver_t* driver, device_t* device){
    if(driver == NULL || device == NULL){
        return REGISTER_FAILURE;
    }
    if(device->driver != NULL){
        return REGISTER_DEVICE_ALREADY_REGISTERED;
    }
    device->driver = driver;
    driver->device = device;
    return REGISTER_SUCCESS;
}

device_t* GetDeviceByID(device_id_t deviceID){
    if(deviceRegistry == NULL){
        return NULL;
    }
    device_t* current = deviceRegistry->firstDevice;
    while(current != NULL){
        if(current->deviceID == deviceID){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

device_t* GetFirstDevice(device_type_t type){
    if(deviceRegistry == NULL){
        return NULL;
    }
    device_t* current = deviceRegistry->firstDevice;
    while(current != NULL){
        if(current->type == type){
            return current;
        }
        current = current->next;
    }
    return NULL;
}