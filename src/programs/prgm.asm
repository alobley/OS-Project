; Tiny program that debugs the system call interface and the program loader/executor
; My first "Hello, World!" program for my OS!
BITS 32
section .text
    global _start

%define VGA_MODE_TEXT 0x03
%define VGA_MODE_GRAPHICS 0x13

_start:
    call _main              ; Call the main function to get the program's physical memory address
_main:
    ; Calculate the program's physical memory address since the OS has no memory protection
    pop esi
    sub esi, _main

    mov eax, 1              ; System call number for print
    lea ebx, [esi + msg]    ; Load the address of the message to print
    int 0x30                ; System call

    mov eax, 0              ; Return value. Indicates success.
    ret                     ; Return to the kernel

; The message to print
msg: db "Hello, World!", 10, 0