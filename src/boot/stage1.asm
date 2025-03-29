ORG 0x7E00
CPU 586
BITS 16

%include "src/boot/boot.inc"

section .text
; CL should contain the drive number of the boot disk
stage1:
    ; We made it to stage 1!
    mov byte [driveno], cl                  ; Save the boot disk number

    ; Reset the disk controller
    ; This is necessary to ensure the disk controller is in a known state
    call disk_reset

    ; Primt a message confirming stage 1 loaded
    mov si, stage1msg
    mov ah, 0x07
    call printl

    ; Load the BPB from the boot sector of the boot disk
    mov ax, FS_BUFFER_SEGMENT
    mov es, ax

    mov ah, 0x02
    mov al, 1
    mov cx, 1
    movzx dx, byte [driveno]
    mov bx, 0
    int 0x13

    ; Print the OEM ID to confirm the BPB was properly loaded
    mov ax, FS_BUFFER_SEGMENT
    mov es, ax
    mov si, BPB_OEM_OFF
    mov ah, 0x07
    call printfar

    ; Enable the A20 line
    push .a20after
    call A20_chk
    cmp al, 0
    je .a20_disabled
    jne .a20_enabled
    .a20after:

    ; Get the memory size and memory map

    ; Get low memory in KB
    clc
    int 0x12
    jc error

    ; AX now holds the amount of contiguous KB in memory starting from 0x0000
    mov word [lowmemKB], ax

    mov si, mmapldmsg
    mov ah, 0x07
    call printl

    ; Get the memory map
    call GetMemoryMap

    mov si, mmapsucces
    mov ah, 0x07
    call printl

    ; Get high memory in KB
    call Get_Highmem

    mov si, gothimem
    mov ah, 0x07
    call printl

    ; Load the kernel piece by piece, switching between real and protected mode to load the kernel at the right spot
    mov si, kernelmsg
    mov ah, 0x07
    call printl

    mov ax, FS_BUFFER_SEGMENT
    mov fs, ax
    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax

    ; Get the correct disk geometry
    push es
    mov ah, 0x08
    mov dl, [driveno]
    int 0x13
    jc disk_err
    pop es

    and cl, 0x3F
    xor ch, ch
    mov si, BPB_SECTORS_PER_TRACK_OFF
    mov word [fs:si], cx
    ;inc dh
    ;mov si, BPB_NUM_HEADS_OFF
    ;mov word [fs:si], dx

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    call load_kernel

    ; Load GDT and jump to kernel

    ; Kernel returned to bootloader unexpectedly.
    ; jmp .error
    cli
    hlt

.a20_disabled:
    call A20_enable
    call A20_chk
    cmp al, 1
    je .a20_enabled

    mov si, a20no
    mov ah, 0x07
    call printl

    ; I support i586 and later. If the A20 line can't be enabled, it's likely a 286 or older.
    cli
    hlt

.a20_enabled:
    mov si, a20yes
    mov ah, 0x07
    call printl

    ret

driveno: db 0

error:
    call clear
    mov si, retmsg
    mov ah, 0x07
    call printl

    cli
    hlt

; Print a line to the VGA console
; DS:SI: pointer to the string
; AH: attribute
printl:
    ; Push the registers we will use (so as not to clobber them)
    push ax
    push gs
    push si
    push di

    mov di, [screenOffset]

    ; Set up the video memory segment
    push ax
    mov ax, VGA_TEXT_SEGMENT
    mov gs, ax
    pop ax
.loop:
    ; Put the next character into al
    mov al, byte [ds:si]

    ; If we hit a null terminator, we're done
    cmp al, 0
    je .done

    ; Otherwise, write the value of AX to the framebuffer
    mov word [gs:di], ax

    ; Increment the pointers
    add di, 2
    inc si

    ; Repeat the loop
    jmp .loop
.done:
    ; Move to the next line
    add word [screenOffset], 160

    ; Restore saved registers
    pop di
    pop si
    pop gs
    pop ax

    ; return
    ret

; Print a line from a far pointer
; ES:SI: pointer to the string
; AH: attribute
; DI: offset in the video memory
printfar:
    ; Push the registers we will use (so as not to clobber them)
    push ax
    push gs
    push si
    push di

    mov di, [screenOffset]

    ; Set up the video memory segment
    push ax
    mov ax, VGA_TEXT_SEGMENT
    mov gs, ax
    pop ax
.loop:
    ; Put the next character into al
    mov al, byte [es:si]

    ; If we hit a null terminator, we're done
    cmp al, 0
    je .done

    ; Otherwise, write the value of AX to the framebuffer
    mov word [gs:di], ax

    ; Increment the pointers
    add di, 2
    inc si

    ; Repeat the loop
    jmp .loop
