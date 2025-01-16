#ifndef BOOTUTIL_H
#define BOOTUTIL_H

#define STOP __asm__("cli\nhlt")

#define clc __asm__("clc")

// Register manipulation
#define getax(val) __asm__("mov %%ax, %0" : "=r"(val))
#define setax(val) __asm__("mov %0, %%ax" : : "r"(val))

#define getbx(val) __asm__("mov %%bx, %0" : "=r"(val))
#define setbx(val) __asm__("mov %0, %%bx" : : "r"(val))

#define getcx(val) __asm__("mov %%cx, %0" : "=r"(val))
#define setcx(val) __asm__("mov %0, %%cx" : : "r"(val))

#define getdx(val) __asm__("mov %%dx, %0" : "=r"(val))
#define setdx(val) __asm__("mov %0, %%dx" : : "r"(val))

#define getsi(val) __asm__("mov %%si, %0" : "=r"(val))
#define setsi(val) __asm__("mov %0, %%si" : : "r"(val))

#define getdi(val) __asm__("mov %%di, %0" : "=r"(val))
#define setdi(val) __asm__("mov %0, %%di" : : "r"(val))

#define getbp(val) __asm__("mov %%bp, %0" : "=r"(val))
#define setbp(val) __asm__("mov %0, %%bp" : : "r"(val))

#define getsp(val) __asm__("mov %%sp, %0" : "=r"(val))
#define setsp(val) __asm__("mov %0, %%sp" : : "r"(val))

#define setes(val) __asm__("mov %0, %%es" : : "r"(val))
#define setfs(val) __asm__("mov %0, %%fs" : : "r"(val))
#define setgs(val) __asm__("mov %0, %%gs" : : "r"(val))
#define setss(val) __asm__("mov %0, %%ss" : : "r"(val))
#define setds(val) __asm__("mov %0, %%ds" : : "r"(val))
#define setcs(val) __asm__("mov %0, %%cs" : : "r"(val))

#define getflags(val) __asm__("pushf\npop %0" : "=r"(val))
#define carryflag(eflags) (eflags & 0x1)

typedef unsigned char bool;
#define true 1
#define false 0

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
typedef signed long long int64;

typedef unsigned long size_t;

typedef struct fat12_entry {
    char filename[8];
    char ext[3];
    char attributes;
    char reserved;
    char creation_time_tenths;
    uint16 creation_time;
    uint16 creation_date;
    uint16 last_access_date;
    uint16 first_cluster_high;
    uint16 last_mod_time;
    uint16 last_mod_date;
    uint16 first_cluster_low;
    uint32 file_size;
} __attribute__((packed)) fat12_entry_t;

typedef struct fat12_boot_sector {
    char jmp[3];
    char oem[8];
    uint16 bytes_per_sector;
    uint8 sectors_per_cluster;
    uint16 reserved_sectors;
    uint8 fat_count;
    uint16 root_dir_entries;
    uint16 total_sectors;
    uint8 media_descriptor;
    uint16 sectors_per_fat;
    uint16 sectors_per_track;
    uint16 heads;
    uint32 hidden_sectors;
    uint32 total_sectors_large;
    uint8 driveNum;
    uint8 reserved;
    uint8 signature;
    uint32 volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) fat12_boot_sector_t;

#endif