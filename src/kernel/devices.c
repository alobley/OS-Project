#include <devices.h>
#include <alloc.h>
#include <vfs.h>
#include <console.h>

device_registry_t deviceRegistry = {0};
driver_registry_t driverRegistry = {0};

/// @brief Find a free index in a bitmap
/// @param bitmap Pointer to the bitmap
/// @param bitmapSize The size of the bitmap in uint32_t
/// @return The index that is free, or an index that is greater than the size of the bitmap on failure
index_t FindIndex(uint32_t* bitmap, size_t bitmapSize){
    for(size_t i = 0; i < bitmapSize; i++){
        if(bitmap[i] != 0xFFFFFFFF){
            for(size_t j = 0; j < 32; j++){
                if(!(bitmap[i] & (1 << j))){
                    bitmap[i] |= 1 << j;
                    return i * 32 + j;
                }
            }
        }
    }

    return bitmapSize + 1;
}

/// @brief Obtain a free device ID to use
/// @return The new device ID
device_id_t AllocateDeviceID(){
    device_id_t id = (device_id_t)FindIndex(deviceRegistry.bitmap, REGISTRY_BITMAP_SIZE);
    SetBitmapBit(deviceRegistry.bitmap, (index_t)id);

    return id;
}

/// @brief Set a device ID as free for use
/// @param id The device ID to free
void ReleaseDeviceID(device_id_t id){
    SetBitmapBit(deviceRegistry.bitmap, id);
}

/// @brief Create the system's device registry
/// @return Success or error code
int CreateDeviceRegistry(){
    deviceRegistry.bitmap = halloc(REGISTRY_BITMAP_SIZE * sizeof(uint32_t));
    if(deviceRegistry.bitmap == NULL){
        return STANDARD_FAILURE;
    }
    memset(deviceRegistry.bitmap, 0, REGISTRY_BITMAP_SIZE * sizeof(uint32_t));

    deviceRegistry.devices = halloc(DEFAULT_REGISTRY_ARRAY_SIZE);
    if(deviceRegistry.devices == NULL){
        return STANDARD_FAILURE;
    }
    memset(deviceRegistry.devices, 0, DEFAULT_REGISTRY_ARRAY_SIZE);
    deviceRegistry.devArrSize = DEFAULT_REGISTRY_ARRAY_SIZE;

    deviceRegistry.numDevices = 0;

    return STANDARD_SUCCESS;
}

/// @brief Clear the system's device registry
void DestroyDeviceRegistry(){
    hfree(deviceRegistry.bitmap);
    for(index_t i = 0; i < deviceRegistry.devArrSize; i++){
        if(deviceRegistry.devices[i] != NULL){
            UnregisterDevice(deviceRegistry.devices[i]);
        }
    }
    hfree(deviceRegistry.devices);
    memset(&deviceRegistry, 0, sizeof(device_registry_t));
}

/// @brief Takes a pointer to a device that a driver has constructed, copies it, initializes it, and adds it to the VFS.
/// @param device The device created by the driver
/// @param path The (absolute) path the device should be located in. Must be in /dev.
/// @param permissions The permissions of the new device
/// @return Success or error code
dresult_t RegisterDevice(device_t* device, char* path, int permissions){
    if(strncmp(path, "/dev", 4) != 0){
        // Devices are only allowed in the /dev directory!
        return DRIVER_ACCESS_DENIED;
    }

    if(deviceRegistry.numDevices == UINT16_MAX){
        return DRIVER_REGISTRY_FULL;
    }

    if(device == NULL || path == NULL || permissions == 0){
        return DRIVER_INVALID_ARGUMENT;
    }

    device->id = AllocateDeviceID();
    if(device->id > UINT16_MAX){
        return DRIVER_FAILURE;
    }

    device->lock = MUTEX_INIT;

    // Extract the device name from the given path and separate them
    char* devName = strrchr(path, '/');
    *devName = '\0';
    devName++;

    //printk("Registering device %s with ID %d\n", devName, device->id);

    // Create the new device node
    device->node = VfsAddDevice(device, devName, path, permissions);
    if(device->node == NULL){
        ReleaseDeviceID(device->id);
        //printk("Failed to create device node %s\n", path);
        //STOP
        return DRIVER_FAILURE;
    }

    //printk("Adding device node %s to the device registry\n", path);
    // Add the device to the device registry
    if(deviceRegistry.devArrSize > (size_t)device->id){
        if(deviceRegistry.devArrSize == 0){
            deviceRegistry.devArrSize++;
        }
        deviceRegistry.devices = rehalloc(deviceRegistry.devices, deviceRegistry.devArrSize * 2);
        deviceRegistry.devArrSize *= 2;
    }
    deviceRegistry.devices[device->id] = device;
    deviceRegistry.numDevices++;

    return DRIVER_SUCCESS;
}

/// @brief Unregister a device from the system
/// @param device The device to unregister
int UnregisterDevice(device_t* device){
    VfsRemoveNode(device->node);
    deviceRegistry.devices[device->id] = NULL;
    deviceRegistry.numDevices--;
    ReleaseDeviceID(device->id);
    hfree(device->name);
    hfree(device);
    return 0;
}

