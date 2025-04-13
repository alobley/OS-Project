#include <devices.h>
#include <alloc.h>
#include <vfs.h>
#include <console.h>

device_registry_t* registry = NULL;

int CreateDeviceRegistry(){
    registry = (device_registry_t*)halloc(sizeof(device_registry_t));
    if(registry == NULL){
        return DRIVER_OUT_OF_MEMORY;
    }
    memset(registry, 0, sizeof(device_registry_t));

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
    if(device == NULL || device->userDevice == NULL){
        printk("Error: NULL device passed to RegisterDevice\n");
        return DRIVER_NOT_INITIALIZED;
    }

    if(registry->firstDevice == NULL){
        registry->firstDevice = device;
        registry->lastDevice = device;
        device->next = NULL;
    }else{
        registry->lastDevice->next = device;
        registry->lastDevice->userDevice->next = device->userDevice;
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
        current->userDevice->next = device->userDevice->next;
        if(registry->lastDevice == device){
            registry->lastDevice = current;
        }
    }
    registry->numDevices--;
    return DRIVER_SUCCESS;
}

/// @brief Register a driver into the device registry
/// @param driver Pointer to the driver to register
/// @param device Pointer to the device this driver is responsible for - can be null.
/// @return Code based on the result of the operation
DRIVERSTATUS RegisterDriver(driver_t* driver, device_t* device){
    if(device != NULL){
        device->driver = driver;
    }
    if(driver == NULL){
        return DRIVER_NOT_INITIALIZED;
    }
    if(registry->firstDriver == NULL){
        registry->firstDriver = driver;
        registry->lastDriver = driver;
        driver->next = NULL;
    }else{
        registry->lastDriver->next = driver;
        registry->lastDriver = driver;
        driver->next = NULL;
    }
    return DRIVER_SUCCESS;
}

DRIVERSTATUS UnregisterDriver(driver_t* driver, device_t* device){
    device->driver = NULL;
    if(registry->firstDriver == NULL){
        return DRIVER_NOT_INITIALIZED;
    }
    if(registry->firstDriver == driver){
        registry->firstDriver = driver->next;
        if(registry->lastDriver == driver){
            registry->lastDriver = NULL;
        }
    }else{
        driver_t* current = registry->firstDriver;
        while(current != NULL && current->next != driver){
            current = current->next;
        }
        if(current == NULL){
            return DRIVER_NOT_INITIALIZED;
        }
        current->next = driver->next;
        if(registry->lastDriver == driver){
            registry->lastDriver = current;
        }
    }
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

device_id_t devid = 0;
device_t* CreateDevice(const char* name, const char* devName, const char* description, void* data, driver_t* driver, DEVICE_TYPE type, vendor_id_t vendorId, device_flags_t flags, readhandle_t read, writehandle_t write, ioctlhandle_t ioctl, device_t* parent){
    device_t* device = (device_t*)halloc(sizeof(device_t));
    if(device == NULL){
        return NULL;
    }
    memset(device, 0, sizeof(device_t));
    device->id = devid;
    devid++;
    device->vendorId = vendorId;
    device->status = DEVICE_STATUS_IDLE;
    memcpy(&device->flags, &flags, sizeof(device_flags_t));
    device->name = name;
    device->description = description;
    device->devName = devName;
    device->driver = driver;
    device->deviceInfo = data;                                   // Partitions identified by filesystem drivers
    device->type = DEVICE_TYPE_BLOCK;
    device->read = read;
    device->write = write;
    device->ioctl = ioctl;
    device->last_error[0] = '\0';
    device->lock = MUTEX_INIT;
    device->parent = parent;
    device->next = NULL;
    device->firstChild = NULL;

    user_device_t* userDevice = (user_device_t*)halloc(sizeof(user_device_t));
    if(userDevice == NULL){
        hfree(device);
        return NULL;
    }
    userDevice->id = device->id;
    userDevice->vendorId = device->vendorId;
    userDevice->name = device->name;
    userDevice->description = device->description;
    userDevice->devName = device->devName;
    userDevice->type = device->type;
    userDevice->last_error[0] = '\0';
    userDevice->parent = NULL;
    userDevice->next = NULL;
    userDevice->firstChild = NULL;
    userDevice->deviceInfo = data;
    if(parent != NULL){
        userDevice->parent = parent->userDevice;
    }
    device->userDevice = userDevice;

    return device;
}

driver_id_t id = 0;
driver_t* CreateDriver(const char* name, const char* description, uint32_t version, DEVICE_TYPE type, driver_init_t init, driver_deinit_t deinit, driver_probe_t probe){
    driver_t* driver = (driver_t*)halloc(sizeof(driver_t));
    memset(driver, 0, sizeof(driver_t));
    driver->name = name;
    driver->description = description;
    driver->id = id;
    id++;
    driver->type = type;
    driver->version = version;
    driver->init = init;
    driver->deinit = deinit;
    driver->probe = probe;
    return driver;
}

// Search for a compatible driver in the device tree
driver_t* FindDriver(device_t* device, DEVICE_TYPE type){
    if(device == NULL || registry->firstDriver == NULL){
        //printk("No drivers found!\n");
        return NULL;
    }
    driver_t* current = registry->firstDriver;
    while(current != NULL){
        if(current == NULL){
            break;
        }
        if(current->probe == NULL){
            current = current->next;
            continue;
        }
        if(current->probe(device) == DRIVER_INITIALIZED && current->type == type){
            //printk("Found driver %s for device %s\n", current->name, device->name);
            return current;
        }
        current = current->next;
    }
    //printk("No valid drivers found!\n");
    return NULL;
}