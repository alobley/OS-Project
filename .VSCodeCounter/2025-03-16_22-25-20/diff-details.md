# Diff Details

Date : 2025-03-16 22:25:20

Directory /home/andrew/osdev

Total : 69 files,  2617 codes, 393 comments, 553 blanks, all 3563 lines

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 14 | 1 | 3 | 18 |
| [readme.md](/readme.md) | Markdown | 0 | 0 | -2 | -2 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 6 | 0 | -1 | 5 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 37 | 22 | 8 | 67 |
| [src/boot/bootfs.h](/src/boot/bootfs.h) | C | 53 | 0 | 5 | 58 |
| [src/boot/bootutil.h](/src/boot/bootutil.h) | C | 180 | 3 | 45 | 228 |
| [src/boot/stage1.asm](/src/boot/stage1.asm) | NASM Assembly | 233 | 13 | 71 | 317 |
| [src/boot/stage2.c](/src/boot/stage2.c) | C | 105 | 15 | 28 | 148 |
| [src/console/console.h](/src/console/console.h) | C | 1 | 0 | 1 | 2 |
| [src/console/tty.c](/src/console/tty.c) | C | 113 | 6 | 18 | 137 |
| [src/console/tty.h](/src/console/tty.h) | C++ | 19 | 1 | 3 | 23 |
| [src/datastructures/btree.c](/src/datastructures/btree.c) | C | -1 | 0 | 0 | -1 |
| [src/datastructures/btree.h](/src/datastructures/btree.h) | C++ | -13 | 0 | -4 | -17 |
| [src/datastructures/hash.c](/src/datastructures/hash.c) | C | 74 | 2 | 5 | 81 |
| [src/datastructures/hash.h](/src/datastructures/hash.h) | C | 17 | 1 | 6 | 24 |
| [src/datastructures/hashtable.c](/src/datastructures/hashtable.c) | C | -72 | -5 | -16 | -93 |
| [src/datastructures/hashtable.h](/src/datastructures/hashtable.h) | C++ | -20 | 0 | -5 | -25 |
| [src/disk/ahci.c](/src/disk/ahci.c) | C | 0 | 0 | -1 | -1 |
| [src/disk/ahci.h](/src/disk/ahci.h) | C++ | -3 | -1 | -2 | -6 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 511 | 76 | 88 | 675 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 8 | -1 | 3 | 10 |
| [src/disk/disk.c](/src/disk/disk.c) | C | -11 | -6 | -8 | -25 |
| [src/disk/disk.h](/src/disk/disk.h) | C++ | -90 | -10 | -16 | -116 |
| [src/disk/mbr.c](/src/disk/mbr.c) | C | 79 | 4 | 7 | 90 |
| [src/disk/mbr.h](/src/disk/mbr.h) | C++ | 48 | 3 | 9 | 60 |
| [src/drivers/ata/ata.c](/src/drivers/ata/ata.c) | C | -18 | 0 | -5 | -23 |
| [src/drivers/ata/ata.h](/src/drivers/ata/ata.h) | C++ | -9 | -1 | -4 | -14 |
| [src/drivers/drivers.md](/src/drivers/drivers.md) | Markdown | -1 | 0 | 0 | -1 |
| [src/drivers/filesystems/ramfs.c](/src/drivers/filesystems/ramfs.c) | C | -1 | 0 | 0 | -1 |
| [src/drivers/filesystems/ramfs.h](/src/drivers/filesystems/ramfs.h) | C++ | -3 | 0 | -3 | -6 |
| [src/drivers/filesystems/vfs/vfs.c](/src/drivers/filesystems/vfs/vfs.c) | C | -171 | -29 | -38 | -238 |
| [src/drivers/filesystems/vfs/vfs.h](/src/drivers/filesystems/vfs/vfs.h) | C++ | -36 | -1 | -6 | -43 |
| [src/drivers/graphics/vesa.c](/src/drivers/graphics/vesa.c) | C | -14 | 0 | -2 | -16 |
| [src/drivers/graphics/vesa.h](/src/drivers/graphics/vesa.h) | C++ | -67 | -1 | -7 | -75 |
| [src/filesystems/fat.c](/src/filesystems/fat.c) | C | 52 | 17 | 9 | 78 |
| [src/filesystems/fat.h](/src/filesystems/fat.h) | C++ | 5 | 0 | 3 | 8 |
| [src/interrupts/interrupts.c](/src/interrupts/interrupts.c) | C | 321 | 117 | 39 | 477 |
| [src/interrupts/interrupts.h](/src/interrupts/interrupts.h) | C | 1 | 0 | 0 | 1 |
| [src/interrupts/pic.c](/src/interrupts/pic.c) | C | 17 | -7 | 4 | 14 |
| [src/kernel/devices.c](/src/kernel/devices.c) | C | 130 | -13 | 7 | 124 |
| [src/kernel/devices.h](/src/kernel/devices.h) | C | 98 | 41 | 48 | 187 |
| [src/kernel/drivers.c](/src/kernel/drivers.c) | C | 97 | 0 | 17 | 114 |
| [src/kernel/drivers.h](/src/kernel/drivers.h) | C++ | 20 | 22 | 17 | 59 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 90 | 29 | 25 | 144 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C | -4 | 2 | -2 | -4 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 2 | 0 | 0 | 2 |
| [src/kernel/system.c](/src/kernel/system.c) | C | 155 | 0 | 27 | 182 |
| [src/kernel/system.h](/src/kernel/system.h) | C | 103 | 32 | 42 | 177 |
| [src/lib/char.h](/src/lib/char.h) | C++ | 10 | 0 | 3 | 13 |
| [src/lib/common.h](/src/lib/common.h) | C | 10 | 0 | 2 | 12 |
| [src/lib/elf.h](/src/lib/elf.h) | C++ | 51 | 3 | 8 | 62 |
| [src/lib/kio.c](/src/lib/kio.c) | C | 4 | 0 | 2 | 6 |
| [src/lib/kio.h](/src/lib/kio.h) | C++ | 26 | 3 | 11 | 40 |
| [src/lib/stdint.h](/src/lib/stdint.h) | C | -1 | 0 | -1 | -2 |
| [src/lib/string.h](/src/lib/string.h) | C | 1 | 0 | 1 | 2 |
| [src/lib/util.h](/src/lib/util.h) | C | 7 | 0 | 6 | 13 |
| [src/memory/alloc.c](/src/memory/alloc.c) | C | 20 | 5 | 8 | 33 |
| [src/memory/alloc.h](/src/memory/alloc.h) | C | 1 | 0 | 0 | 1 |
| [src/memory/paging.c](/src/memory/paging.c) | C | 28 | 2 | 6 | 36 |
| [src/memory/paging.h](/src/memory/paging.h) | C++ | 1 | 0 | 0 | 1 |
| [src/multitasking/multitasking.c](/src/multitasking/multitasking.c) | C | 39 | 5 | 9 | 53 |
| [src/multitasking/multitasking.h](/src/multitasking/multitasking.h) | C | 10 | 2 | 0 | 12 |
| [src/ps2/keyboard.c](/src/ps2/keyboard.c) | C | 16 | 1 | 6 | 23 |
| [src/ps2/keyboard.h](/src/ps2/keyboard.h) | C | -7 | 0 | -2 | -9 |
| [src/time/time.c](/src/time/time.c) | C | 0 | -1 | 0 | -1 |
| [src/time/time.h](/src/time/time.h) | C++ | -7 | 0 | -1 | -8 |
| [src/userland/shell.c](/src/userland/shell.c) | C | 48 | 0 | 22 | 70 |
| [src/vfs/vfs.c](/src/vfs/vfs.c) | C | 252 | 39 | 46 | 337 |
| [src/vfs/vfs.h](/src/vfs/vfs.h) | C | 53 | 2 | 11 | 66 |

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details