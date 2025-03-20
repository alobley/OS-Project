; My first userland program for my custom operating system!
ORG 0
BITS 32
CPU 686

%define SYS_WRITE 4             ; Syscall number for sys_write
%define STDOUT_FILENO 1         ; File descriptor 1 is stdout
%define NEWL 0x0A               ; Newline character (\n)

section .text
global _start
_start:
    mov eax, SYS_WRITE
    mov ebx, STDOUT_FILENO
    mov ecx, msg                ; Pointer to the message
    mov edx, msg_len            ; Length of the message
    int 0x30                    ; Call the kernel to print the message

    ; Exit the program
    ret                         ; Return to the operating system (need to implement SYS_EXIT)

section .data
msg: db "I am a user program loaded from the disk!", NEWL
msg_len: equ $ - msg