[org 0x7c00]
bits 16

KERNEL_OFFSET equ 0x1000    ; Куда BIOS скопирует Си-ядро в ОЗУ

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov bp, 0x7c00
    mov sp, bp
    sti

    ; --- ЧИТАЕМ СИ-ЯДРО ИЗ СЛЕДУЮЩИХ СЕКТОРОВ ---
    mov bx, KERNEL_OFFSET   
    mov dh, 30              
    mov ah, 0x02            
    mov al, dh
    mov ch, 0x00            ; Цилиндр 0
    mov dh, 0x00            ; Головка 0
    mov cl, 0x02            ; Начинаем строго со 2-го сектора диска
    mov dl, 0x00            ; Дисковод A:

    int 0x13
    jc .disk_error          

    ; Переход в 32-битный Защищенный режим
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Длинный прыжок для сброса CS в 0x08
    jmp 0x08:init_pm

.disk_error:
    jmp $

; ==============================================================================
bits 32
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebp, 0x90000
    mov esp, ebp

    ; СРАЗУ ПРЫГАЕМ НА СИ-ЯДРО (оно уже лежит по адресу 0x1000)
    call KERNEL_OFFSET

.halt:
    cli
    hlt
    jmp .halt

; ==============================================================================
align 4
gdt_start:
    dd 0x0, 0x0
gdt_code:
    dw 0xffff, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xffff, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; --- СТРОГАЯ РАЗМЕТКА СЕКТОРОВ ---

; Добиваем ПЕРВЫЙ сектор ровно до 510 байт
times 510 - ($ - $$) db 0
; Магическая сигнатура BIOS на 511 и 512 байтах
dw 0xaa55

; С этого момента начинается ФИЗИЧЕСКИЙ ВТОРОЙ СЕКТОР диска.
; Внедряем сюда наше скомпилированное Си-ядро.
incbin "kernel.bin"

; Добиваем весь файл нулями до стандартного размера дискеты, чтобы BIOS не ругался
times 1474560 - ($ - $$) db 0
