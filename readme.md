# Dedication OS - My Custom OS Project

This project is meant to be a lightweight 32-bit x86 operating system developed entirely and exclusively by me, using only my own code. Every line of code in every file was written by me, unless stated otherwise. Written entirely from scratch.

## Project Status
Currently, this project is still in the early stages. I've spent the past year learning about x86 and I started with basically zero knowledge, including in C and assembly.

## System Requirements (subject to change)
- Any i686-compatible CPU
- More than 1-4 GiB of RAM(?)

## Features
  - Boots
  - Sets up paging
  - Initializes heap
  - Sets up keyboard



## To-Do
- to-do list...

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