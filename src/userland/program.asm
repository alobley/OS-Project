ORG 0
BITS 32
CPU 686

; Just a simple example program in assembly

%define SYS_WRITE 6                     ; Syscall number for sys_write
%define STDOUT_FILENO 1                 ; File descriptor 1 is stdout
%define SYS_GETCWD 14                   ; Syscall number for sys_getcwd
%define SYS_EXIT 8                      ; Syscall number for sys_exit
%define SYS_SLEEP 19                    ; Syscall number for sys_sleep
%define NEWL 0x0A                       ; Newline character (\n)

section .text
global _start
_start:
    ; Print a message confirming the program loaded and executed to the console
    mov eax, SYS_WRITE          ; Write to a file
    mov ebx, STDOUT_FILENO      ; File descriptor 1 is stdout
    mov ecx, msg                ; Pointer to the message
    mov edx, msg_len            ; Length of the message
    int 0x30                    ; Call the kernel to print the message

    ; Get the current working directory
    mov eax, SYS_GETCWD         ; Syscall number for sys_getcwd
    mov ebx, workingdir         ; Pointer to buffer for current working directory
    mov ecx, 256                ; Size of the buffer
    int 0x30                    ; Call the kernel to get the current working directory

    ; Print dirmsg to the console
    mov eax, SYS_WRITE          ; Write to a file
    mov ebx, STDOUT_FILENO      ; File descriptor 1 is stdout
    mov ecx, dirmsg             ; Pointer to the directory message
    mov edx, dirlen             ; Length of the directory message
    int 0x30                    ; Call the kernel to print the directory message

    ; Get the length of the current working directory
    mov ecx, workingdir         ; Pointer to the current working directory
    call strlen                 ; Calculate the length of the current working directory

    ; Print the current working directory
    mov eax, SYS_WRITE          ; Write to a file
    mov ebx, STDOUT_FILENO      ; File descriptor 1 is stdout
    mov ecx, workingdir         ; Pointer to the current working directory
    mov edx, edx                ; Length of the current working directory
    int 0x30                    ; Call the kernel to print the current working directory

    ; Print a newline
    mov eax, SYS_WRITE          ; Write to a file
    mov ebx, STDOUT_FILENO      ; File descriptor 1 is stdout
    mov ecx, end                ; Pointer to the newline
    mov edx, 1                  ; Length of the newline
    int 0x30                    ; Call the kernel to print the newline

    ; When we get proper process management working
    mov eax, SYS_EXIT           ; Syscall number for sys_exit
    mov ebx, 0                  ; Exit code 0
    int 0x30                    ; Call the kernel to exit the program


; Calculate the length of a string
; Input: ecx = pointer to the string
; Output: edx = length of the string
strlen:
    xor edx, edx               ; Clear edx (length)
.loop:
    cmp byte [ecx + edx], 0    ; Check if the current byte is null
    je .done                    ; If it is, we are done
    inc edx                     ; Increment length
    jmp .loop                   ; Repeat the loop
.done:
    ret                         ; Return to the caller


section .data
; Message to print to confirm the program was loaded
msg: db "I am a user program loaded from the disk!", NEWL
msg_len: equ $ - msg

; Message to print the current working directory
dirmsg: db "The current working directory of this program: "
dirlen: equ $ - dirmsg

end: db NEWL

section .bss
; Buffer for the current working directory
workingdir: resb 256
