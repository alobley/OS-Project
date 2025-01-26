ASM=nasm
CC=gcc
ARCH=x86_64

EMARGS=-m 8G -smp 4 -vga none -display gtk -cpu qemu64 -enable-kvm
EMARGS+=-machine q35,accel=kvm -device vmware-svga,vgamem_mb=512 -device ich9-intel-hda -device hda-output 
EMARGS+=-netdev user,id=net0,hostfwd=tcp::2222-:22 -device e1000,netdev=net0 -device usb-ehci,id=usb-bus -device usb-tablet -device usb-kbd -device usb-mouse
EMARGS+=-drive file=bin/drive.img,format=raw,if=none,id=drive-sata0 -device ich9-ahci,id=ahci -device ide-hd,drive=drive-sata0,bus=ahci.0 
EMARGS+=-L /usr/share/edk2/x64/ -drive if=pflash,format=raw,unit=0,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd 
EMARGS+=-d int #-no-reboot -no-shutdown

BOOT_FILE=boot64
KERNEL_FILE=kernel

BUILD_DIR=build
BIN_DIR=bin
SRC_DIR=src
BOOT_DIR=$(SRC_DIR)/boot
MNT_DIR=mnt
LIB_DIR=$(SRC_DIR)/lib

EFI_DIR=$(SRC_DIR)/boot

EFIBIN=BOOTX64.EFI

# Use clang for the EFI bootloader because it is easier to build using it.
BOOT_CC=clang -target x86_64-unknown-windows -fuse-ld=lld-link -nostdlib -Wl,-subsystem:efi_application -Wl,-entry:efi_main -I$(EFI_DIR)
BOOT_CFLAGS=-std=c23 -Wall -Wextra -Wpedantic -mno-red-zone -ffreestanding -Wno-varargs -Werror

KERNEL_CFLAGS=-ffreestanding -m64 -O2 -Wall -Wextra -Werror -I$(SRC_DIR)/kernel -I$(LIB_DIR) -fno-stack-protector -fno-stack-check -mno-red-zone -nostdlib --std=gnu17

all: dirs compile_boot assemble compile_kernel copy qemu

compile_boot:
	@echo "Compiling bootloader..."
	@$(BOOT_CC) $(BOOT_CFLAGS) $(EFI_DIR)/boot64.c -o $(BUILD_DIR)/BOOTX64.EFI

assemble:
	@echo "Assembling kernel assembly..."
	@$(ASM) -f elf64 $(SRC_DIR)/kernel/kstart.asm -o $(BUILD_DIR)/kstart.o

compile_kernel:
	@echo "Compiling and linking kernel..."
	@$(CC) -m64 -c $(SRC_DIR)/kernel/kernel.c -o $(BUILD_DIR)/kernel.o $(KERNEL_CFLAGS)
	@$(CC) -T linker.ld -o $(BUILD_DIR)/kernel.elf -ffreestanding -O2 -nostdlib $(BUILD_DIR)/kernel.o $(BUILD_DIR)/kstart.o -fno-pie -no-pie -fno-pic
	@echo "Done."

copy:
	@echo "Copying files..."
	@sudo losetup --offset 1048576 --sizelimit 46934528 -P /dev/loop0 bin/drive.img
	@sudo mount /dev/loop0 $(MNT_DIR)
	@sudo rm -rf $(MNT_DIR)/*
	@sudo mkdir -p $(MNT_DIR)/EFI/BOOT
	@sudo cp $(BUILD_DIR)/$(EFIBIN) $(MNT_DIR)/EFI/BOOT/$(EFIBIN)
#sudo cp $(BUILD_DIR)/$(EFIBIN) $(MNT_DIR)/$(EFIBIN)
	@sudo cp $(BUILD_DIR)/kernel.elf $(MNT_DIR)/KERNEL.ELF
	@sudo umount $(MNT_DIR)
	@sudo losetup -d /dev/loop0
	@echo "Done."

qemu:
	@echo "Running QEMU..."
	@qemu-system-$(ARCH) $(EMARGS)

dirs:
	@echo "Creating directories..."
	@mkdir -p bin
	@mkdir -p build
	@mkdir -p mnt
	@echo "Done."

ovmf:
	cp /usr/share/edk2/x64/OVMF_VARS.4m.fd bin/OVMF_VARS.fd

# gdisk commands: o, y, n, 1, 2048, 93716, ef00, w, y
drive:
	dd if=/dev/zero of=bin/drive.img bs=512 count=93716
	sudo gdisk bin/drive.img
	sudo losetup --offset 1048576 --sizelimit 46934528 -P /dev/loop0 bin/drive.img
	sudo mkdosfs -F 32 /dev/loop0
	sudo losetup -d /dev/loop0

clean:
	@echo "If this results in an error, then the disk likely wasn't unmounted."
	@rm -rf $(BUILD_DIR)/*
	@sudo umount $(MNT_DIR)
	@sudo losetup -d /dev/loop0