.done:
    ; Move to the next line
    add word [screenOffset], 160

    ; Restore saved registers
    pop di
    pop si
    pop gs
    pop ax

    ; return
    ret

; Clear the screen
clear:
    push ax
    push gs
    push si

    mov ax, VGA_TEXT_SEGMENT
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

    mov word [screenOffset], 0

    ret

; Check if the A20 line is enabled
; Returns 0 if it is disabled, 1 if it is enabled
A20_chk:
    pushf
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
    sti

    ret

; Enable the A20 line
A20_enable:
    cli

    call a20wait1
    outb 0x64, 0xAD

    call a20wait1
    outb 0x64, 0xD0

    call a20wait2
    inb 0x60
    push ax

    call a20wait1
    outb 0x64, 0xD1

    call a20wait1
    pop ax
    or al, 0x02
    outb 0x60, al

    call a20wait1
    outb 0x64, 0xAE

    call a20wait1
    sti
    ret

a20wait1:
    ; Wait for the 8042 to be ready
    inb 0x64
    test al, 0x02
    jnz a20wait1

    ret

a20wait2:
    ; Wait for the 8042 to be ready
    inb 0x64
    test al, 0x01
    jnz a20wait2

    ret

; Get the memory map of the system
GetMemoryMap:
    push es
    push di
    push ax

    clc

    ; Set ES to the memory map buffer and DI to the start of the buffer
    mov ax, MMAP_BUFFER_SEGMENT
    mov es, ax
    mov di, 0

    xor eax, eax                    ; Just in case, clear EAX
    mov eax, 0xE820
    xor ebx, ebx
    mov ecx, 24
    mov edx, 0x534D4150
.loop:
    mov byte [es:di + MMAP_ENTRY_ACPI_OFF], 0x01        ; This makes the entry compatible with ACPI
    int 0x15
    jc .done
    cmp ebx, 0
    je .done

    cmp eax, 0x534D4150
    jne .mmap_error
    
    add di, MMAP_ENTRY_SIZE
    mov eax, 0xE820
    mov ecx, 24

.done:
    pop ax
    pop di
    pop es
    ret
.mmap_error:
    ; Error reading memory map
    mov si, mmaperr
    mov ah, 0x07
    call printl

    cli
    hlt

Get_Highmem:
    xor cx, cx
    xor dx, dx
    mov eax, 0xE801
    int 0x15
    jc error

    cmp ah, 0x86                ; Unsupported function
    je error

    cmp ah, 0x80                ; Invalid command
    je error

    jcxz .useax

    mov ax, cx
    mov bx, dx
.useax:
    ; AX = number of contiguous KB from 1M to 16M
    ; BX = number of contiguous 16KB pages above 16M
    mov word [himemKB], ax
    mov word [extmem16KB], bx
    ret

; FS:0: pointer to the BPB
; ES:0: pointer to the kernel's load segment
; Load the kernel from the disk
load_kernel:
    ; Get the number of root directory sectors
    mov si, BPB_NUM_ROOT_DIR_ENTRIES_OFF
    mov ax, word [fs:si]
    mov bx, 32
    mul bx
    mov si, BPB_BYTES_PER_SECTOR_OFF
    mov bx, word [fs:si]
    dec bx
    add ax, bx
    inc bx
    div bx
    mov word [.root_dir_sectors], ax

    ; Get the first data sector and root directory LBA
    mov si, BPB_NUM_FATS_OFF
    mov al, byte [fs:si]
    mov ah, 0
    mov si, BPB_SECTORS_PER_FAT_OFF
    mov bx, word [fs:si]
    mul bx
    mov si, BPB_RESERVED_SECTORS_OFF
    add ax, word [fs:si]
    mov word [.root_dir_lba], ax
    add ax, word [.root_dir_sectors]
    mov word [.first_data_sector], ax

    mov ax, word [.first_data_sector]

    ; Get the first FAT sector
    mov si, BPB_NUM_FATS_OFF
    movzx bx, byte [fs:si]
    mov si, BPB_SECTORS_PER_FAT_OFF
    mov ax, word [fs:si]
    mul bx
    mov si, BPB_RESERVED_SECTORS_OFF
    add ax, word [fs:si]
    mov word [.first_fat_sector], ax

    ; Read the root directory
    mov ax, STAGE1_CS
    mov es, ax
    mov bx, buffer
    mov ax, [.root_dir_lba]
    mov cx, word [.root_dir_sectors]
    mov dl, [driveno]
    push cx
    call lba_to_chs
    pop ax

    call load_sectors

    mov si, BPB_NUM_ROOT_DIR_ENTRIES_OFF
    mov cx, word [fs:si]
