# Details

Date : 2025-03-16 22:25:20

Directory /home/andrew/osdev

Total : 70 files,  6720 codes, 963 comments, 1402 blanks, all 9085 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 82 | 20 | 21 | 123 |
| [readme.md](/readme.md) | Markdown | 70 | 0 | 13 | 83 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 110 | 13 | 20 | 143 |
| [src/acpi/acpi.h](/src/acpi/acpi.h) | C | 129 | 5 | 20 | 154 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 138 | 32 | 38 | 208 |
| [src/boot/bootfs.h](/src/boot/bootfs.h) | C | 53 | 0 | 5 | 58 |
| [src/boot/bootutil.h](/src/boot/bootutil.h) | C | 180 | 3 | 45 | 228 |
| [src/boot/grub.cfg](/src/boot/grub.cfg) | Properties | 3 | 0 | 0 | 3 |
| [src/boot/stage1.asm](/src/boot/stage1.asm) | NASM Assembly | 233 | 13 | 71 | 317 |
| [src/boot/stage2.c](/src/boot/stage2.c) | C | 105 | 15 | 28 | 148 |
| [src/console/console.c](/src/console/console.c) | C | 217 | 22 | 36 | 275 |
| [src/console/console.h](/src/console/console.h) | C | 24 | 6 | 15 | 45 |
| [src/console/tty.c](/src/console/tty.c) | C | 113 | 6 | 18 | 137 |
| [src/console/tty.h](/src/console/tty.h) | C++ | 19 | 1 | 3 | 23 |
| [src/datastructures/hash.c](/src/datastructures/hash.c) | C | 74 | 2 | 5 | 81 |
| [src/datastructures/hash.h](/src/datastructures/hash.h) | C | 17 | 1 | 6 | 24 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 511 | 76 | 89 | 676 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 11 | 0 | 5 | 16 |
| [src/disk/mbr.c](/src/disk/mbr.c) | C | 79 | 4 | 7 | 90 |
| [src/disk/mbr.h](/src/disk/mbr.h) | C++ | 48 | 3 | 9 | 60 |
| [src/filesystems/fat.c](/src/filesystems/fat.c) | C | 52 | 17 | 9 | 78 |
| [src/filesystems/fat.h](/src/filesystems/fat.h) | C++ | 5 | 0 | 3 | 8 |
| [src/interrupts/interrupts.c](/src/interrupts/interrupts.c) | C | 635 | 212 | 75 | 922 |
| [src/interrupts/interrupts.h](/src/interrupts/interrupts.h) | C | 78 | 4 | 15 | 97 |
| [src/interrupts/pic.c](/src/interrupts/pic.c) | C | 67 | 0 | 15 | 82 |
| [src/kernel/devices.c](/src/kernel/devices.c) | C | 225 | 8 | 20 | 253 |
| [src/kernel/devices.h](/src/kernel/devices.h) | C | 199 | 52 | 64 | 315 |
| [src/kernel/drivers.c](/src/kernel/drivers.c) | C | 97 | 0 | 17 | 114 |
| [src/kernel/drivers.h](/src/kernel/drivers.h) | C++ | 20 | 22 | 17 | 59 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 164 | 110 | 65 | 339 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C | 59 | 7 | 10 | 76 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 169 | 1 | 30 | 200 |
| [src/kernel/multiboot.h](/src/kernel/multiboot.h) | C | 62 | 13 | 18 | 93 |
| [src/kernel/system.c](/src/kernel/system.c) | C | 155 | 0 | 27 | 182 |
| [src/kernel/system.h](/src/kernel/system.h) | C | 103 | 32 | 42 | 177 |
| [src/kernel/users.c](/src/kernel/users.c) | C | 14 | 0 | 1 | 15 |
| [src/kernel/users.h](/src/kernel/users.h) | C++ | 16 | 1 | 7 | 24 |
| [src/lib/char.h](/src/lib/char.h) | C++ | 10 | 0 | 3 | 13 |
| [src/lib/cmos.h](/src/lib/cmos.h) | C++ | 16 | 0 | 5 | 21 |
| [src/lib/common.h](/src/lib/common.h) | C | 10 | 0 | 2 | 12 |
| [src/lib/elf.h](/src/lib/elf.h) | C++ | 54 | 4 | 10 | 68 |
| [src/lib/fpu.h](/src/lib/fpu.h) | C++ | 16 | 0 | 4 | 20 |
| [src/lib/kio.c](/src/lib/kio.c) | C | 4 | 0 | 2 | 6 |
| [src/lib/kio.h](/src/lib/kio.h) | C++ | 26 | 3 | 11 | 40 |
| [src/lib/stdalign.h](/src/lib/stdalign.h) | C++ | 5 | 0 | 3 | 8 |
| [src/lib/stdarg.h](/src/lib/stdarg.h) | C++ | 8 | 0 | 4 | 12 |
| [src/lib/stdbool.h](/src/lib/stdbool.h) | C++ | 7 | 0 | 4 | 11 |
| [src/lib/stddef.h](/src/lib/stddef.h) | C++ | 15 | 0 | 6 | 21 |
| [src/lib/stdint.h](/src/lib/stdint.h) | C | 72 | 10 | 23 | 105 |
| [src/lib/string.h](/src/lib/string.h) | C | 206 | 22 | 25 | 253 |
| [src/lib/util.h](/src/lib/util.h) | C | 48 | 0 | 19 | 67 |
| [src/memory/alloc.c](/src/memory/alloc.c) | C | 138 | 32 | 23 | 193 |
| [src/memory/alloc.h](/src/memory/alloc.h) | C | 11 | 2 | 7 | 20 |
| [src/memory/paging.c](/src/memory/paging.c) | C | 213 | 41 | 65 | 319 |
| [src/memory/paging.h](/src/memory/paging.h) | C++ | 49 | 3 | 18 | 70 |
| [src/multitasking/multitasking.c](/src/multitasking/multitasking.c) | C | 147 | 23 | 36 | 206 |
| [src/multitasking/multitasking.h](/src/multitasking/multitasking.h) | C | 88 | 10 | 21 | 119 |
| [src/ps2/keyboard.c](/src/ps2/keyboard.c) | C | 294 | 46 | 60 | 400 |
| [src/ps2/keyboard.h](/src/ps2/keyboard.h) | C | 129 | 5 | 14 | 148 |
| [src/ps2/mouse.c](/src/ps2/mouse.c) | C | 0 | 0 | 1 | 1 |
| [src/ps2/mouse.h](/src/ps2/mouse.h) | C++ | 0 | 0 | 1 | 1 |
| [src/ps2/ps2.c](/src/ps2/ps2.c) | C | 1 | 0 | 0 | 1 |
| [src/ps2/ps2.h](/src/ps2/ps2.h) | C++ | 0 | 0 | 1 | 1 |
| [src/sound/pcspkr.c](/src/sound/pcspkr.c) | C | 32 | 0 | 10 | 42 |
| [src/sound/pcspkr.h](/src/sound/pcspkr.h) | C++ | 11 | 0 | 3 | 14 |
| [src/time/time.c](/src/time/time.c) | C | 168 | 8 | 30 | 206 |
| [src/time/time.h](/src/time/time.h) | C++ | 24 | 0 | 11 | 35 |
| [src/userland/shell.c](/src/userland/shell.c) | C | 277 | 12 | 34 | 323 |
| [src/vfs/vfs.c](/src/vfs/vfs.c) | C | 252 | 39 | 46 | 337 |
| [src/vfs/vfs.h](/src/vfs/vfs.h) | C | 53 | 2 | 11 | 66 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)