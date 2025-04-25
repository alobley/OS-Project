# Compiler and Assembler (code is specific to GCC and supported compilers)
ASM=nasm
CCOM=i686-elf-gcc
ARCH=i386
BOOTDISK=boot.img

# QEMU Arguments
EMARGS=-m 512M -smp 1 -vga std -display sdl,gl=on -machine pc-i440fx-5.2 -cpu pentium -accel kvm
EMARGS+=-hda bin/harddisk.vdi
EMARGS+=-audiodev sdl,id=sdl,out.frequency=48000,out.channels=2,out.format=s32
EMARGS+=-device sb16,audiodev=sdl -machine pcspk-audiodev=sdl
EMARGS+=-device ich9-usb-uhci1 -monitor stdio -boot d -d int #-no-reboot -no-shutdown #-s -S

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
MULTITASK_DIR=$(SRC_DIR)/multitasking
USER_DIR=$(SRC_DIR)/userland
SOUND_DIR=$(SRC_DIR)/sound
ACPI_DIR=$(SRC_DIR)/acpi
STRUCT_DIR=$(SRC_DIR)/datastructures
DISK_DIR=$(SRC_DIR)/disk
GRAPHICS_DIR=$(SRC_DIR)/graphics
FS_DIR=$(SRC_DIR)/filesystems
DRIVER_DIR=$(SRC_DIR)/drivers
LIBC_DIR=$(SRC_DIR)/libc

# Include Directories
INCLUDES=-I $(SRC_DIR) -I $(KERNEL_DIR) -I $(LIB_DIR) -I $(CONSOLE_DIR) -I $(INT_DIR) -I $(MEM_DIR) -I $(PS2_DIR) -I $(TIME_DIR) -I $(GRAPHICS_DIR)
INCLUDES+=-I $(USER_DIR) -I $(MULTITASK_DIR) -I $(SOUND_DIR) -I $(ACPI_DIR) -I $(STRUCT_DIR) -I $(VFS_DIR) -I $(DISK_DIR) -I $(STRUCT_DIR) -I $(FS_DIR) 
INCLUDES+=-I $(LIBC_DIR)

# Compilation Flags (TODO: don't compile with lGCC)
CFLAGS=-T linker.ld -m32 -ffreestanding -O2 -nostdlib --std=c99 -Wall -Wextra -Wcast-align -lgcc -fno-stack-protector -fno-delete-null-pointer-checks -fno-tree-dce
CFLAGS+=$(INCLUDES) -Wno-unused -Wno-array-bounds -Werror -march=i586 -mtune=generic -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer #-save-temps

# Libraries to compile with
LIBS=$(BUILD_DIR)/kernel_start.o $(CONSOLE_DIR)/console.c $(INT_DIR)/interrupts.c $(KERNEL_DIR)/devices.c $(LIB_DIR)/kernel_system.c
LIBS+=$(INT_DIR)/pic.c $(TIME_DIR)/time.c $(MEM_DIR)/paging.c $(MEM_DIR)/alloc.c $(PS2_DIR)/keyboard.c $(VFS_DIR)/vfs.c #$(PS2_DIR)/ps2.c 
LIBS+=$(MULTITASK_DIR)/multitasking.c $(SOUND_DIR)/pcspkr.c $(ACPI_DIR)/acpi.c $(KERNEL_DIR)/users.c $(STRUCT_DIR)/hash.c  $(USER_DIR)/shell.c
LIBS+=$(CONSOLE_DIR)/tty.c $(DISK_DIR)/ata.c $(FS_DIR)/fat.c $(LIBC_DIR)/stdio.c

# Assembly and Kernel Files
ASMFILE=stage0
CFILE=kernel

# Placeholder for additional kernel functionality
PROGRAM_FILE=programtoload

USER_CFLAGS=-I $(USER_DIR) -I $(KERNEL_DIR) -I $(LIBC_DIR) -I $(DRIVER_DIR) -I $(LIB_DIR) -static -m32 -ffreestanding -O2 -nostdlib --std=c99 -Wall -Wextra -Wcast-align
USER_CFLAGS+=-lgcc -fno-stack-protector -fno-delete-null-pointer-checks -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -T $(USER_DIR)/userland.ld
USER_CFLAGS+=-Wno-unused -Wno-array-bounds -Werror -march=i586 -mtune=generic

# Build Targets
all: clean assemble compile drive_image addfiles qemu_grub

