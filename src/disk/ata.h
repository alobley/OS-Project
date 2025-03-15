#ifndef ATA_H
#define ATA_H

#include <util.h>
#include <stdint.h>
#include <stddef.h>
#include <devices.h>

#define PRIMARY_ATA_IRQ 14
#define SECONDARY_ATA_IRQ 15

#define MAX_ATA_DRIVES 8

DRIVERSTATUS InitializeAta();

#endif