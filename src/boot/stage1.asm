; 99% of this code is my own
BITS 16
;org 0x0

; Reference my 16-bit C code
extern Stage1_Start

section .text
; AX: Sectors per track
; BX: Number of heads
_start:
    mov word [sectorsPerTrack], ax
    mov word [numHeads], bx
    mov ax, 0xB800
    mov gs, ax

    call ClearScreen
    mov di, bootmsg
    call print

    ; Check if A20 line is enabled
    call Check_A20

    ; If disabled, enable it
    cmp ax, 0
    push .continue              ; Save return address
    je EnableA20

.continue:
    mov sp, 0x7C00      ; Restore stack pointer if A20 line was already enabled

    call Check_A20      ; Check if A20 line is enabled
    cmp ax, 0

    je unsupported      ; A20 line is still disabled, hardware unsupported

    ; Print a message to the screen indicating the success of the A20 line
    mov di, a20_success
    call print

    call GetMemoryMap

    call Stage1_Start
    
    cli
    hlt
    jmp $


; AX = 0: A20 line is disabled
; AX = 1: A20 line is enabled
Check_A20:
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

EnableA20:
    ; Enable A20 line using the fast A20 method (We can assume the computer is AT or newer)
    in al, 0x92
    test al, 2
    jnz .exit
    or al, 2
    and al, 0xFE
    out 0x92, al
.exit:
    ret

global print:function (print.end - print)
; DI: Pointer to the string to print
print:
    push ax
    push di
    push si
    mov ax, 0xB800
    mov gs, ax
    mov si, [screenoffset]
.loop:
    mov ah, 0x0F
    mov al, byte [di]
    mov word [gs:si], ax
    inc di
    add si, 2
    cmp byte [di], 0
    jne .loop
    jmp .done
.done:
    add word [screenoffset], 160
    pop si
    pop di
    pop ax
    ret
.end:

global ClearScreen:function (ClearScreen.end - ClearScreen)
ClearScreen:
    push di
    push cx
    mov cx, 80*25
    mov di, 0
.loop:
    mov word [gs:di], 0x0000
    add di, 2
    dec cx
    cmp cx, 0
    jne .loop
.done:
    pop cx
    pop di
    mov word [screenoffset], 0
    ret
.end:
    

unsupported:
    ; Unsupported hardware
    mov di, badsystem
    mov si, [screenoffset]
    call print

.done:
    cli
    hlt
    jmp $

;
; Converts an LBA address to a CHS address
; Parameters:
;   - ax: LBA address
; Returns:
;   - cx [bits 0-5]: sector number
;   - cx [bits 6-15]: cylinder
;   - dh: head
;

lba_to_chs:

    push ax
    push dx

    xor dx, dx                          ; dx = 0
    div word [sectorsPerTrack]          ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack

    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [numHeads]                 ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                           ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                          ; restore DL
    pop ax
    ret

GetMemoryMap:
    mov di, mmapinit
    call print

    mov ax, 0x8000
    mov es, ax

    mov ax, 0xE820

    db 0x66
    mov edx, 0x534D4150
    mov bx, 0
    mov cx, 24
    mov di, 0

    int 0x15

    jc .fail
.loop:
    add di, 24

    db 0x66
    cmp eax, 0x534D4150
    jne .fail
    cmp ebx, 0
    je .done

    db 0x66
    mov eax, 0x534D4150
    mov ecx, 24
    int 0x15
    jnc .loop

.done:
    ret
.fail:
    jmp unsupported


;
; Reads sectors from a disk
; Parameters:
;   - ax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;
global disk_read:function (disk_read.end - disk_read)
disk_read:

    push ax                             ; save registers we will modify
    push bx
    push cx
    push dx
    push di

    push cx                             ; temporarily save CL (number of sectors to read)
    call lba_to_chs                     ; compute CHS
    pop ax                              ; AL = number of sectors to read
    
    mov ah, 02h
    mov di, 3                           ; retry count

.retry:
    pusha                               ; save all registers, we don't know what bios modifies
    stc                                 ; set carry flag, some BIOS'es don't set it
    int 13h                             ; carry flag cleared = success
    jnc .done                           ; jump if carry not set

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail:
    ; all attempts are exhausted
    jmp DiskError

.done:
    popa

    pop di
    pop dx
    pop cx
    pop bx
    pop ax                             ; restore registers modified
    ret
.end:


;
; Resets disk controller
; Parameters:
;   dl: drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc DiskError
    popa
    ret

DiskError:
    mov di, diskerr
    call print
    cli
    hlt
    jmp $

section .data
numHeads: dw 0
sectorsPerTrack: dw 0

screenoffset: dw 0

badsystem: db "System unsupported", 0

bootmsg: db "Booting OS...", 0

a20_success: db "A20 line enabled", 0

mmapinit: db "Getting memory map...", 0

diskerr: db "Disk error", 0

kernelname: db "KERNEL  BIN", 0