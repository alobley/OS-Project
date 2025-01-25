# Details

Date : 2025-01-19 11:28:01

Directory /home/andrew/osdev

Total : 55 files,  5339 codes, 702 comments, 1141 blanks, all 7182 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [makefile](/makefile) | Makefile | 67 | 17 | 18 | 102 |
| [readme.md](/readme.md) | Markdown | 80 | 0 | 16 | 96 |
| [src/VGA/pixel.c](/src/VGA/pixel.c) | C | 28 | 0 | 5 | 33 |
| [src/VGA/pixel.h](/src/VGA/pixel.h) | C++ | 17 | 0 | 8 | 25 |
| [src/VGA/text.c](/src/VGA/text.c) | C | 240 | 4 | 44 | 288 |
| [src/VGA/text.h](/src/VGA/text.h) | C++ | 11 | 0 | 8 | 19 |
| [src/VGA/vga.c](/src/VGA/vga.c) | C | 515 | 48 | 35 | 598 |
| [src/VGA/vga.h](/src/VGA/vga.h) | C | 59 | 19 | 25 | 103 |
| [src/acpi/acpi.c](/src/acpi/acpi.c) | C | 127 | 20 | 27 | 174 |
| [src/acpi/acpi.h](/src/acpi/acpi.h) | C++ | 122 | 4 | 19 | 145 |
| [src/boot/bios.h](/src/boot/bios.h) | C++ | 29 | 3 | 9 | 41 |
| [src/boot/boot.asm](/src/boot/boot.asm) | NASM Assembly | 227 | 70 | 85 | 382 |
| [src/boot/bootutil.h](/src/boot/bootutil.h) | C++ | 78 | 1 | 19 | 98 |
| [src/boot/grub.cfg](/src/boot/grub.cfg) | Properties | 3 | 0 | 0 | 3 |
| [src/boot/stage1.asm](/src/boot/stage1.asm) | NASM Assembly | 223 | 39 | 72 | 334 |
| [src/boot/stage1.c](/src/boot/stage1.c) | C | 29 | 1 | 6 | 36 |
| [src/disk/ata.c](/src/disk/ata.c) | C | 394 | 77 | 75 | 546 |
| [src/disk/ata.h](/src/disk/ata.h) | C++ | 35 | 1 | 9 | 45 |
| [src/disk/customfs.h](/src/disk/customfs.h) | C++ | 25 | 4 | 5 | 34 |
| [src/disk/fat.c](/src/disk/fat.c) | C | 296 | 52 | 70 | 418 |
| [src/disk/fat.h](/src/disk/fat.h) | C++ | 172 | 11 | 16 | 199 |
| [src/disk/vfs.c](/src/disk/vfs.c) | C | 96 | 12 | 20 | 128 |
| [src/disk/vfs.h](/src/disk/vfs.h) | C++ | 42 | 2 | 15 | 59 |
| [src/interrupts/idt.c](/src/interrupts/idt.c) | C | 33 | 0 | 6 | 39 |
| [src/interrupts/idt.h](/src/interrupts/idt.h) | C++ | 7 | 0 | 4 | 11 |
| [src/interrupts/irq.c](/src/interrupts/irq.c) | C | 59 | 0 | 11 | 70 |
| [src/interrupts/irq.h](/src/interrupts/irq.h) | C++ | 19 | 0 | 7 | 26 |
| [src/interrupts/isr.c](/src/interrupts/isr.c) | C | 202 | 19 | 16 | 237 |
| [src/interrupts/isr.h](/src/interrupts/isr.h) | C | 12 | 0 | 4 | 16 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 55 | 22 | 23 | 100 |
| [src/kernel/kernel.h](/src/kernel/kernel.h) | C++ | 29 | 0 | 6 | 35 |
| [src/kernel/kernel\_start.asm](/src/kernel/kernel_start.asm) | NASM Assembly | 168 | 1 | 30 | 199 |
| [src/kernel/kish.c](/src/kernel/kish.c) | C | 181 | 10 | 30 | 221 |
| [src/kernel/multitasking.h](/src/kernel/multitasking.h) | C++ | 39 | 8 | 9 | 56 |
| [src/kernel/smallgame.c](/src/kernel/smallgame.c) | C | 49 | 10 | 12 | 71 |
| [src/keyboard/keyboard.c](/src/keyboard/keyboard.c) | C | 281 | 42 | 59 | 382 |
| [src/keyboard/keyboard.h](/src/keyboard/keyboard.h) | C++ | 159 | 6 | 21 | 186 |
| [src/lib/fpu.c](/src/lib/fpu.c) | C | 13 | 0 | 2 | 15 |
| [src/lib/fpu.h](/src/lib/fpu.h) | C++ | 5 | 0 | 2 | 7 |
| [src/lib/io.c](/src/lib/io.c) | C | 25 | 6 | 6 | 37 |
| [src/lib/io.h](/src/lib/io.h) | C++ | 10 | 0 | 3 | 13 |
| [src/lib/math.c](/src/lib/math.c) | C | 146 | 7 | 35 | 188 |
| [src/lib/math.h](/src/lib/math.h) | C++ | 16 | 0 | 4 | 20 |
| [src/lib/stdarg.h](/src/lib/stdarg.h) | C++ | 7 | 0 | 5 | 12 |
| [src/lib/string.h](/src/lib/string.h) | C++ | 83 | 9 | 25 | 117 |
| [src/lib/types.h](/src/lib/types.h) | C++ | 49 | 9 | 14 | 72 |
| [src/lib/util.h](/src/lib/util.h) | C++ | 33 | 2 | 13 | 48 |
| [src/memory/memmanage.c](/src/memory/memmanage.c) | C | 391 | 78 | 79 | 548 |
| [src/memory/memmanage.h](/src/memory/memmanage.h) | C++ | 65 | 4 | 13 | 82 |
| [src/memory/multiboot.h](/src/memory/multiboot.h) | C++ | 100 | 19 | 26 | 145 |
| [src/programs/prgm.asm](/src/programs/prgm.asm) | NASM Assembly | 14 | 4 | 4 | 22 |
| [src/sound/pcspkr.c](/src/sound/pcspkr.c) | C | 32 | 0 | 10 | 42 |
| [src/sound/pcspkr.h](/src/sound/pcspkr.h) | C | 11 | 0 | 3 | 14 |
| [src/time/time.c](/src/time/time.c) | C | 120 | 59 | 48 | 227 |
| [src/time/time.h](/src/time/time.h) | C++ | 11 | 2 | 5 | 18 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)