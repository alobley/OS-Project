BITS 32
CPU 386

extern kmain

MBALIGN equ 1 << 0
MEMINFO equ 1 << 1
MBFLAGS equ MBALIGN | MEMINFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)

section .multiboot
; Commented out because the custom bootloader doesn't use multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

section .text
global _start:function (_start.end - _start)
_start:
    ;cli
    ;hlt

    cld

    lgdt [gdtp]

    mov dx, (gdt_data_segment - gdt_start)
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx
    mov ss, dx

    mov esi, stack_begin
.clear_stack:
    mov byte [esi], 0
    inc esi
    cmp esi, stack
    je .done

.done:
    mov esp, stack
    mov ebp, stack_begin
    cld

    push ebx
    push eax
    push .end

    ; For some reason using the call instruction will always, without fail, cause the first argument to be 0x10.
    ; Note: I have learned that the reason is because it's a far call, so the 0x10 was actually ss. lol.
    jmp 0x8:kmain
.end:
    cli
    hlt
    jmp .end


section .text.interrupts
ALIGN 4

global LoadNewGDT:function (LoadNewGDT.end - LoadNewGDT)
LoadNewGDT:
    mov eax, 4[esp]

    lgdt [eax]

    ret
.end:


global LoadIDT

LoadIDT:
    mov eax, 4[esp]
    lidt [eax]
    ret

%macro ISR_NO_ERR 1
global _isr%1
_isr%1:
    cli
    push 0
    push dword %1
    jmp IsrCommon
%endm

%macro ISR_ERR 1
global _isr%1
_isr%1:
    cli
    push dword %1
    jmp IsrCommon
%endm

global _isr0
_isr0:
    ; Special handling for the timer interrupt
    cli
    mov esp, timer_stack
    mov ebp, timer_stack_base
    push dword 0
    jmp IsrCommon

ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR 8
ISR_NO_ERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_NO_ERR 17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_NO_ERR 21
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28
ISR_NO_ERR 29
ISR_NO_ERR 30
ISR_NO_ERR 31
ISR_NO_ERR 32
ISR_NO_ERR 33
ISR_NO_ERR 34
ISR_NO_ERR 35
ISR_NO_ERR 36
ISR_NO_ERR 37
ISR_NO_ERR 38
ISR_NO_ERR 39
ISR_NO_ERR 40
ISR_NO_ERR 41
ISR_NO_ERR 42
ISR_NO_ERR 43
ISR_NO_ERR 44
ISR_NO_ERR 45
ISR_NO_ERR 46
ISR_NO_ERR 47
ISR_NO_ERR 48

extern ISRHandler

IsrCommon:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld

    push esp
    call ISRHandler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8

    sti
    iretd

section .rodata
ALIGN 16
gdtp:
    dw gdt_end - gdt_start - 1
    dd gdt_start

ALIGN 16
gdt_start:
gdt_null:
    dq 0
gdt_code_segment:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10011010
    db 0b11001111
    db 0x00             
gdt_data_segment:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0b10010010
    db 0b11001111
    db 0x00
gdt_end:

section .data

section .bss
align 16
global stack_begin
global stack
stack_begin:
    resb 0x10000
stack:

global intstack_base
global intstack
align 16
intstack_base:
    resb 0x1000
intstack:

align 16
timer_stack_base:
    resb 0x1000
timer_stack: