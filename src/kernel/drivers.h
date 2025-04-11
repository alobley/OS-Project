#ifndef DRIVERS_H
#define DRIVERS_H

// An extension to system.h specifically for device drivers

#include <devices.h>
#include <system.h>

// Load a module
DRIVERSTATUS module_load(driver_t* this, device_t* device);

// Unload a module
DRIVERSTATUS module_unload(driver_t* this, device_t* device);

// Add a device to the VFS
int add_vfs_device(device_t* device, const char* name, const char* path);

// Query a driver to see if it supports a given device
DRIVERSTATUS query_module(driver_t* this, device_t* device);

// Find a module by its supported device type
driver_t* find_module(device_t* device, DEVICE_TYPE type);

// Register a device
DRIVERSTATUS register_device(device_t* device);

// Unregister a device
DRIVERSTATUS unregister_device(device_t* device);

// Get a device from a given path
device_t* get_device(char* path);

// Request an IRQ from the operating system
int request_irq(unsigned int irqno, void(*handler)(struct Registers* regs));

// Release an IRQ from the operating system
int release_irq(unsigned int irqno);

// Page a specific physical address into memory
int page_physical_address(physaddr_t paddr, virtaddr_t vaddr, unsigned int flags, size_t size);

// Unpage a specific virtual address from memory
int unpage_physical_address(virtaddr_t vaddr, size_t size);

/// @brief Read from an I/O port
/// @param port The I/O port to read from
/// @param size The number of bits to read (8, 16, or 32)
/// @return the value read as a 32-bit integer, regardless of actual size (returns DRIVER_INVALID_ARGUMENT on invalid size)
DRIVERSTATUS io_port_read(unsigned short port, unsigned int size);

/// @brief Write a value to an I/O port
/// @param port The I/O port to write to
/// @param size The number of bits to write (8, 16, or 32)
/// @param value The value to write (concatenated automatically depending on the size of the write)
/// @return Success or error code
DRIVERSTATUS io_port_write(unsigned short port, unsigned int size, unsigned int value);

#endif  // DRIVERS_H