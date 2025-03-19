ORG 0
BITS 32
CPU 686

section .text
_start:
    mov eax, 4
    mov ebx, 1
    mov ecx, msg
    mov edx, msg_len
    int 0x30

    ret

section .data
msg: db "I am a user program loaded from the disk!", 0xA
msg_len: equ $ - msg