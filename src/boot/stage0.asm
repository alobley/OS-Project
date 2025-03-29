ORG 0x7C00
CPU 586
BITS 16

; Reserve space for the BPB
jmp short boot
nop

; FAT12 BPB
bdb_oem:                    db 'DEDOSDEV'           ; 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:    db 1
bdb_reserved_sectors:       dw 11
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880                 ; 2880 * 512 = 1.44MB
bdb_media_descriptor_type:  db 0F0h                 ; F0 = 3.5" floppy disk
bdb_sectors_per_fat:        dw 9                    ; 9 sectors/fat
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; extended boot record
ebr_drive_number:           db 0                    ; 0x00 floppy, 0x80 hdd, useless
                            db 0                    ; reserved
ebr_signature:              db 29h
ebr_volume_id:              db 12h, 34h, 56h, 78h   ; serial number, value doesn't matter
ebr_volume_label:           db 'BOOTDISK   '        ; 11 bytes, padded with spaces
ebr_system_id:              db 'FAT12   '           ; 8 bytes

section .text

; For setting up a proper segmented memory model
%define STAGE1_CS 0x07E0
%define STAGE1_SS 0x2000

%define VGA_SEGMENT 0xB800

boot:
    ; Far jump to reload CS
    jmp 0:start

start:
    ; Clear interrupts
    cli

    ; Set the other segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack
    mov sp, 0x7C00
    mov bp, 0x0000

    ; Save the boot disk number
    mov byte [disk_no], dl

    ; Enable interrupts
    sti

    call clear

    ; Print the boot message
    mov si, bootmsg
    mov di, 0
    mov ah, 0x07
    call print

    mov ax, STAGE1_CS
    mov es, ax

    ; Load stage 1 (the next 10 sectors)
    ; Since we only have 512 bytes, there's not much else we can do here.
    mov bx, 0
    mov al, 10
    mov ch, 0
    mov cl, 2
    mov dh, 0
    call load_sectors

    ; Print a boot success message
    mov si, successmsg
    mov di, 160
    mov ah, 0x07
    call print

    ; Set the stack segment
    mov ax, STAGE1_SS
    mov ss, ax
    mov sp, 0xFFFF
    mov bp, 0

    ; Set the data segments (they will just be where the code segment is, we're loading raw binary)
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Pass the disk number to stage 1
    mov cl, [disk_no]
    ; Far jump to stage 1
    jmp 0x7E00

    ; If we didn't jump, print an error message and halt the system
    mov si, jerrmsg
    mov di, 160 * 2
    mov ah, 0x07
    call print
    
    cli
    hlt

; ES:BX: Pointer to the data buffer
; AL: Number of sectors to read
; CH: low 8 bits of the cylinder number
; CL: bits 0-5 of the sector number and high 2 bits of the cylinder number
; DH: head number
; Clobbers ah, dl, and di
load_sectors:
    push ax
    push bx
    push cx
    push dx

.retry:
    mov ah, 0x02
    mov dl, [disk_no]

    int 0x13
    jc .error

.done:
    pop dx
    pop cx
    pop bx
    pop ax

    ret
.error:
    ; In case of an error, try again
    ; Allow 4 retries

    ; Reload the registers
    pop dx
    pop cx
    pop bx
    pop ax
    push ax
    push bx
    push cx
    push dx

    dec byte [.retries]
    cmp byte [.retries], 0
    jg .retry

    ; Print an error message when retries are exhausted
    call clear
    mov si, ferrmsg
    mov di, 0
    mov ah, 0x07
    call print

    cli
    hlt
.retries: db 4

; DS:SI: pointer to the string
; AH: attribute
; DI: offset in the video memory
print:
    ; Push the registers we will use (so as not to clobber them)
    push ax
    push gs
    push si
    push di

    ; Set up the video memory segment
    push ax
    mov ax, VGA_SEGMENT
    mov gs, ax
    pop ax

.loop:
    ; Put the next character into al
    mov al, byte [ds:si]

    ; If we hit a null terminator, we're done
    cmp al, 0
    je .done

    ; Otherwise, write the value of EAX to the framebuffer
    mov word [gs:di], ax

    ; Increment the pointers
    add di, 2
    inc si

    ; Repeat the loop
    jmp .loop
.done:
    pop di
    pop si
    pop gs
    pop ax

    ret

clear:
    push ax
    push gs
    push si

    mov ax, VGA_SEGMENT
    mov gs, ax
    xor si, si

.loop:
    mov word [gs:si], 0
    add si, 2
    cmp si, 4000
    jge .done
    jmp .loop
.done:
    pop si
    pop gs
    pop ax

    ret


disk_no: db 0

bootmsg: db 'Bootloader loaded!', 0
ferrmsg: db 'Floppy error!', 0
jerrmsg: db 'Could not jump to stage 1!', 0
successmsg: db 'Bootloader loaded successfully!', 0

times 510 - ($ - $$) db 0
dw 0xAA55