/// @brief Create the system driver registry for driver lookup
/// @return Success or error code
int CreateDriverRegistry(){
    driverRegistry.bitmap = halloc(REGISTRY_BITMAP_SIZE * sizeof(uint32_t));
    if(driverRegistry.bitmap == NULL){
        return STANDARD_FAILURE;
    }
    memset(driverRegistry.bitmap, 0, REGISTRY_BITMAP_SIZE * sizeof(uint32_t));

    driverRegistry.drivers = halloc(sizeof(driver_t*) * DEFAULT_REGISTRY_ARRAY_SIZE);
    if(driverRegistry.drivers == NULL){
        return STANDARD_FAILURE;
    }
    memset(driverRegistry.drivers, 0, sizeof(driver_t*) * DEFAULT_REGISTRY_ARRAY_SIZE);
    driverRegistry.drvArrSize = DEFAULT_REGISTRY_ARRAY_SIZE;

    driverRegistry.numDrivers = 0;

    return STANDARD_SUCCESS;
}

/// @brief Destroy the system's driver registry
void DestroyDriverRegistry(){
    hfree(driverRegistry.bitmap);
    for(index_t i = 0; i < driverRegistry.drvArrSize; i++){
        if(driverRegistry.drivers[i] != NULL){
            UnregisterDriver(driverRegistry.drivers[i]);
        }
    }
    hfree(driverRegistry.drivers);
    memset(&driverRegistry, 0, sizeof(driver_registry_t));
}

/// @brief Register a driver with the system (does load it)
/// @warning Make sure the kernel calls the driver's init function!
/// @param driver A pointer to the driver struct that the driver has created
/// @param inKernel Whether or not the driver is directly part of the kernel's binary (those drivers will call this function directly)
/// @return Success or error code
dresult_t RegisterDriver(driver_t* driver, bool inKernel){
    if(driver == NULL){
        // The driver structure must be properly initialized by the driver!
        return DRIVER_INVALID_ARGUMENT;
    }

    if(driverRegistry.numDrivers == UINT16_MAX){
        return DRIVER_REGISTRY_FULL;
    }

    // Add the driver to the driver registry
    modid_t arrIndex = FindIndex(driverRegistry.bitmap, REGISTRY_BITMAP_SIZE);
    driver->id = arrIndex;
    if(driverRegistry.drvArrSize > arrIndex){
        driverRegistry.drivers = rehalloc(driverRegistry.drivers, driverRegistry.drvArrSize * 2);
        driverRegistry.drvArrSize *= 2;
    }
    ClearBitmapBit(driverRegistry.bitmap, arrIndex);
    driverRegistry.drivers[arrIndex] = driver;
    driverRegistry.numDrivers++;

    if(inKernel){
        driver->inKernel = true;
        return DRIVER_SUCCESS;
    } else {
        driver->inKernel = false;
    }

    // Call a driver load function...

    return DRIVER_SUCCESS;
}

/// @brief Unregister a driver from the system (does unload it)
/// @param driver The driver to unregister
int UnregisterDriver(driver_t* driver){
    driver->deinit();
    SetBitmapBit(driverRegistry.bitmap, driver->id);
    driverRegistry.drivers[driver->id] = NULL;
    hfree(driver);
    driverRegistry.numDrivers--;

    if(driver->inKernel){
        return DRIVER_SUCCESS;
    }
    // Call a driver unload function...
    return DRIVER_FAILURE;
}

// Find a driver compatible with the given device
dresult_t FindDriver(device_t* device){
    // Just linearly search
    for(index_t i = 0; i < driverRegistry.drvArrSize / 4; i++){
        if(driverRegistry.drivers[i] != NULL){
            if(driverRegistry.drivers[i]->probe(device->id, device->class, device->type) == DRIVER_SUCCESS){
                // This only works for ring 0 drivers. I'll need to context switch to user-space drivers...
                device->driver = driverRegistry.drivers[i];
                return DRIVER_SUCCESS;
            }
        }
    }

    // No supported driver found, remove it from the registry
    UnregisterDevice(device);
    return DRIVER_FAILURE;
}

fs_driver_t* FindFsDriver(device_t* device){
    for(index_t i = 0; i < driverRegistry.drvArrSize / 4; i++){
        if(driverRegistry.drivers[i] != NULL){
            if(driverRegistry.drivers[i]->probe(device->id, device->class, device->type) == DRIVER_SUCCESS && (driverRegistry.drivers[i]->type & DEVICE_TYPE_FILESYSTEM)){
                // This only works for ring 0 drivers. I'll need to context switch to user-space drivers...
                return (fs_driver_t*)driverRegistry.drivers[i];
            }
        }
    }
    return NULL;
}

/// @brief Obtain a pointer to a device based on its device ID
/// @param device The device ID to locate
/// @return Pointer to the device with the corresponding ID, or NULL if none.
device_t* GetDeviceByID(device_id_t device){
    // This is, thankfully, very simple
    if((size_t)device > deviceRegistry.devArrSize / sizeof(device_t*)){
        return NULL;
    }
    return deviceRegistry.devices[device];
}

size_t GetNumDevices(){
    return deviceRegistry.numDevices;
}

uint32_t loadedModules = 0;
void* moduleRegionStart = NULL;

int LoadModule(char* path){
    // Load a module from the given path
    // This is a placeholder for now, but it should be implemented in the future
    return STANDARD_FAILURE;
}

int UnloadModule(modid_t driver){
    // Unload a module
    // This is a placeholder for now, but it should be implemented in the future
    return 0;
}