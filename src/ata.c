#include "ata.h"

// Нам нужны внешние функции вывода из kernel.c, чтобы печатать на экран
extern void print_string(const char* str, int row, int col, char color);
extern void clear_screen(void);

// Функция считывает инфу о диске
void ata_identify() {
    // 1. Выбираем мастер-диск (0xA0) на первичном канале
    outb(0x1F6, 0xA0);
    
    // 2. Сбрасываем сектора в ноль
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    
    // 3. Посылаем команду IDENTIFY (0xEC)
    outb(0x1F7, 0xEC);

    // 4. Проверяем, существует ли вообще диск (если статус 0, диска нет)
    uint8_t status = inb(0x1F7);
    if (status == 0) return; 

    // 5. Ждем, пока диск занят (BSY флаг) и пока он не будет готов отдать данные (DRQ флаг)
    while (inb(0x1F7) & 0x80); // Ждем пока BSY очистится
    while (!(inb(0x1F7) & 0x08)); // Ждем пока DRQ установится
}

// Команда DISK для шелла
void cmd_disk(int* terminal_row) {
    // Опрашиваем диск прямо перед выводом
    outb(0x1F6, 0xA0);
    outb(0x1F7, 0xEC);
    
    uint8_t status = inb(0x1F7);
    
    (*terminal_row)++;
    if (*terminal_row > 22) { clear_screen(); *terminal_row = 1; }
    
    print_string("--- KrexOS Hard Disk Drive Controller ---", *terminal_row, 0, 0x0B); // Голубой
    
    (*terminal_row)++;
    if (status == 0) {
        print_string("Status: No HDD detected in system!", *terminal_row, 0, 0x0C); // Красный
    } else {
        print_string("Status: [ONLINE] ATA IDE Drive detected!", *terminal_row, 0, 0x0A); // Зеленый
        
        // Читаем 256 слов (512 байт) буфера данных диска, чтобы очистить порт
        uint16_t disk_data[256];
        for (int i = 0; i < 256; i++) {
            disk_data[i] = inw(0x1F0);
        }
        
        // В секторе идентификации 11-20 слова хранят серийный номер, а 27-46 модель.
        // А в словах 60-61 лежит общий объем секторов (LBA28 capacity)
        uint32_t sectors = ((uint32_t)disk_data[61] << 16) | disk_data[60];
        uint32_t size_mb = (sectors * 512) / 1024 / 1024;
        
        (*terminal_row)++;
        print_string("Drive Size: ", *terminal_row, 0, 0x0F);
        
        // Перевод размера в строку на чистом Си без стандартных библиотек
        char size_str[16];
        int idx = 0;
        if (size_mb == 0) {
            size_str[idx++] = '0';
        } else {
            uint32_t temp = size_mb;
            char rev[16];
            int r_idx = 0;
            while (temp > 0) {
                rev[r_idx++] = (temp % 10) + '0';
                temp /= 10;
            }
            while (r_idx > 0) {
                size_str[idx++] = rev[--r_idx];
            }
        }
        size_str[idx++] = ' ';
        size_str[idx++] = 'M';
        size_str[idx++] = 'B';
        size_str[idx++] = '\0';
        
        print_string(size_str, *terminal_row, 12, 0x0E); // Желтый размер в MB!
    }
    (*terminal_row)++;
}