.search_loop:
    ; Search for the kernel entry in the root directory
    mov si, buffer + DIRENTRY_NAME_OFF
    add si, word [.bufoffset]

    mov ah, 0x07
    call printl

    push cx
    mov cx, 11
    mov di, kernelname
    repe cmpsb
    pop cx
    jne .next_entry

.kernel_found:
    mov si, buffer
    add si, DIRENTRY_FIRST_CLUS_LO_OFF
    add si, word [.bufoffset]
    mov ax, word [si]
    ;cli
    ;hlt
    mov word [.first_kernel_cluster], ax

    mov si, kernellocated
    mov ah, 0x07
    call printl

    jmp .load_data
.next_entry:
    add word [.bufoffset], DIRENTRY_SIZE
    dec cx
    jnz .search_loop


    mov si, kernel_not_found
    mov ah, 0x07
    call printl

    cli
    hlt
.load_data:
    ; Load the FAT into memory
    mov ax, STAGE1_CS
    mov es, ax
    mov bx, buffer

    mov ax, word [.first_fat_sector]
    mov cx, word [fs:BPB_SECTORS_PER_FAT_OFF] ; Number of sectors in the FAT
    mov dl, byte [driveno]
    push cx
    call lba_to_chs
    pop ax

    call load_sectors

    mov ax, word [.first_kernel_cluster]
    mov word [.currentcluster], ax

    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax

    xor bx, bx

.load_loop:
    ; Load the cluster from the disk

    ; Calculate the LBA offset
    push bx
    mov ax, word [.currentcluster]
    sub ax, 2
    xor bx, bx
    mov bl, byte [fs:BPB_SECTORS_PER_CLUSTER_OFF]
    mul bx
    add ax, word [.first_data_sector]
    pop bx
    ; Now we have the LBA of the current cluster in AX

    mov cl, byte [fs:BPB_SECTORS_PER_CLUSTER_OFF]
    mov dl, [driveno]
    push cx
    call lba_to_chs
    pop ax

    call load_sectors

    mov ax, [fs:BPB_BYTES_PER_SECTOR_OFF] ; Get the bytes per sector
    movzx cx, byte [fs:BPB_SECTORS_PER_CLUSTER_OFF]
    mul cx

    add bx, ax
    push bx

    ; Check if we have filled this segment with data (it should add up nicely. if not, that would suck very much)
    cmp bx, 0xFFFF
    je .next_segment

.fat_off:
    ; Calculate the FAT offset
    mov ax, word [.currentcluster]
    mov bx, word [.currentcluster]
    shr bx, 1
    add ax, bx

    ; Calculate the entry offset
    xor bx, bx
    mov bx, word [fs:BPB_BYTES_PER_SECTOR_OFF]
    div bx
    mov ax, dx
    add ax, buffer
    mov si, ax

    ; AX now contains the table value (although it has some extra bits because of FAT12)
    mov ax, word [si]

    mov bx, word [.currentcluster]
    and bx, 1

    cmp bx, 1
    je .odd

.even:
    and ax, 0x0FFF
    jmp .after
.odd:
    shr ax, 4
.after:
    ; AX now contains the next cluster number
    pop bx
    cmp ax, 0xFF7
    jge .done
    mov word [.currentcluster], ax
    jmp .load_loop
.next_segment:
    mov bx, es
    add bx, 0x1000
    cmp bx, EBDA_BASE_SEGMENT
    jge .done ; If we exceed the EBDA, stop loading more segments
    mov es, bx
    xor bx, bx
    jmp .fat_off
.done:
    jmp copy_kernel
    
    cli
    hlt


;.fatoffset: dw 0
.currentcluster: dw 0

.root_dir_sectors: dw 0
.root_dir_lba: dw 0
.first_data_sector: dw 0
.first_fat_sector: dw 0

.first_kernel_cluster: dw 0
.first_sector_of_cluster: dw 0

.bufoffset: dw 0
    

; Convert an LBA offset to CHS
; AX: LBA offset
; DL: drive number
; CL: Number of sectors to read
; FS:0: pointer to the BPB
; Returns the CHS address in the proper registers needed for the int 0x13 call
lba_to_chs:
    push ax
    push dx

    xor dx, dx                          ; dx = 0
    div word [fs:BPB_SECTORS_PER_TRACK_OFF]    ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack

    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [fs:BPB_NUM_HEADS_OFF]     ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                           ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                          ; restore DL
    pop ax
    ret

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
    mov dl, [driveno]

    int 0x13
    jc .error
