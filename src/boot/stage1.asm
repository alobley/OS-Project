; stage1.asm
; Stage 1 bootloader
; This is stored in 10 reserved sectors in a FAT filesystem

BITS 16
CPU 686
;ORG 0x8000

%macro STOP 0
    cli
    hlt
    jmp $
%endmacro

%define ENDL 0x0D, 0x0A

section .text.start
extern stage2_main

global bootmain
bootmain:
    mov sp, 0x7C00
    mov al, 9
    call ScrollScreen

    ; Get the cursor position
    mov ah, 0x03
    mov bh, 0
    int 0x10

    ; Move the cursor up after the scroll
    sub dh, 9
    mov ah, 0x02
    mov bh, 0
    int 0x10

    mov si, bootmsg
    call print

    mov si, a20msg
    call print
    call EnableA20

    call CheckA20
    cmp ax, 1
    jne bootfail

    mov si, a20success
    call print

    mov si, memmsg
    call print
    call GetMemoryMap

    mov si, memsuccess
    call print

    mov si, unrealmsg
    call print
    call enterunreal

    mov si, unrealsuccess
    call print

    ; Move the cursor off the screen
    mov dh, 255
    mov dl, 255
    mov ah, 0x02
    mov bh, 0
    int 0x10

    ; The system is more or less initialized now. Jump to the C code for easier disk access and boot environment
    ; Use fastcall in C to make this easier
    movzx ecx, word [nummmapentries]
    jmp stage2_main

    ; Should never reach here
    mov si, errmsg
    call print
    
    cli
    hlt
    jmp $

section .text

; SI: pointer to string
print:
    xor bh, bh
    mov ah, 0x0E

.nextchar:
    lodsb                    ; Load byte from SI into AL and increment SI

    cmp al, 0
    je .done

    int 0x10
    jmp .nextchar
.done:
    ret
.end:

ClearScreen:
    mov ah, 0x06
    mov al, 0
    mov bh, 0x07
    mov cx, 0
    mov dx, 0x184F
    int 0x10
    ret
.end:

global ScrollScreen:function (ScrollScreen.end - ScrollScreen)
; AL: number of lines to scroll
ScrollScreen:
    mov ah, 0x06
    mov bh, 0x07
    mov ch, 0
    mov cl, 0
    mov dh, 0x18
    mov dl, 0x4F
    int 0x10
    ret
.end:

bootfail:
    mov si, errmsg
    call print
    cli
    hlt
    jmp $

EnableA20:
    cli

    call    a20wait
    mov     al,0xAD
    out     0x64,al

    call    a20wait
    mov     al,0xD0
    out     0x64,al

    call    a20wait2
    in      al,0x60
    push    eax

    call    a20wait
    mov     al,0xD1
    out     0x64,al

    call    a20wait
    pop     eax
    or      al,2
    out     0x60,al

    call    a20wait
    mov     al,0xAE
    out     0x64,al

    call    a20wait
    sti

    ret
.end:

a20wait:
    in      al,0x64
    test    al,2
    jnz     a20wait
    ret

a20wait2:
    in      al,0x64
    test    al,1
    jz      a20wait2
    ret

CheckA20:
    pushf       ; Save flags
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax
    mov es, ax

    not ax
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax

    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al

    pop ax
    mov byte [es:di], al

    mov ax, 0
    je .exit

    mov ax, 1
.exit:
    pop si
    pop di
    pop es
    pop ds
    popf

    ret
.end:

GetMemoryMap:
    mov ax, 0xC000
    mov es, ax
    xor di, di
    xor cx, cx
    xor ebx, ebx        ; Start with ebx = 0
    mov edx, 0x534D4150 ; "SMAP" in ASCII
    
.loop:
    mov eax, 0xE820
    mov ecx, 24         ; Entry size we want
    int 0x15
    
    jc .exit            ; Carry flag indicates error/end
    cmp eax, 0x534D4150 ; Check for "SMAP" response
    jne .exit
    
    add di, 24          ; Move to next entry
    inc word [nummmapentries]
    
    cmp ebx, 0          ; If ebx=0, we're done
    je .exit
    
    jmp .loop           ; Continue for more entries
    
.exit:
    cmp di, 0
    je bootfail
    ret
.end:

enterunreal:
    cli
    push ds
    lgdt [gdtinfo]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

pmode:
    mov bx, 0x10
    mov ds, bx

    mov bx, 0x18
    mov es, bx

    and al, 0xFE
    mov cr0, eax
    jmp 0x0:unreal

unreal:
    pop ds
    sti
    ret

section .rodata
gdtinfo:
   dw gdt_end - gdt - 1   ;last byte in table
   dd gdt                 ;start of table

gdt:        dd 0,0        ; entry 0 is always unused
codedesc:   db 0xff, 0xff, 0, 0, 0, 10011010b, 00000000b, 0
flatdesc:   db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0
; Set to 0x200000
fardesc:   
    dw 0xFFFF       ; Limit (4GB segment)
    dw 0x0000       ; Base Low  (bits 0-15)
    db 0x00         ; Base Middle (bits 16-23)
    db 0b10010010   ; Access Byte (Data segment, Read/Write)
    db 0b11001111   ; Granularity (4KB blocks, 32-bit segment)
    db 0x02         ; Base High (bits 24-31): Changed from 0x20 to 0x02 for base 0x200000
gdt_end:

section .data
bootmsg: db "Reached Stage 1", ENDL, 0
a20msg: db "Enabling A20...", ENDL, 0
a20success: db "A20 enabled", ENDL, 0
memmsg: db "Getting memory map...", ENDL, 0
memsuccess: db "Memory map obtained", ENDL, 0
unrealmsg: db "Entering Unreal Mode...", ENDL, 0
unrealsuccess: db "Unreal Mode entered", ENDL, 0

errmsg: db "BOOT ERROR", ENDL, 0

nummmapentries: dw 0

section .bss
