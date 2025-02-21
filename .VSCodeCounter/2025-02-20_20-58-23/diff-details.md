# Diff Details

Date : 2025-02-20 20:58:23

Directory /home/andrew/osdev

Total : 46 files,  1327 codes, 283 comments, 242 blanks, all 1852 lines

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 9 | 1 | 1 | 11 |
| [readme.md](/readme.md) | Markdown | 1 | 0 | 0 | 1 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 104 | 13 | 21 | 138 |
| [src/acpi/acpi.h](/src/acpi/acpi.h) | C++ | 129 | 5 | 20 | 154 |
| [src/console/console.c](/src/console/console.c) | C | 0 | 4 | 0 | 4 |
| [src/datastructures/btree.c](/src/datastructures/btree.c) | C | 1 | 0 | 0 | 1 |
| [src/datastructures/btree.h](/src/datastructures/btree.h) | C++ | 13 | 0 | 4 | 17 |
| [src/datastructures/hashtable.c](/src/datastructures/hashtable.c) | C | 72 | 5 | 16 | 93 |
| [src/datastructures/hashtable.h](/src/datastructures/hashtable.h) | C++ | 20 | 0 | 5 | 25 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 0 | 0 | 1 | 1 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 3 | 1 | 2 | 6 |
| [src/disk/disk.c](/src/disk/disk.c) | C | 10 | 6 | 8 | 24 |
| [src/disk/disk.h](/src/disk/disk.h) | C++ | 22 | 2 | 4 | 28 |
| [src/disk/ide.c](/src/disk/ide.c) | C | 0 | 0 | -1 | -1 |
| [src/disk/ide.h](/src/disk/ide.h) | C++ | -3 | -1 | -3 | -7 |
| [src/drivers/ata/ata.c](/src/drivers/ata/ata.c) | C | 18 | 0 | 5 | 23 |
| [src/drivers/ata/ata.h](/src/drivers/ata/ata.h) | C++ | 9 | 1 | 4 | 14 |
| [src/drivers/drivers.md](/src/drivers/drivers.md) | Markdown | 1 | 0 | 0 | 1 |
| [src/drivers/filesystems/ramfs.c](/src/drivers/filesystems/ramfs.c) | C | 1 | 0 | 0 | 1 |
| [src/drivers/filesystems/ramfs.h](/src/drivers/filesystems/ramfs.h) | C++ | 3 | 0 | 3 | 6 |
| [src/drivers/filesystems/vfs/vfs.c](/src/drivers/filesystems/vfs/vfs.c) | C | 171 | 29 | 38 | 238 |
| [src/drivers/filesystems/vfs/vfs.h](/src/drivers/filesystems/vfs/vfs.h) | C++ | 36 | 1 | 6 | 43 |
| [src/drivers/graphics/vesa.c](/src/drivers/graphics/vesa.c) | C | 14 | 0 | 2 | 16 |
| [src/drivers/graphics/vesa.h](/src/drivers/graphics/vesa.h) | C++ | 67 | 1 | 7 | 75 |
| [src/interrupts/interrupts.c](/src/interrupts/interrupts.c) | C | 100 | 91 | 16 | 207 |
| [src/kernel/devices.c](/src/kernel/devices.c) | C | 95 | 21 | 13 | 129 |
| [src/kernel/devices.h](/src/kernel/devices.h) | C++ | 101 | 11 | 16 | 128 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 8 | 76 | 17 | 101 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C | 44 | 4 | 5 | 53 |
| [src/kernel/kstart.asm](/src/kernel/kstart.asm) | NASM Assembly | -15 | -3 | -4 | -22 |
| [src/kernel/users.c](/src/kernel/users.c) | C | 14 | 0 | 1 | 15 |
| [src/kernel/users.h](/src/kernel/users.h) | C++ | 16 | 1 | 7 | 24 |
| [src/kernel/util.h](/src/kernel/util.h) | C++ | -42 | -1 | -14 | -57 |
| [src/lib/cmos.h](/src/lib/cmos.h) | C++ | 16 | 0 | 5 | 21 |
| [src/lib/string.h](/src/lib/string.h) | C++ | 1 | 0 | 0 | 1 |
| [src/lib/util.h](/src/lib/util.h) | C++ | 31 | 0 | 10 | 41 |
| [src/memory/alloc.c](/src/memory/alloc.c) | C | 0 | 1 | 1 | 2 |
| [src/memory/paging.c](/src/memory/paging.c) | C | 27 | 4 | 10 | 41 |
| [src/multitasking/multitasking.c](/src/multitasking/multitasking.c) | C | 2 | 2 | 2 | 6 |
| [src/multitasking/multitasking.h](/src/multitasking/multitasking.h) | C++ | 8 | 2 | 3 | 13 |
| [src/ps2/keyboard.c](/src/ps2/keyboard.c) | C | 4 | -1 | -1 | 2 |
| [src/time/time.c](/src/time/time.c) | C | 77 | 4 | 9 | 90 |
| [src/time/time.h](/src/time/time.h) | C++ | 10 | 0 | 3 | 13 |
| [src/userland/shell.c](/src/userland/shell.c) | C | 129 | 3 | 2 | 134 |
| [src/vfs/vfs.c](/src/vfs/vfs.c) | C | 0 | 0 | -1 | -1 |
| [src/vfs/vfs.h](/src/vfs/vfs.h) | C++ | 0 | 0 | -1 | -1 |

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details