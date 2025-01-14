; Tiny program that debugs the system call interface and the program loader/executor
; My first "Hello, World!" program for my OS!
BITS 32
section .text
    global _start

; For later when I want to expand this program
%define VGA_MODE_TEXT 0x03
%define VGA_MODE_GRAPHICS 0x13
%define VGA_GRAPHICS_FRAMEBUFFER 0xA0000
%define VGA_TEXT_FRAMEBUFFER 0xB8000

_start:
    mov eax, 1              ; System call number for print
    lea ebx, [msg]          ; Load the address of the message to print
    int 0x30                ; System call

    mov eax, 0              ; Return value. Indicates success.
    ret                     ; Return to the kernel

; The message to print
msg: db "Hello, World!", 10, 0