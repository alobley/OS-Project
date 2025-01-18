# Dedication OS - My Custom OS Project

This project is meant to be a lightweight 32-bit x86 operating system developed entirely and exclusively by me, using only my own code. Every line of code in every file was written by me, unless stated otherwise. GRUB, of course, also does not belong to me. Written entirely from scratch.

## Project Status
Currently, this project is still in the early stages. I've spent the past year learning about x86 and I started with basically zero knowledge, including in C and assembly.
Kernel Version Alpha 0.0.1 is on the way.

## System Requirements (subject to change)
- An i686 processor or newer
- At least 10MB of RAM
- A VGA-compatible display
- PATA/PATAPI compatible storage
- PS/2 keyboard

## Features
- **Boot Process**:
  - Boots with GRUB
  - Installs the Global Descriptor Table (GDT)
- **System Initialization**:
  - Initializes memory management
  - Sets up interrupt handling
- **Hardware Support**:
  - Keyboard and timer ISRs mapped to the Programmable Interrupt Controller (PIC)
  - PC speaker initialization
  - Very basic disk driver (currently incomplete)
  - Parses ACPI tables
- **Graphics**:
  - VGA driver setup
- **Kernel**:
  - Executes minimal kernel functionality



## To-Do
- Comment and document the code. Both are very sparse.
- Learn how to use git and GitHub properly. (done)
- Write an ATA and FAT driver for disk access. (working on it - functional)
- Write a system call interrupt and an ABI. (working on it - functional)
- Move away from ATA PIO Mode
- Write to the disk
- Read FAT subdirectories
- Add ISO9660 support
- Finish the new memory management system (currently broken)
- Improve upon the VFS

## Long-Term Goals
- Write a terminal application using my custom ABI.
- Utilize the majority of the GRUB Multiboot info structure
- Implement paging and better memory allocation. (working on it)
- Implement process management and multitasking.  (primitives exist, but incomplete)
- Add a userland and memory protection. (Working on it)
- Add the ability for programs to interact with each other
- Add the ability for drivers to run separately from the kernel itself (how do I do that and have a standard system for interacting with hardware?)
- Add USB support. (basically required for running on real hardware)
- Add milticore support.
- Create a custom filesystem (very long-term goal)

## Completed Goals
- Get the GRUB memory map
- Learn Paging (Thanks, OSDev community!)
- Implement paging
- Read from the disk
- Load and execute a program
- Install a syscall ISR
- Have a minimal CLI to make debugging easier

## Build & Run

**Note:** This is currently largely untested on real hardware. It is recommended to use QEMU for emulation.

### Prerequisites
Ensure you have the following installed:
- A Linux or FreeBSD system
- `i686-elf-gcc` (cross-compiler)
- `i686-elf-ld`
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
This project is dual-licensed:
- The **GRand Unified Bootloader (GRUB)**, which is used to load the OS, is under the GNU General Public License (GPL). To obtain the GRUB source: `git clone https://git.savannah.gnu.org/git/grub.git`
- All other parts of the project are licensed under the **MIT License**. See the `LICENSE` file in the root directory for more information.
