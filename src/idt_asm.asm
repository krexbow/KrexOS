[bits 32]
section .text

; Экспортируем оба варианта имён (для Linux и для Windows схем линковки)
global idt_load
global _idt_load
global keyboard_handler_asm
global _keyboard_handler_asm
global ignore_int_handler
global _ignore_int_handler

; Импортируем Си-функцию (тоже оба варианта на случай капризов компилятора)
extern keyboard_handler_main
extern _keyboard_handler_main

; Названия меток для Си без подчёркивания
idt_load:
_idt_load:
    mov edx, [esp + 4]
    lidt [edx]
    ret

ignore_int_handler:
_ignore_int_handler:
    iretd

keyboard_handler_asm:
_keyboard_handler_asm:
    pushad
    cld
    ; Вызываем ту функцию, которую импортировал компилятор
    call _keyboard_handler_main
    popad
    iretd