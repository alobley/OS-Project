ORG 0
BITS 32
CPU 686

; Simple hello world program to test my OS's ABI

section .text
global _start

%define SYS_WRITE 6                     ; Syscall number for sys_write
%define STDOUT_FILENO 1                 ; File descriptor 1 is stdout
%define SYS_EXIT 8                      ; Syscall number for sys_exit
%define NEWL 0x0A                       ; Newline character (\n)

_start:
    mov eax, SYS_WRITE                  ; Write to a file
    mov ebx, STDOUT_FILENO              ; File descriptor 1 is stdout
    mov ecx, msg                        ; Pointer to the message
    mov edx, msg_len                    ; Length of the message
    int 0x30                            ; Call the kernel to print the message

    mov eax, SYS_EXIT                   ; Syscall number for sys_exit
    mov ebx, 0                          ; Exit code 0
    int 0x30                            ; Call the kernel to exit the program

section .data
msg: db 'Hello, World!', 0x0A           ; Message to print
msg_len: equ $ - msg                    ; Length of the message