BITS 64
extern kernel_main

section .text

global _start:function (_start.end - _start)
_start:
    ; Pointer to bootutil_t structure is in rdi
    ; We don't have to do much here, just set up the stack and call kernel_main. GDT and Paging can be done there.
    mov rsp, stack_top
    mov rbp, stack_bottom
    mov rdi, rdi
    jmp kernel_main
.end:


section .bss
align 16        ; Align to 16 bytes (required on x86_64)
stack_bottom: 
    ; 64KiB stack
    resb 0x10000
stack_top: