# Details

Date : 2025-02-20 20:58:23

Directory /home/andrew/osdev

Total : 65 files,  4103 codes, 570 comments, 849 blanks, all 5522 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 68 | 19 | 18 | 105 |
| [readme.md](/readme.md) | Markdown | 70 | 0 | 15 | 85 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 104 | 13 | 21 | 138 |
| [src/acpi/acpi.h](/src/acpi/acpi.h) | C++ | 129 | 5 | 20 | 154 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 101 | 10 | 30 | 141 |
| [src/boot/grub.cfg](/src/boot/grub.cfg) | Properties | 3 | 0 | 0 | 3 |
| [src/console/console.c](/src/console/console.c) | C | 217 | 22 | 36 | 275 |
| [src/console/console.h](/src/console/console.h) | C++ | 23 | 6 | 14 | 43 |
| [src/datastructures/btree.c](/src/datastructures/btree.c) | C | 1 | 0 | 0 | 1 |
| [src/datastructures/btree.h](/src/datastructures/btree.h) | C++ | 13 | 0 | 4 | 17 |
| [src/datastructures/hashtable.c](/src/datastructures/hashtable.c) | C | 72 | 5 | 16 | 93 |
| [src/datastructures/hashtable.h](/src/datastructures/hashtable.h) | C++ | 20 | 0 | 5 | 25 |
| [src/disk/ahci.c](/src/disk/ahci.c) | C | 0 | 0 | 1 | 1 |
| [src/disk/ahci.h](/src/disk/ahci.h) | C++ | 3 | 1 | 2 | 6 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 0 | 0 | 1 | 1 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 3 | 1 | 2 | 6 |
| [src/disk/disk.c](/src/disk/disk.c) | C | 11 | 6 | 8 | 25 |
| [src/disk/disk.h](/src/disk/disk.h) | C++ | 90 | 10 | 16 | 116 |
| [src/drivers/ata/ata.c](/src/drivers/ata/ata.c) | C | 18 | 0 | 5 | 23 |
| [src/drivers/ata/ata.h](/src/drivers/ata/ata.h) | C++ | 9 | 1 | 4 | 14 |
| [src/drivers/drivers.md](/src/drivers/drivers.md) | Markdown | 1 | 0 | 0 | 1 |
| [src/drivers/filesystems/ramfs.c](/src/drivers/filesystems/ramfs.c) | C | 1 | 0 | 0 | 1 |
| [src/drivers/filesystems/ramfs.h](/src/drivers/filesystems/ramfs.h) | C++ | 3 | 0 | 3 | 6 |
| [src/drivers/filesystems/vfs/vfs.c](/src/drivers/filesystems/vfs/vfs.c) | C | 171 | 29 | 38 | 238 |
| [src/drivers/filesystems/vfs/vfs.h](/src/drivers/filesystems/vfs/vfs.h) | C++ | 36 | 1 | 6 | 43 |
| [src/drivers/graphics/vesa.c](/src/drivers/graphics/vesa.c) | C | 14 | 0 | 2 | 16 |
| [src/drivers/graphics/vesa.h](/src/drivers/graphics/vesa.h) | C++ | 67 | 1 | 7 | 75 |
| [src/interrupts/interrupts.c](/src/interrupts/interrupts.c) | C | 314 | 95 | 36 | 445 |
| [src/interrupts/interrupts.h](/src/interrupts/interrupts.h) | C | 77 | 4 | 15 | 96 |
| [src/interrupts/pic.c](/src/interrupts/pic.c) | C | 50 | 7 | 11 | 68 |
| [src/kernel/devices.c](/src/kernel/devices.c) | C | 95 | 21 | 13 | 129 |
| [src/kernel/devices.h](/src/kernel/devices.h) | C++ | 101 | 11 | 16 | 128 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 74 | 81 | 40 | 195 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C | 63 | 5 | 12 | 80 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 167 | 1 | 30 | 198 |
| [src/kernel/multiboot.h](/src/kernel/multiboot.h) | C++ | 62 | 13 | 18 | 93 |
| [src/kernel/users.c](/src/kernel/users.c) | C | 14 | 0 | 1 | 15 |
| [src/kernel/users.h](/src/kernel/users.h) | C++ | 16 | 1 | 7 | 24 |
| [src/lib/cmos.h](/src/lib/cmos.h) | C++ | 16 | 0 | 5 | 21 |
| [src/lib/elf.h](/src/lib/elf.h) | C++ | 3 | 1 | 2 | 6 |
| [src/lib/fpu.h](/src/lib/fpu.h) | C++ | 16 | 0 | 4 | 20 |
| [src/lib/stdalign.h](/src/lib/stdalign.h) | C++ | 5 | 0 | 3 | 8 |
| [src/lib/stdarg.h](/src/lib/stdarg.h) | C++ | 8 | 0 | 4 | 12 |
| [src/lib/stdbool.h](/src/lib/stdbool.h) | C | 7 | 0 | 4 | 11 |
| [src/lib/stddef.h](/src/lib/stddef.h) | C++ | 15 | 0 | 6 | 21 |
| [src/lib/stdint.h](/src/lib/stdint.h) | C++ | 73 | 10 | 24 | 107 |
| [src/lib/string.h](/src/lib/string.h) | C++ | 205 | 22 | 24 | 251 |
| [src/lib/util.h](/src/lib/util.h) | C++ | 41 | 0 | 13 | 54 |
| [src/memory/alloc.c](/src/memory/alloc.c) | C | 118 | 27 | 15 | 160 |
| [src/memory/alloc.h](/src/memory/alloc.h) | C | 10 | 2 | 7 | 19 |
| [src/memory/paging.c](/src/memory/paging.c) | C | 185 | 39 | 59 | 283 |
| [src/memory/paging.h](/src/memory/paging.h) | C++ | 48 | 3 | 18 | 69 |
| [src/multitasking/multitasking.c](/src/multitasking/multitasking.c) | C | 108 | 18 | 27 | 153 |
| [src/multitasking/multitasking.h](/src/multitasking/multitasking.h) | C++ | 78 | 8 | 21 | 107 |
| [src/ps2/keyboard.c](/src/ps2/keyboard.c) | C | 278 | 45 | 54 | 377 |
| [src/ps2/keyboard.h](/src/ps2/keyboard.h) | C | 136 | 5 | 16 | 157 |
| [src/ps2/mouse.c](/src/ps2/mouse.c) | C | 0 | 0 | 1 | 1 |
| [src/ps2/mouse.h](/src/ps2/mouse.h) | C++ | 0 | 0 | 1 | 1 |
| [src/ps2/ps2.c](/src/ps2/ps2.c) | C | 1 | 0 | 0 | 1 |
| [src/ps2/ps2.h](/src/ps2/ps2.h) | C++ | 0 | 0 | 1 | 1 |
| [src/sound/pcspkr.c](/src/sound/pcspkr.c) | C | 32 | 0 | 10 | 42 |
| [src/sound/pcspkr.h](/src/sound/pcspkr.h) | C++ | 11 | 0 | 3 | 14 |
| [src/time/time.c](/src/time/time.c) | C | 168 | 9 | 30 | 207 |
| [src/time/time.h](/src/time/time.h) | C++ | 31 | 0 | 12 | 43 |
| [src/userland/shell.c](/src/userland/shell.c) | C | 229 | 12 | 12 | 253 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)