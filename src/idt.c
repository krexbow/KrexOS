#include "idt.h"

idt_entry_t idt[256];
idt_ptr_t idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = selector;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void init_idt(void) {
    idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)ignore_int_handler, 0x08, 0x8E);
    }

    idt_set_gate(33, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
}