# Dedication OS - My Custom OS Project

This project is meant to be a lightweight 32-bit x86 operating system developed entirely and exclusively by me, using only my own code. Every line of code in every file was written by me, unless stated otherwise. Written entirely from scratch.

## Project Status
Currently, this project is still in the early stages. I've spent the past year learning about x86 and I started with basically zero knowledge, including in C and assembly.

## System Requirements (subject to change)
- Any i686-compatible CPU
- At least 10MB of RAM
- A PS/2 keyboard
- ACPI support

## To-Do (road to 0.90.0 beta)
[*] = must have (Will likely be completed first)
[-] = nice to have
[x] = complete
- Implement a PC speaker driver (this one should be very simple) [x]
- Add ACPI and/or APM support [x]
- Make a more complete list of system calls [x]
- Create a standardized driver interface and add device management [x]
- Implement a VFS for standardized file interaction [x]
- Add a PATA disk driver [x]
- Add FAT support [x] (Current goal)
- Add the ability to load and execute programs [x]
- Implement a userland kernel API in C [x] (more or less functional, can be compiled with user programs)
- Fully implement process scheduling and multitasking [x]
- Replace KISh with a proper userland shell [x]
- Make a libc and more complete userland [*]
- Add PCI/PCIE support [*]
- Add USB support [*]
- Add an initramfs for use on startup [-]
- Add a SATA disk driver [-]
- Fully implement mouse driver [-]
- Optimize paging and improve heap memory management (algorithm is spaghetti code currently) [x]
- Harden the kernel against vulnerabilities [x]
- Implement a better VGA driver, a VMWare SVGA driver, and an i915 driver [-]

## Goals to reach before 1.0
- EXT2 support [*]
- ISO9660 support [*]
- Optimization, especially of TTY interaction and file descriptors [*]
- HD audio support [-]
- Networking [-]
- Window manager (no promises) [-]
- Init system [-]
- Follow UNIX philosophy [-]
- Write some documentation [-]

## Post-1.0
- EXT4 support
- NTFS support
- SMP support
- 64-bit BIOS version
- 64-bit UEFI version

## Build & Run

**Note:** This is currently largely untested on real hardware. It is recommended to use QEMU for emulation.

### Prerequisites
Ensure you have the following installed:
- A Linux or FreeBSD system
- `i386-elf-gcc`
- `i386-elf-ld`
- `binutils`
- `NASM`
- `grub-mkrescue`
- `dd`
- `xorriso`
- `QEMU`

### Build Instructions
1. Clone the repository:
   `git clone https://github.com/alobley/os-project.git`
2. Change directory:
    `cd os-project`
3. Build and run the project:
    `make`

## Contributing
Feel free to fork this if you wish to do so, but the goal of this project is to see what I can do if everything is supported by my own code. Feedback, however, is always welcome.

## License
- This software is licensed under the **MIT License**. See the `LICENSE` file in the root directory for more information.

- GRUB, the bootloader, is licensed under the **GNU General Public License**. See the `LICENCE` file for more information.