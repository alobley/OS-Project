#ifndef ATA_H
#define ATA_H

#include <devices.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>
#include <disk.h>

// ATA (specifically PATA) driver
void InitializeATA();

#endif // ATA_H