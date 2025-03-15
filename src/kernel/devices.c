#include <devices.h>
#include <alloc.h>
#include <vfs.h>

device_registry_t* registry = NULL;

int CreateDeviceRegistry(){
    registry = (device_registry_t*)halloc(sizeof(device_registry_t));
    if(registry == NULL){
        return DRIVER_OUT_OF_MEMORY;
    }

    registry->firstDevice = NULL;
    registry->lastDevice = NULL;
    registry->numDevices = 0;

    return DRIVER_SUCCESS;
}

void DestroyDeviceRegistry(){
    device_t* current = registry->firstDevice;
    while(current != NULL){
        device_t* next = current->next;
        hfree(current);
        current = next;
    }
    hfree(registry);
}

int RegisterDevice(device_t* device){
    if(registry->firstDevice == NULL){
        registry->firstDevice = device;
        registry->lastDevice = device;
        device->next = NULL;
    }else{
        registry->lastDevice->next = device;
        registry->lastDevice = device;
        device->next = NULL;
    }
    registry->numDevices++;
    return DRIVER_SUCCESS;
}

int UnregisterDevice(device_t* device){
    if(registry->firstDevice == NULL){
        return DRIVER_NOT_INITIALIZED;
    }

    if(registry->firstDevice == device){
        registry->firstDevice = device->next;
        if(registry->lastDevice == device){
            registry->lastDevice = NULL;
        }
    }else{
        device_t* current = registry->firstDevice;
        while(current != NULL && current->next != device){
            current = current->next;
        }
        if(current == NULL){
            return DRIVER_NOT_INITIALIZED;
        }
        current->next = device->next;
        if(registry->lastDevice == device){
            registry->lastDevice = current;
        }
    }
    hfree(device);
    registry->numDevices--;
    return DRIVER_SUCCESS;
}

int RegisterDriver(driver_t* driver, device_t* device){
    device->driver = driver;
    return DRIVER_SUCCESS;
}

int UnregisterDriver(driver_t* driver, device_t* device){
    device->driver = NULL;
    return DRIVER_SUCCESS;
}

device_t* GetDeviceByID(device_id_t id){
    device_t* current = registry->firstDevice;
    while(current != NULL){
        if(current->id == id){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

device_t* GetFirstDeviceByType(DEVICE_TYPE type){
    device_t* current = registry->firstDevice;
    while(current != NULL){
        if(current->type == type){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

device_t* GetNextDeviceByType(device_t* previous, DEVICE_TYPE type){
    device_t* current = previous->next;
    while(current != NULL){
        if(current->type == type){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

device_t* GetDeviceFromVfs(char* path){
    vfs_node_t* node = VfsFindNode(path);
    if(node == NULL){
        return NULL;
    }
    return (device_t*)node->data;
}

driver_t* CreateDriver(const char* name, const char* description, driver_id_t id, uint32_t version, DRIVERSTATUS (*init)(void), DRIVERSTATUS (*deinit)(void), DRIVERSTATUS (*probe)(device_t* device)){
    driver_t* driver = (driver_t*)halloc(sizeof(driver_t));
    memset(driver, 0, sizeof(driver_t));
    driver->name = name;
    driver->description = description;
    driver->id = id;
    driver->version = version;
    driver->init = init;
    driver->deinit = deinit;
    driver->probe = probe;
    return driver;
}