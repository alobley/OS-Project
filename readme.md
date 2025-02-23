# Dedication OS - My Custom OS Project

This project is meant to be a lightweight 32-bit x86 operating system developed entirely and exclusively by me, using only my own code. Every line of code in every file was written by me, unless stated otherwise. Written entirely from scratch.

## Project Status
Currently, this project is still in the early stages. I've spent the past year learning about x86 and I started with basically zero knowledge, including in C and assembly.

## System Requirements (subject to change)
- Any i686-compatible CPU
- At least 10MB of RAM

## To-Do (road to 0.90.0 beta)
[*] = must have (Will likely be completed first)
[-] = nice to have
[x] = complete
- Add ACPI and/or APM support [x]
- Create a standardized driver interface and add device management [x] (will likely never not be in progress)
- Implement a VFS for standardized file interaction [x]
- Add an initramfs for use on startup [-]
- Add a PATA disk driver [*] (Current goal)
- Add a SATA disk driver [-]
- Add FAT support [*]
- Implement a PC speaker driver (this one should be very simple) [x]
- Fully implement mouse driver [-]
- Make a more complete list of system calls [*]
- Add the ability to load and execute programs [*]
- Optimize paging and improve heap memory management (algorithm is spaghetti code currently) [-]
- Fully implement process scheduling and multitasking [*]
- Harden the kernel against vulnerabilities [-]
- Add PCI/PCIE support [*]
- Add USB support [*]
- Implement a userland kernel API in C [*]
- Replace KISh with a proper userland shell [*]
- Make a libc and more complete userland [*]
- Implement a better VGA driver, a VMWare SVGA driver, and an i915 driver [-]

## Goals to reach before 1.0
- EXT2 support [*]
- ISO9660 support [*]
- HD audio support [-]
- Networking [-]
- Window manager (no promises) [-]
- Init system [-]
- Follow UNIX philosophy [-]
- Write some documentation [-]

## Build & Run

**Note:** This is currently largely untested on real hardware. It is recommended to use QEMU for emulation.

### Prerequisites
Ensure you have the following installed:
- A Linux or FreeBSD system
- `i386-elf-gcc`
- `i386-elf-ld`
- `binutils`
- `NASM`
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