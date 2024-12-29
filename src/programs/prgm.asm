BITS 32
section .text
    global _start

_start:
    ; Perform a system call and return. That's all this needs to do.
    mov eax, 2
    int 0x30
    ret