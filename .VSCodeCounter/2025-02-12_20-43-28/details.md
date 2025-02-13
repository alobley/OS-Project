# Details

Date : 2025-02-12 20:43:28

Directory /home/andrew/osdev

Total : 49 files,  2776 codes, 287 comments, 607 blanks, all 3670 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 59 | 18 | 17 | 94 |
| [readme.md](/readme.md) | Markdown | 69 | 0 | 15 | 84 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 101 | 10 | 30 | 141 |
| [src/boot/grub.cfg](/src/boot/grub.cfg) | Properties | 3 | 0 | 0 | 3 |
| [src/console/console.c](/src/console/console.c) | C | 217 | 18 | 36 | 271 |
| [src/console/console.h](/src/console/console.h) | C++ | 23 | 6 | 14 | 43 |
| [src/disk/ahci.c](/src/disk/ahci.c) | C | 0 | 0 | 1 | 1 |
| [src/disk/ahci.h](/src/disk/ahci.h) | C++ | 3 | 1 | 2 | 6 |
| [src/disk/disk.c](/src/disk/disk.c) | C | 1 | 0 | 0 | 1 |
| [src/disk/disk.h](/src/disk/disk.h) | C++ | 68 | 8 | 12 | 88 |
| [src/disk/ide.c](/src/disk/ide.c) | C | 0 | 0 | 1 | 1 |
| [src/disk/ide.h](/src/disk/ide.h) | C++ | 3 | 1 | 3 | 7 |
| [src/interrupts/interrupts.c](/src/interrupts/interrupts.c) | C | 214 | 4 | 20 | 238 |
| [src/interrupts/interrupts.h](/src/interrupts/interrupts.h) | C | 77 | 4 | 15 | 96 |
| [src/interrupts/pic.c](/src/interrupts/pic.c) | C | 50 | 7 | 11 | 68 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 66 | 5 | 23 | 94 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C++ | 19 | 1 | 7 | 27 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 167 | 1 | 30 | 198 |
| [src/kernel/kstart.asm](/src/kernel/kstart.asm) | NASM Assembly | 15 | 3 | 4 | 22 |
| [src/kernel/multiboot.h](/src/kernel/multiboot.h) | C++ | 62 | 13 | 18 | 93 |
| [src/kernel/util.h](/src/kernel/util.h) | C++ | 42 | 1 | 14 | 57 |
| [src/lib/elf.h](/src/lib/elf.h) | C++ | 3 | 1 | 2 | 6 |
| [src/lib/fpu.h](/src/lib/fpu.h) | C++ | 16 | 0 | 4 | 20 |
| [src/lib/stdalign.h](/src/lib/stdalign.h) | C++ | 5 | 0 | 3 | 8 |
| [src/lib/stdarg.h](/src/lib/stdarg.h) | C++ | 8 | 0 | 4 | 12 |
| [src/lib/stdbool.h](/src/lib/stdbool.h) | C | 7 | 0 | 4 | 11 |
| [src/lib/stddef.h](/src/lib/stddef.h) | C++ | 15 | 0 | 6 | 21 |
| [src/lib/stdint.h](/src/lib/stdint.h) | C++ | 73 | 10 | 24 | 107 |
| [src/lib/string.h](/src/lib/string.h) | C++ | 204 | 22 | 24 | 250 |
| [src/lib/util.h](/src/lib/util.h) | C++ | 10 | 0 | 3 | 13 |
| [src/memory/alloc.c](/src/memory/alloc.c) | C | 118 | 26 | 14 | 158 |
| [src/memory/alloc.h](/src/memory/alloc.h) | C | 10 | 2 | 7 | 19 |
| [src/memory/paging.c](/src/memory/paging.c) | C | 158 | 35 | 49 | 242 |
| [src/memory/paging.h](/src/memory/paging.h) | C++ | 48 | 3 | 18 | 69 |
| [src/multitasking/multitasking.c](/src/multitasking/multitasking.c) | C | 106 | 16 | 25 | 147 |
| [src/multitasking/multitasking.h](/src/multitasking/multitasking.h) | C++ | 70 | 6 | 18 | 94 |
| [src/ps2/keyboard.c](/src/ps2/keyboard.c) | C | 274 | 46 | 55 | 375 |
| [src/ps2/keyboard.h](/src/ps2/keyboard.h) | C | 136 | 5 | 16 | 157 |
| [src/ps2/mouse.c](/src/ps2/mouse.c) | C | 0 | 0 | 1 | 1 |
| [src/ps2/mouse.h](/src/ps2/mouse.h) | C++ | 0 | 0 | 1 | 1 |
| [src/ps2/ps2.c](/src/ps2/ps2.c) | C | 1 | 0 | 0 | 1 |
| [src/ps2/ps2.h](/src/ps2/ps2.h) | C++ | 0 | 0 | 1 | 1 |
| [src/sound/pcspkr.c](/src/sound/pcspkr.c) | C | 32 | 0 | 10 | 42 |
| [src/sound/pcspkr.h](/src/sound/pcspkr.h) | C++ | 11 | 0 | 3 | 14 |
| [src/time/time.c](/src/time/time.c) | C | 91 | 5 | 21 | 117 |
| [src/time/time.h](/src/time/time.h) | C++ | 21 | 0 | 9 | 30 |
| [src/userland/shell.c](/src/userland/shell.c) | C | 100 | 9 | 10 | 119 |
| [src/vfs/vfs.c](/src/vfs/vfs.c) | C | 0 | 0 | 1 | 1 |
| [src/vfs/vfs.h](/src/vfs/vfs.h) | C++ | 0 | 0 | 1 | 1 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)