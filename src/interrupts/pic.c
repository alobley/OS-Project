#include <interrupts.h>
#include <system.h>

#define PIC_WAIT() do {         \
        asm volatile ("jmp 1f\n\t"       \
                "1:\n\t"        \
                "    jmp 2f\n\t"\
                "2:");          \
    } while (0)

static void (*handlers[32])(struct Registers* regs) = {0};

static void stub(struct Registers* regs){
    if(regs->int_no <= 47 && regs->int_no >= 32){
        if(handlers[regs->int_no - 32]){
            handlers[regs->int_no - 32](regs);
        }
    }

    if(regs->int_no >= 0x40){
        outb(PIC2, PIC_EOI);
    }

    outb(PIC1, PIC_EOI);
}

static void RemapIRQ(){
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    outb(PIC1, ICW1_INIT | ICW1_ICW4);
    outb(PIC2, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, PIC1_OFFSET);
    outb(PIC2_DATA, PIC2_OFFSET);
    outb(PIC1_DATA, 0x04); // PIC2 at IRQ2
    outb(PIC2_DATA, 0x02); // Cascade indentity
    outb(PIC1_DATA, PIC_MODE_8086);
    outb(PIC1_DATA, PIC_MODE_8086);
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}


static void SetIRQMask(size_t i){
    uint16_t port = i < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t value = inb(port) | 1 << i;
    outb(port, value);
}


static void ClearIRQMask(size_t i){
    uint16_t port = i < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t value = inb(port) & ~(1 << i);
    outb(port, value);
}

int InstallIRQ(size_t i, void (*handler)(struct Registers*)){
    if(handlers[i] != stub && handlers[i] != NULL){
        return STANDARD_FAILURE; // IRQ already installed
    }
    cli;
    handlers[i] = handler;
    ClearIRQMask(i);
    sti;

    return STANDARD_SUCCESS;
}

int RemoveIRQ(size_t i){
    cli;
    handlers[i] = stub;
    SetIRQMask(i);
    sti;
    return STANDARD_SUCCESS;
}

void InitIRQ(){
    RemapIRQ();

    for(size_t i = 0; i < 16; i++){
        InstallISR(32 + i, stub);
    }
}