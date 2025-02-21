#include <devices.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>
#include <disk.h>

#define ATA_PIMARY_BASE 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_PRIMARY_STATUS 0x3F6

#define ATA_SECONDARY_BASE 0x170
#define ATA_SECONDARY_CONTROL 0x376
#define ATA_SECONDARY_STATUS 0x376

#define ATA_TERTIARY_BASE 0x1E8
#define ATA_TERTIARY_CONTROL 0x3E6
#define ATA_TERTIARY_STATUS 0x3E6

#define ATA_QUATERNARY_BASE 0x168
#define ATA_QUATERNARY_CONTROL 0x366
#define ATA_QUATERNARY_STATUS 0x366