custom_boot: assemble compile boot_image addfiles qemu_custom
grub_boot: assemble compile drive_image addfiles qemu_grub

create_dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR) $(MNT_DIR)


# Create boot disk image with custom bootloader
boot_image: create_dirs assemble compile
	dd if=/dev/zero of=$(BIN_DIR)/boot.img bs=512 count=2880
    # Reserve 11 sectors for the bootloader
	mkfs.fat -F 12 -R 11 $(BIN_DIR)/boot.img
	sudo mount -o loop,rw $(BIN_DIR)/boot.img mnt
	sudo cp $(BUILD_DIR)/$(CFILE).bin mnt/KERNEL.ELF
	sync
	sudo umount mnt
	dd if=$(BUILD_DIR)/$(ASMFILE).bin of=$(BIN_DIR)/boot.img conv=notrunc
	dd if=$(BUILD_DIR)/stage1.bin of=$(BIN_DIR)/boot.img seek=1 conv=notrunc

# Create Boot Disk Image with GRUB
drive_image: create_dirs assemble compile
	mkdir -p isodir/boot/grub
	cp $(BUILD_DIR)/$(CFILE).elf isodir/boot/$(CFILE).elf
	cp $(BOOT_DIR)/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o bin/main.iso isodir

# Assemble Kernel Startup and a Test Program
assemble: create_dirs
	$(ASM) -fbin $(BOOT_DIR)/$(ASMFILE).asm -o $(BUILD_DIR)/$(ASMFILE).bin
	$(ASM) -felf32 $(KERNEL_DIR)/kernel_start.asm -o $(BUILD_DIR)/kernel_start.o

	$(ASM) -fbin $(BOOT_DIR)/stage1.asm -o $(BUILD_DIR)/stage1.bin

	$(ASM) -felf32 $(USER_DIR)/program.asm -o $(BUILD_DIR)/prgm.o
	$(ASM) -felf32 $(USER_DIR)/hello.asm -o $(BUILD_DIR)/hello.o

	i686-elf-ld $(BUILD_DIR)/prgm.o -o $(BUILD_DIR)/prgm.elf -m elf_i386 -T $(USER_DIR)/temp_userland.ld
	i686-elf-ld $(BUILD_DIR)/hello.o -o $(BUILD_DIR)/hello.elf -m elf_i386 -T $(USER_DIR)/temp_userland.ld

# Compile Kernel
compile: create_dirs $(KERNEL_DIR)/$(CFILE).c
	@$(CCOM) -o $(BUILD_DIR)/$(CFILE).elf $(KERNEL_DIR)/$(CFILE).c $(LIBS) $(CFLAGS)
	@$(CCOM) -o $(BUILD_DIR)/usershell.elf $(USER_DIR)/userc.c $(USER_DIR)/system.c $(USER_CFLAGS)
#src/libc/stdio.c
#$(CCOM) -m16 -o $(BUILD_DIR)/stage1.bin $(BOOT_DIR)/stage2.c $(BUILD_DIR)/stage1.o -T$(BOOT_DIR)/boot.ld -static -ffreestanding -nostdlib -fno-stack-protector -lgcc --std=c99 -Wall -Wextra -Wcast-align -Wno-unused -Wno-array-bounds -Werror -I $(BOOT_DIR)

# Run QEMU
qemu_custom: create_dirs
	qemu-system-$(ARCH) $(EMARGS) -fda bin/boot.img

qemu_grub: create_dirs
	qemu-system-$(ARCH) $(EMARGS) -cdrom bin/main.iso

# Add Files to Virtual Disk
addfiles: create_dirs
	sync
	sudo mount -o loop,rw bin/harddisk.vdi mnt
	sudo rm -rf mnt/*
	sudo cp $(BUILD_DIR)/prgm.elf mnt/PROGRAM.ELF
	sudo cp $(BUILD_DIR)/hello.elf mnt/HELLO.ELF
	sudo cp $(BUILD_DIR)/usershell.elf mnt/SHELL.ELF
	sync
	@sleep 1
	sudo umount mnt

# Create the Hard Drive Image, for some reason .qcow2 doesn't show sectors properly.
hard_drive: create_dirs
	qemu-img create -f raw $(BIN_DIR)/harddisk.vdi 2G
	mkfs.fat -F 32 $(BIN_DIR)/harddisk.vdi

# Update the Repository
update:
	git add .
	git commit -m "Minor Update"
	git push -u origin main

# Clean Build Artifacts
clean:
	rm -rf build/*
	rm -rf isodir/*