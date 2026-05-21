#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#pragma pack(push, 1)
struct idt_entry_struct { 
    uint16_t base_low; 
    uint16_t sel; 
    uint8_t always0; 
    uint8_t flags; 
    uint16_t base_high; 
};
typedef struct idt_entry_struct idt_entry_t;

struct idt_ptr_struct { 
    uint16_t limit; 
    uint32_t base; 
};
typedef struct idt_ptr_struct idt_ptr_t;
#pragma pack(pop)

// Просто чистые прототипы функций
extern void idt_load(uint32_t ptr);
extern void keyboard_handler_asm(void);
extern void ignore_int_handler(void);

void init_idt(void);

#endif