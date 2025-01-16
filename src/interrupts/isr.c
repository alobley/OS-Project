#include "isr.h"
#include "idt.h"
#include <vga.h>
#include <util.h>
#include <time/time.h>
#include <disk/vfs.h>
#include <keyboard.h>

#define NUM_ISRS 49

extern void _isr0(struct Registers*);
extern void _isr1(struct Registers*);
extern void _isr2(struct Registers*);
extern void _isr3(struct Registers*);
extern void _isr4(struct Registers*);
extern void _isr5(struct Registers*);
extern void _isr6(struct Registers*);
extern void _isr7(struct Registers*);
extern void _isr8(struct Registers*);
extern void _isr9(struct Registers*);
extern void _isr10(struct Registers*);
extern void _isr11(struct Registers*);
extern void _isr12(struct Registers*);
extern void _isr13(struct Registers*);
extern void _isr14(struct Registers*);
extern void _isr15(struct Registers*);
extern void _isr16(struct Registers*);
extern void _isr17(struct Registers*);
extern void _isr18(struct Registers*);
extern void _isr19(struct Registers*);
extern void _isr20(struct Registers*);
extern void _isr21(struct Registers*);
extern void _isr22(struct Registers*);
extern void _isr23(struct Registers*);
extern void _isr24(struct Registers*);
extern void _isr25(struct Registers*);
extern void _isr26(struct Registers*);
extern void _isr27(struct Registers*);
extern void _isr28(struct Registers*);
extern void _isr29(struct Registers*);
extern void _isr30(struct Registers*);
extern void _isr31(struct Registers*);
extern void _isr32(struct Registers*);
extern void _isr33(struct Registers*);
extern void _isr34(struct Registers*);
extern void _isr35(struct Registers*);
extern void _isr36(struct Registers*);
extern void _isr37(struct Registers*);
extern void _isr38(struct Registers*);
extern void _isr39(struct Registers*);
extern void _isr40(struct Registers*);
extern void _isr41(struct Registers*);
extern void _isr42(struct Registers*);
extern void _isr43(struct Registers*);
extern void _isr44(struct Registers*);
extern void _isr45(struct Registers*);
extern void _isr46(struct Registers*);
extern void _isr47(struct Registers*);
extern void _isr48(struct Registers*);

extern fat_disk_t* fatdisks[MAX_DRIVES];

//extern void syscall_handler(struct Registers* regs);

// This is what is processed when you perform an ABI call (int 0x30). It works!
void syscall_handler(struct Registers *regs){
    switch(regs->eax){
        case 0:
            // SYS_DEBUG
            WriteStr("Debug!\n");
            break;
        case 1:
            // SYS_PRINT
            // EBX = pointer to string
            WriteStr((char*)regs->ebx);
            break;
        case 2:
            // SYS_CREATE_TIMER
            // EBX = callback function pointer
            // ECX = interval in ms
            AddTimerCallback((TimerCallback)regs->ebx, regs->ecx);
            break;
        case 3:
            // SYS_REMOVE_TIMER
            // EBX = pointer to callback function
            RemoveTimerCallback(regs->ebx);
            break;
        case 4:
            // SYS_FSEEK
            // EBX = Pointer to string containing the name of the file to search for
            // Returns a pointer to the file_t struct's data, in EBX
            char* fileName = (char*)regs->ebx;
            regs->ebx = (uint32)GetFile(fileName)->data;
        case 5:
            // SYS_SET_VGA_MODE
            // EBX = mode
            // Right now we don't need to provide a pointer to the VGA buffer since there's no virtual memory
            VGA_SetMode(regs->ebx);
            break;
        case 6:
            // SYS_SET_VGA_PALETTE
            VGA_SetPalette((color_rgb_t*)regs->ebx);
        case 7:
            // SYS_GET_KEY
            // Returns the key in EAX
            regs->eax = GetKey();
        default:
            WriteStr("Invalid syscall!\n");
            break;
    }
}

static void (*stubs[NUM_ISRS])(struct Registers*) = {
    _isr0,
    _isr1,
    _isr2,
    _isr3,
    _isr4,
    _isr5,
    _isr6,
    _isr7,
    _isr8,
    _isr9,
    _isr10,
    _isr11,
    _isr12,
    _isr13,
    _isr14,
    _isr15,
    _isr16,
    _isr17,
    _isr18,
    _isr19,
    _isr20,
    _isr21,
    _isr22,
    _isr23,
    _isr24,
    _isr25,
    _isr26,
    _isr27,
    _isr28,
    _isr29,
    _isr30,
    _isr31,
    _isr32,
    _isr33,
    _isr34,
    _isr35,
    _isr36,
    _isr37,
    _isr38,
    _isr39,
    _isr40,
    _isr41,
    _isr42,
    _isr43,
    _isr44,
    _isr45,
    _isr46,
    _isr47,
    _isr48
};

static const char *exceptions[32] = {
    "Divide by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "OOB",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unrecognized interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED"
};

static struct {
    size_t index;
    void (*stub)(struct Registers*);
} isrs[NUM_ISRS];

static void (*handlers[NUM_ISRS])(struct Registers*) = { 0 };

void InstallISR(size_t i, void (*handler)(struct Registers*)){
    handlers[i] = handler;
}

void ISRHandler(struct Registers *regs){
    if (handlers[regs->intNum]) {
        handlers[regs->intNum](regs);
    }
}

extern void reboot();
static void ExceptionHandler(struct Registers *regs){
    ClearTerminal();
    WriteStr(exceptions[regs->intNum]);
    cli;
    for(;;) hlt;
}

void InitISR(){
    for(size_t i = 0; i < NUM_ISRS; i++){
        isrs[i].index = i;
        isrs[i].stub = stubs[i];
        SetIDT(isrs[i].index, isrs[i].stub, 0x08, 0x8E);
    }

    for(size_t i = 0; i < 32; i++){
        InstallISR(i, ExceptionHandler);
    }

    InstallISR(SYSCALL_INT, syscall_handler);
}