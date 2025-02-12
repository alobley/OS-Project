%define FREE_SPACE 0x9000
 
ORG 0x7C00
BITS 16
CPU 386

LOAD_ADDRESS: equ 0x7E00
TOTAL_SECTORS: equ 1024
 
; Main entry point where BIOS leaves us.
 
Main:
    jmp 0x0000:.FlushCS               ; Some BIOSs may load us at 0x0000:0x7C00 while other may load us at 0x07C0:0x0000.
                                      ; Do a far jump to fix this issue, and reload CS to 0x0000.
 
.FlushCS:
    cli
    ; Set up segment registers.
    xor ax, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld

    ; Set up stack so that it starts below Main.
    mov sp, 0x7c00

    mov byte [drive_num], dl
    
    sti
 
    ; Point edi to a free space bracket.
    mov edi, FREE_SPACE

    call load_disk

    ; Switch to Long Mode.
    jmp SwitchToPMode
 
.Die:
    hlt
    jmp .Die

drive_num: db 0

load_disk:
    mov ah, 0x02
    mov al, 0x13
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [drive_num]
    mov bx, LOAD_ADDRESS
    int 0x13
    mov ah, 0x00

    mov cx, TOTAL_SECTORS
.readloop:
    int 0x13

    add bx, 512
    inc dl
    dec cx
    jnz .readloop
    ret

ALIGN 4
IDT:
    .length dw 0
    .base dd 0

SwitchToPMode:
    cli
    mov cr0, eax
    or eax, 0x1
    mov eax, cr0

    jmp flush

flush:
    lidt [IDT]
    lgdt [gdtp]
    mov ax, (gdt_data_segment - gdt_start)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Just set the stack pointer as high as it can possibly go. The kernel will change this later.
    mov esp, 0xFFFFFFFF
    jmp 0x8:ProtectedMode

BITS 32
ProtectedMode:
    ; I give up on paging. I have spent 18 hours trying to figure it out. 32 bits were always better anyway.
    call kernelspace

die:
    cli
    hlt
    jmp die



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

; Pad out file.
times 510 - ($-$$) db 0
dw 0xAA55

section .bss

; Reserve the space for the kernel right after the bootloader (We can only load this much of it)
kernelspace:
    resb 65536
kernelspace_end: