# Compiler and Assembler (code is specific to GCC and supported compilers)
ASM=nasm
CCOM=i686-elf-gcc
ARCH=i386

# QEMU Arguments
EMARGS=-m 512M -smp 1 -vga vmware -display gtk
EMARGS+=-cdrom build/main.iso 
EMARGS+=-hda bin/harddisk.vdi 
EMARGS+=-audiodev sdl,id=sdl,out.frequency=48000,out.channels=2,out.format=s32
EMARGS+=-device sb16,audiodev=sdl -machine pcspk-audiodev=sdl
EMARGS+=-device ich9-usb-uhci1 -monitor stdio -boot d -d int -no-reboot -no-shutdown

# Directories
SRC_DIR=src
BUILD_DIR=build
BIN_DIR=bin
MNT_DIR=mnt
KERNEL_DIR=$(SRC_DIR)/kernel
BOOT_DIR=$(SRC_DIR)/boot
LIB_DIR=$(SRC_DIR)/lib
CONSOLE_DIR=$(SRC_DIR)/console
INT_DIR=$(SRC_DIR)/interrupts
MEM_DIR=$(SRC_DIR)/memory
PS2_DIR=$(SRC_DIR)/ps2
TIME_DIR=$(SRC_DIR)/time
VFS_DIR=$(SRC_DIR)/vfs
DISK_DIR=$(SRC_DIR)/disk

# Include Directories
INCLUDES=-I $(SRC_DIR) -I $(KERNEL_DIR) -I $(LIB_DIR) -I $(CONSOLE_DIR) -I $(INT_DIR) -I $(MEM_DIR) -I $(PS2_DIR) -I $(TIME_DIR) -I $(VFS_DIR) -I $(DISK_DIR)

# Compilation Flags (TODO: don't compile with lGCC)
CFLAGS=-T linker.ld -ffreestanding -O2 -nostdlib --std=c99 -Wall -Wextra -Wcast-align -Wpedantic -lgcc $(INCLUDES) -Werror

# Libraries to Link
LIBS=$(BUILD_DIR)/kernel_start.o $(CONSOLE_DIR)/console.c $(INT_DIR)/interrupts.c 
LIBS+=$(INT_DIR)/pic.c $(TIME_DIR)/time.c $(MEM_DIR)/paging.c #$(MEM_DIR)/alloc.c $(VFS_DIR)/vfs.c $(DISK_DIR)/disk.c $(PS2_DIR)/ps2.c 

# Assembly and Kernel Files
ASMFILE=boot
CFILE=kernel

# Placeholder for additional kernel functionality
PROGRAM_FILE=programtoload

# Build Targets
all: assemble compile drive_image addfiles qemu

create_dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR) $(MNT_DIR)

# Create Boot Disk Image
drive_image: create_dirs
	mkdir -p isodir/boot/grub
	cp $(BUILD_DIR)/$(CFILE).bin isodir/boot/$(CFILE).bin
	cp $(BOOT_DIR)/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o build/main.iso isodir

# Assemble Kernel Startup
assemble: create_dirs
	$(ASM) -felf32 $(KERNEL_DIR)/kernel_start.asm -o $(BUILD_DIR)/kernel_start.o
#$(ASM) -fbin $(PROG_DIR)/prgm.asm -o $(BUILD_DIR)/prgm.bin

# Compile Kernel
compile: create_dirs $(KERNEL_DIR)/$(CFILE).c
	$(CCOM) -o $(BUILD_DIR)/$(CFILE).bin $(KERNEL_DIR)/$(CFILE).c $(LIBS) $(CFLAGS)

# Run QEMU
qemu: create_dirs $(BUILD_DIR)/main.iso
	qemu-system-$(ARCH) $(EMARGS)

# Add Files to Virtual Disk
addfiles: create_dirs
	sudo mount -o loop,rw bin/harddisk.vdi mnt
#sudo cp $(BUILD_DIR)/prgm.bin mnt/PROGRAM.BIN
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