.done:
    pop dx
    pop cx
    pop bx
    pop ax
    mov byte [.retries], 4 ; Reset retries on success

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

    call disk_reset
    dec byte [.retries]
    cmp byte [.retries], 0
    jg .retry
.final:
    ; Print an error message when retries are exhausted
    mov si, ferrmsg
    mov di, 0
    mov ah, 0x07
    call printl

    cli
    hlt
.retries: db 4

disk_reset:
    pusha
    mov ah, 0x00
    stc
    mov dl, [driveno]
    int 0x13
    jc disk_err
    popa
    ret

disk_err:
    mov si, ferrmsg
    mov ah, 0x07
    call printl

    cli
    hlt

kernel_not_found:
    mov si, nokernel
    mov ah, 0x07
    call printl

    cli
    hlt

kernel_load_error:
    mov si, kernelerr
    mov ah, 0x07
    call printl

    cli
    hlt

copy_kernel:
    cli
    cld

    mov si, bootdone
    mov ah, 0x07
    call printl

    mov ax, 0xFFFF
    hlt

    lgdt [gdtp]

    ; First configure your GDT properly
    ; Your code segment (0x8) should have a base address that matches where your code is
    ; OR use an absolute far jump that includes the correct physical address

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Do NOT set up segment registers before the jump - this must happen after
    ; when you're actually in protected mode with proper descriptors loaded

    ; Use the absolute physical address in the far jump
    jmp 0x8:pmode

BITS 32
pmode:
    ; Now set up segment registers in protected mode
    mov ax, 0x10  ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, PMODE_STACK_ADDR

    ; Now continue with protected mode code
    mov esi, 0xB8000
    mov ah, 0x0F
    mov al, 'H'
    mov word [esi], ax

    mov esi, PMODE_KERNEL_BUFFER_ADDR
    mov edi, KERNEL_FINAL_ADDRESS

    ; Just copy everything up to the BIOS extended data area (EBDA)
    mov ecx, PMODE_KERNEL_BUFFER_ADDR

.copyloop:
    cmp ecx, 0x9FC00
    jge .done
    ; Copy the kernel from the buffer to the final destination
    mov eax, dword [esi]
    mov dword [edi], eax
    ; Move to the next dword bytes
    add edi, 4
    add esi, 4
    sub cx, 4

.done:
    ; Copy memory information into the registers
    movzx eax, word [lowmemKB]
    movzx ebx, word [himemKB]
    add eax, ebx

    mov edx, eax
    movzx eax, word [extmem16KB]
    mov ebx, 16
    mul ebx
    add edx, eax

    ; Save the size of memory in EAX and a pointer to the memory map in EBX and jump to the kernel.
    mov eax, edx
    mov ebx, PMODE_MMAP_ADDR
    jmp KERNEL_FINAL_ADDRESS ; Jump to the kernel's entry point

    hlt
.old_sp: dw 0

kernelname: db "KERNEL  BIN", 0

screenOffset: dw 320
stage1msg: db "Stage 1 loaded successfully!", 0

a20yes: db "A20 line is enabled!", 0
a20no: db "A20 line could not be enabled.", 0
ferrmsg: db "Floppy error!", 0

mmapldmsg: db "Loading memory map...", 0
mmaperr: db "Error reading memory map!", 0
mmapsucces: db "Memory map loaded successfully!", 0

gothimem: db "Successfully obtained high memory size!", 0

retmsg: db "CRITICAL ERROR", 0

nokernel: db "Kernel not found!", 0
kernelmsg: db "Loading kernel...", 0
kernellocated: db "Kernel located! Loading clusters...", 0
kernelsucces: db "Kernel loaded successfully!", 0
kernelerr: db "Error loading kernel!", 0

bootdone: db "Jumping to kernel...", 0

; The offset of the read FAT location in the disk data buffer
fatoff: dw 0

; The offset of the root directory in the buffer, from the start of the buffer
diroff: dw 0

; The amount of low memory in KB up to the EBDA
lowmemKB: dw 0

; The amount of high memory (from 1M to 16M) in KB
himemKB: dw 0

; The amount of extended memory (from 16M to 4G) in 16KB blocks
extmem16KB: dw 0

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

; Load the FAT here (this can extend beyond the size of the reserved sectors)
buffer:

; Only allow the 10 extra reserved sectors for stage 1
times (512 * 10) - ($ - $$) db 0