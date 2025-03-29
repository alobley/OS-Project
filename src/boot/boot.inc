%define STAGE1_CS 0x0000
%define STAGE1_SS 0x2000

; Define the VGA region of memory
%define VGA_TEXT_SEGMENT 0xB800
%define VGA_SEGMENT 0xA000
%define VGA_END_SEGMENT 0xC000

; Define the segments of low memory
%define FREE_SPACE_BASE_SEGMENT 0x0050          ; Space guaranteed to be free starts here
%define EBDA_BASE_SEGMENT 0x9FC0                ; Space guaranteed to be free ends here
%define BIOS_DATA_AREA_SEGMENT 0x0040           ; Likely won't be used, just for convenience

; Buffer for containing the actual data read from the disk
%define KERNEL_LOAD_SEGMENT 0x6000
%define KERNEL_FINAL_ADDRESS 0x200000

; Buffer for containing the filesystem data (such as the BPB and FATs)
%define FS_BUFFER_SEGMENT 0x3000

; Buffer containing the memory map
%define MMAP_BUFFER_SEGMENT 0x4000

; It might be a good idea to pass a multiboot header to the kernel (since it needs that to function properly)
%define MBHDR_BUFFER_SEGMENT 0x5000

; Offsets for the FAT12 BPB and EBR
%define BPB_OEM_OFF 0x03                        ; Is 8 bytes long
%define BPB_BYTES_PER_SECTOR_OFF 0x0B           ; Is a word
%define BPB_SECTORS_PER_CLUSTER_OFF 0x0D        ; Is a byte
%define BPB_RESERVED_SECTORS_OFF 0x0E           ; Is a word
%define BPB_NUM_FATS_OFF 0x10                   ; Is a byte
%define BPB_NUM_ROOT_DIR_ENTRIES_OFF 0x11       ; Is a word
%define BPB_TOTAL_SECTORS_OFF 0x13              ; Is a word
%define BPB_MEDIA_DESCRIPTOR_OFF 0x15           ; Is a byte
%define BPB_SECTORS_PER_FAT_OFF 0x16            ; Is a word
%define BPB_SECTORS_PER_TRACK_OFF 0x18          ; Is a word
%define BPB_NUM_HEADS_OFF 0x1A                  ; Is a word
%define BPB_HIDDEN_SECTORS_OFF 0x1C             ; Is a dword
%define BPB_TOTAL_SECTORS32_OFF 0x20            ; Is a dword

%define EBR_DRIVE_NUM_OFF 0x24                  ; Is a byte
%define EBR_NT_RESERVED_OFF 0x25                ; Is a byte
%define EBR_SIGNATURE_OFF 0x26                  ; Is a byte
%define EBR_SERIAL_NUM_OFF 0x27                 ; Is a dword
%define EBR_VOLUME_LABEL_OFF 0x2B               ; Is 11 bytes long
%define EBR_SYSTEM_ID_OFF 0x36                  ; Is 8 bytes long

%define MBR_SIGNATURE_OFF 0x1FE                 ; Is a word (should be 0xAA55)

; Directory parsing information
%define DIRENTRY_NAME_OFF 0                     ; Is 11 bytes long (8.3 file name)
%define DIRENTRY_ATTR_OFF 11                    ; Is a byte
%define DIRENTRY_NTRES_OFF 12                   ; Is a byte
%define DIRENTRY_CRTTIME_OFF 13                 ; Is a byte
%define DIRENTRY_CRTTIME_OFF 14                 ; Is a word
%define DIRENTRY_CRTDATE_OFF 16                 ; Is a word
%define DIRENTRY_LSTACCESSDATE_OFF 18           ; Is a word
%define DIRENTRY_FIRST_CLUS_HI_OFF 20           ; Is a word (should always be 0 on FAT12/FAT16)
%define DIRENTRY_LASTMODTIME_OFF 22             ; Is a word
%define DIRENTRY_LASTMODDATE_OFF 24             ; Is a word
%define DIRENTRY_FIRST_CLUS_LO_OFF 26           ; Is a word
%define DIRENTRY_FILESIZE_OFF 28                ; Is a dword (file size in bytes)
%define DIRENTRY_SIZE 32                        ; Size of a directory entry in bytes

; Memory map information
%define MMAP_ENTRY_SIZE 24
%define MMAP_ENTRY_BASE_ADDR_OFF 0x00           ; Is a qword
%define MMAP_ENTRY_LENGTH_OFF 0x08              ; Is a qword
%define MMAP_ENTRY_TYPE_OFF 0x10                ; Is a dword
%define MMAP_ENTRY_ACPI_OFF 0x14                ; Is a dword

%define PMODE_STACK_ADDR 0x20000
%define PMODE_KERNEL_BUFFER_ADDR 0x60000
%define PMODE_MMAP_ADDR 0x40000
%define PMODE_MBOOT_HDR_ADDR 0x50000

; Syntax:
; outb port, value
; Clobbers DX and AL
%macro outb 2
    mov dx, %1
    mov al, %2
    out dx, al
%endmacro

; Syntax:
; inb port
; Clobbers DX
; Returns the value in AL
%macro inb 1
    mov dx, %1
    in al, dx
%endmacro

; Syntax:
; outw port, value
; Clobbers DX and AX
%macro outw 2
    mov dx, %1
    mov ax, %2
    out dx, ax
%endmacro

; Syntax:
; inw port
; Clobbers DX
; Returns the value in AX
%macro inw 1
    mov dx, %1
    in ax, dx
%endmacro