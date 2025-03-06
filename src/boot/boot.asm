ORG 0x7C00
BITS 16
CPU 686

;
; FAT12 header
; 
jmp short start
nop

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

start:
    ; Set up segments and stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7C00

    ; Set up text mode video segment
    mov ax, 0xB800
    mov gs, ax

    ; Save drive number from DL
    mov byte [ebr_drive_number], dl

    ; Print boot message
    mov si, bootmsg
    call print

    ; Get drive parameters
    push es
    mov ah, 0x08
    int 0x13
    jc floppy_error          ; Add error handling if get drive params fails
    pop es

    and cl, 0x3F
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh
    mov [bdb_heads], dh

    ; Load the second stage bootloader
    ; Calculate where to read from - start from the first reserved sector (sector 1)
    mov ax, 1                ; Start from sector 1 (right after the boot sector)
    mov bx, 0x0000           ; Destination address offset
    mov es, bx               ; Use segment 0
    mov bx, 0x8000           ; Load to 0000:8000
    mov dl, [ebr_drive_number]
    mov cl, 10               ; Read 10 sectors (5KB)
    
    call disk_read
    
    ; Far jump to the loaded second stage
    jmp 0:0x8000             ; Jump to where we loaded the bootloader

    ; Should never reach here
    cli
    hlt
    jmp $

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

lba_to_chs:
    push ax
    push dx

    xor dx, dx                          ; dx = 0
    div word [bdb_sectors_per_track]    ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack

    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [bdb_heads]                ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                           ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                          ; restore DL
    pop ax
    ret


;
; Reads sectors from a disk
; Parameters:
;   - ax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;
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
    jmp floppy_error

.done:
    popa

    pop di
    pop dx
    pop cx
    pop bx
    pop ax                             ; restore registers modified
    ret

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
    jc floppy_error
    popa
    ret

floppy_error:
    mov si, errmsg
    call print
    mov ah, 0
    int 16h
    jmp 0FFFFh:0                ; jump to beginning of BIOS, should reboot

.halt:
    cli                         ; disable interrupts, this way CPU can't get out of "halt" state
    hlt

bootmsg: db "Loading bootloader stage 1...", 0x0D, 0x0A, 0
;loadmsg: db "Stage 1 loaded, jumping to code...", 0x0D, 0x0A, 0
errmsg: db "Error reading disk", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55