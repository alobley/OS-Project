#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <common.h>
#include <interrupts.h>
#include <stdbool.h>

void kb_handle_install(struct Registers* regs);
void kb_handle_remove(struct Registers* regs);

void sys_write(struct Registers* regs);
void sys_read(struct Registers* regs);

void sys_istat(struct Registers* regs);



void sys_exec(struct Registers* regs);

#endif