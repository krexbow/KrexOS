[bits 32]
[extern _kernel_main] ; Ссылаемся на функцию из kernel.c

global _start
_start:
    call _kernel_main ; Жестко вызываем Си
    jmp $             ; Зависаем, если Си вернул управление

