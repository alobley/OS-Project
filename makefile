# Compiler and Assembler
ASM=nasm
CCOM=i686-elf-gcc
ARCH=i386

# QEMU Arguments
EMARGS=-m 4G -smp 1 -vga std -display sdl -serial stdio -drive file=build/main.iso,media=cdrom,if=ide
EMARGS+=-drive file=bin/harddisk.vdi,format=raw,if=ide -boot d
EMARGS+=-d cpu_reset -audiodev sdl,id=sdl,out.frequency=48000,out.channels=2,out.format=s32
EMARGS+=-device sb16,audiodev=sdl -machine pcspk-audiodev=sdl

# Directories
SRC_DIR=src
BUILD_DIR=build
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

# Include Directories
INCLUDES=-I $(SRC_DIR) -I $(LIB_DIR) -I $(INT_DIR) -I $(VGA_DIR) -I $(BOOT_DIR)
INCLUDES+=-I $(KERNEL_DIR) -I $(MEM_DIR) -I $(TIME_DIR) -I $(KB_DIR) -I $(DISK_DIR) -I $(SOUND_DIR)

# Compilation Flags
CFLAGS=-T linker.ld -ffreestanding -O2 -nostdlib --std=gnu99 -Wall $(INCLUDES)

# Libraries to Link
LIBS=$(BUILD_DIR)/kernel_start.o $(INT_DIR)/isr.c $(INT_DIR)/idt.c $(INT_DIR)/irq.c
LIBS+=$(LIB_DIR)/io.c $(LIB_DIR)/fpu.c $(VGA_DIR)/vga.c $(VGA_DIR)/pixel.c
LIBS+=$(MEM_DIR)/alloc.c $(TIME_DIR)/time.c $(KB_DIR)/keyboard.c $(LIB_DIR)/math.c
LIBS+=$(DISK_DIR)/ata.c $(DISK_DIR)/fat.c $(SOUND_DIR)/pcspkr.c $(VGA_DIR)/text.c
LIBS+=$(KERNEL_DIR)/smallgame.c

# Assembly and Kernel Files
ASMFILE=boot
CFILE=kernel

# Placeholder for additional kernel functionality
PROGRAM_FILE=programtoload

# Build Targets
all: assemble compile drive_image qemu

# Create Boot Disk Image
drive_image:
	mkdir -p isodir/boot/grub
	cp $(BUILD_DIR)/$(CFILE).bin isodir/boot/$(CFILE).bin
	cp $(BOOT_DIR)/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o build/main.iso isodir

# Assemble Kernel Startup
assemble:
	$(ASM) -felf32 $(KERNEL_DIR)/kernel_start.asm -o $(BUILD_DIR)/kernel_start.o

# Compile Kernel
compile: $(KERNEL_DIR)/$(CFILE).c
	$(CCOM) -o $(BUILD_DIR)/$(CFILE).bin $(KERNEL_DIR)/$(CFILE).c $(LIBS) $(CFLAGS)

# Run QEMU
qemu: $(BUILD_DIR)/main.iso
	qemu-system-$(ARCH) $(EMARGS)

# Add Files to Virtual Disk
addfiles:
	mkdir -p mnt
	sudo mount -o loop,rw bin/harddisk.qcow2 mnt
	sudo cp $(BUILD_DIR)/prgm.bin mnt/prgm.bin
	sync
	sudo umount mnt

# Create the Hard Drive Image
hard_drive:
	mkdir -p bin
	qemu-img create -f qcow2 bin/harddisk.qcow2 2G
	mkfs.fat -F 32 bin/harddisk.qcow2

# Clean Build Artifacts
clean:
	rm -rf build/*
	rm -rf isodir/*