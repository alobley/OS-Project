# Compiler and Assembler
ASM=nasm
CCOM=i686-elf-gcc
ARCH=i386

# QEMU Arguments
EMARGS=-m 512M -smp 1 -vga std -display gtk -cdrom build/main.iso
EMARGS+=-drive file=bin/harddisk.vdi,format=raw,if=ide -boot d
EMARGS+=-d cpu_reset -audiodev sdl,id=sdl,out.frequency=48000,out.channels=2,out.format=s32
EMARGS+=-device sb16,audiodev=sdl -machine pcspk-audiodev=sdl
EMARGS+=-device ich9-usb-uhci1 -monitor stdio #-d int,exec -no-reboot

# Directories
SRC_DIR=src
BUILD_DIR=build
BIN_DIR=bin
MNT_DIR=mnt
LIB_DIR=$(SRC_DIR)/lib
INT_DIR=$(SRC_DIR)/interrupts
VGA_DIR=$(SRC_DIR)/VGA
BOOT_DIR=$(SRC_DIR)/boot
KERNEL_DIR=$(SRC_DIR)/kernel
MEM_DIR=$(SRC_DIR)/memory
TIME_DIR=$(SRC_DIR)/time
KB_DIR=$(SRC_DIR)/keyboard
DISK_DIR=$(SRC_DIR)/disk
SOUND_DIR=$(SRC_DIR)/sound
PROG_DIR=$(SRC_DIR)/programs

# Include Directories
INCLUDES=-I $(SRC_DIR) -I $(LIB_DIR) -I $(INT_DIR) -I $(VGA_DIR) -I $(BOOT_DIR)
INCLUDES+=-I $(KERNEL_DIR) -I $(MEM_DIR) -I $(TIME_DIR) -I $(KB_DIR) -I $(DISK_DIR) -I $(SOUND_DIR) -I $(SRC_DIR)/acpi

# Compilation Flags
CFLAGS=-T linker.ld -ffreestanding -O2 -nostdlib --std=gnu99 -Wall -Wextra -Wcast-align $(INCLUDES)

# Libraries to Link
LIBS=$(BUILD_DIR)/kernel_start.o $(INT_DIR)/isr.c $(INT_DIR)/idt.c $(INT_DIR)/irq.c
LIBS+=$(LIB_DIR)/io.c $(LIB_DIR)/fpu.c $(VGA_DIR)/vga.c $(VGA_DIR)/pixel.c
LIBS+=$(TIME_DIR)/time.c $(KB_DIR)/keyboard.c $(LIB_DIR)/math.c
LIBS+=$(DISK_DIR)/ata.c $(DISK_DIR)/fat.c $(SOUND_DIR)/pcspkr.c $(VGA_DIR)/text.c
LIBS+=$(KERNEL_DIR)/smallgame.c $(KERNEL_DIR)/kish.c $(DISK_DIR)/vfs.c $(SRC_DIR)/acpi/acpi.c
LIBS+=$(MEM_DIR)/memmanage.c

# Assembly and Kernel Files
BOOTLOADER=boot
CFILE=kernel

# Placeholder for additional kernel functionality
PROGRAM_FILE=programtoload

# Build Targets
all: assemble compile build_boot drive_image addfiles qemu

create_dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR) $(MNT_DIR)

# Create Boot Disk Image (backup with GRUB)
drive_image: create_dirs
	mkdir -p isodir/boot/grub
	cp $(BUILD_DIR)/$(CFILE).bin isodir/boot/$(CFILE).bin
	cp $(BOOT_DIR)/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o build/main.iso isodir

# Assemble Kernel Startup
assemble: create_dirs
	$(ASM) -felf32 $(KERNEL_DIR)/kernel_start.asm -o $(BUILD_DIR)/kernel_start.o
	$(ASM) -fbin $(PROG_DIR)/prgm.asm -o $(BUILD_DIR)/prgm.bin

# Build Bootloader
build_boot: create_dirs
	$(ASM) -fbin $(BOOT_DIR)/$(BOOTLOADER).asm -o $(BUILD_DIR)/$(BOOTLOADER).bin
	$(ASM) -felf32 $(BOOT_DIR)/stage1.asm -o $(BUILD_DIR)/stage1.o
	$(CCOM) -m16 -o $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage1.o $(BOOT_DIR)/stage1.c -T $(BOOT_DIR)/linker.ld -ffreestanding -O2 -nostdlib -nodefaultlibs -nostartfiles -Wall -Wextra -Wcast-align -I $(BOOT_DIR)

# Compile Kernel
compile: create_dirs $(KERNEL_DIR)/$(CFILE).c
	$(CCOM) -o $(BUILD_DIR)/$(CFILE).bin $(KERNEL_DIR)/$(CFILE).c $(LIBS) $(CFLAGS)

# Run QEMU
qemu: create_dirs $(BUILD_DIR)/main.iso
	qemu-system-$(ARCH) $(EMARGS)

# Add Files to Virtual Disk
addfiles: create_dirs
	sudo mount -o loop,rw bin/harddisk.vdi mnt
	sudo cp $(BUILD_DIR)/prgm.bin mnt/PROGRAM.BIN
	sudo umount mnt
	sync

# Create the Hard Drive Image, for some reason .qcow2 doesn't show sectors properly.
hard_drive: create_dirs
	qemu-img create -f raw $(BIN_DIR)/harddisk.vdi 2G
	mkfs.fat -F 32 $(BIN_DIR)/harddisk.vdi

# Clean Build Artifacts
clean:
	rm -rf build/*
	rm -rf isodir/*
