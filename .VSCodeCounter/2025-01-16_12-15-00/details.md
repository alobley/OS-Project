# Details

Date : 2025-01-16 12:15:00

Directory /home/andrew/osdev

Total : 53 files,  4983 codes, 653 comments, 1056 blanks, all 6692 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 79 | 18 | 24 | 121 |
| [readme.md](/readme.md) | Markdown | 80 | 0 | 16 | 96 |
| [src/VGA/pixel.c](/src/VGA/pixel.c) | C | 28 | 0 | 5 | 33 |
| [src/VGA/pixel.h](/src/VGA/pixel.h) | C++ | 17 | 0 | 8 | 25 |
| [src/VGA/text.c](/src/VGA/text.c) | C | 231 | 4 | 42 | 277 |
| [src/VGA/text.h](/src/VGA/text.h) | C++ | 11 | 0 | 8 | 19 |
| [src/VGA/vga.c](/src/VGA/vga.c) | C | 515 | 46 | 33 | 594 |
| [src/VGA/vga.h](/src/VGA/vga.h) | C | 59 | 19 | 25 | 103 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 129 | 21 | 27 | 177 |
| [src/acpi/acpi.h](/src/acpi/acpi.h) | C++ | 120 | 4 | 18 | 142 |
| [src/boot/bios.h](/src/boot/bios.h) | C++ | 21 | 3 | 8 | 32 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 227 | 70 | 85 | 382 |
| [src/boot/bootutil.h](/src/boot/bootutil.h) | C++ | 41 | 1 | 17 | 59 |
| [src/boot/grub.cfg](/src/boot/grub.cfg) | Properties | 3 | 0 | 0 | 3 |
| [src/boot/stage1.asm](/src/boot/stage1.asm) | NASM Assembly | 189 | 39 | 59 | 287 |
| [src/boot/stage1.c](/src/boot/stage1.c) | C | 12 | 1 | 3 | 16 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 384 | 74 | 71 | 529 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 35 | 1 | 9 | 45 |
| [src/disk/fat.c](/src/disk/fat.c) | C | 296 | 52 | 71 | 419 |
| [src/disk/fat.h](/src/disk/fat.h) | C++ | 172 | 11 | 16 | 199 |
| [src/disk/vfs.c](/src/disk/vfs.c) | C | 96 | 10 | 18 | 124 |
| [src/disk/vfs.h](/src/disk/vfs.h) | C++ | 42 | 2 | 15 | 59 |
| [src/interrupts/idt.c](/src/interrupts/idt.c) | C | 33 | 0 | 6 | 39 |
| [src/interrupts/idt.h](/src/interrupts/idt.h) | C++ | 7 | 0 | 4 | 11 |
| [src/interrupts/irq.c](/src/interrupts/irq.c) | C | 59 | 0 | 11 | 70 |
| [src/interrupts/irq.h](/src/interrupts/irq.h) | C++ | 19 | 0 | 7 | 26 |
| [src/interrupts/isr.c](/src/interrupts/isr.c) | C | 201 | 19 | 15 | 235 |
| [src/interrupts/isr.h](/src/interrupts/isr.h) | C | 12 | 0 | 4 | 16 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 63 | 22 | 17 | 102 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 168 | 1 | 30 | 199 |
| [src/kernel/kish.c](/src/kernel/kish.c) | C | 176 | 11 | 29 | 216 |
| [src/kernel/multitasking.h](/src/kernel/multitasking.h) | C++ | 39 | 8 | 9 | 56 |
| [src/kernel/smallgame.c](/src/kernel/smallgame.c) | C | 49 | 10 | 12 | 71 |
| [src/keyboard/keyboard.c](/src/keyboard/keyboard.c) | C | 239 | 40 | 50 | 329 |
| [src/keyboard/keyboard.h](/src/keyboard/keyboard.h) | C++ | 151 | 6 | 18 | 175 |
| [src/lib/fpu.c](/src/lib/fpu.c) | C | 13 | 0 | 2 | 15 |
| [src/lib/fpu.h](/src/lib/fpu.h) | C++ | 5 | 0 | 2 | 7 |
| [src/lib/io.c](/src/lib/io.c) | C | 25 | 6 | 6 | 37 |
| [src/lib/io.h](/src/lib/io.h) | C++ | 10 | 0 | 3 | 13 |
| [src/lib/math.c](/src/lib/math.c) | C | 146 | 10 | 35 | 191 |
| [src/lib/math.h](/src/lib/math.h) | C++ | 16 | 0 | 4 | 20 |
| [src/lib/stdarg.h](/src/lib/stdarg.h) | C++ | 7 | 0 | 5 | 12 |
| [src/lib/string.h](/src/lib/string.h) | C++ | 83 | 9 | 25 | 117 |
| [src/lib/types.h](/src/lib/types.h) | C++ | 49 | 9 | 14 | 72 |
| [src/lib/util.h](/src/lib/util.h) | C++ | 33 | 2 | 13 | 48 |
| [src/memory/memmanage.c](/src/memory/memmanage.c) | C | 257 | 38 | 53 | 348 |
| [src/memory/memmanage.h](/src/memory/memmanage.h) | C++ | 55 | 4 | 11 | 70 |
| [src/memory/multiboot.h](/src/memory/multiboot.h) | C++ | 100 | 19 | 26 | 145 |
| [src/programs/prgm.asm](/src/programs/prgm.asm) | NASM Assembly | 14 | 4 | 4 | 22 |
| [src/sound/pcspkr.c](/src/sound/pcspkr.c) | C | 32 | 0 | 10 | 42 |
| [src/sound/pcspkr.h](/src/sound/pcspkr.h) | C | 11 | 0 | 3 | 14 |
| [src/time/time.c](/src/time/time.c) | C | 113 | 57 | 45 | 215 |
| [src/time/time.h](/src/time/time.h) | C++ | 11 | 2 | 5 | 18 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)