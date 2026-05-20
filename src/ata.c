#include "ata.h"

// Глобальная переменная для хранения размера диска в мегабайтах
static uint32_t drive_size_mb = 0;
static int drive_online = 0;

// Инициализация и определение жесткого диска
void ata_identify() {
    // ИСПРАВЛЕНИЕ: Выбираем Master-диск на Primary канале с поддержкой LBA (0xE0 вместо 0xA0)
    outb(0x1F6, 0xE0);
    
    // Сброс портов секторов
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    
    // Посылаем команду IDENTIFY (0xEC)
    outb(0x1F7, 0xEC);
    
    // Читаем статус
    uint8_t status = inb(0x1F7);
    if (status == 0) {
        drive_online = 0; // Диска нет
        return;
    }
    
    // Ждем, пока очистится флаг BUSY (0x80) и поднимется DRQ (0x08)
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));
    
    // Читаем 256 слов данных (информация о диске)
    uint16_t id_data[256];
    for (int i = 0; i < 256; i++) {
        id_data[i] = inw(0x1F0);
    }
    
    // Рассчитываем количество секторов (слова 60-61 содержат 32-битное число общего количества LBA секторов)
    uint32_t sectors = *((uint32_t*)&id_data[60]);
    drive_size_mb = (sectors * 512) / (1024 * 1024);
    drive_online = 1;
}

// Функция вывода инфо о диске (если понадобится в шелле)
void cmd_disk(int* terminal_row) {
    // Вспомогательная функция вывода строк (дублирует логику ядра)
    void print_string(const char* str, int row, int col, char color);
    
    if (!drive_online) {
        print_string("HDD Status: OFFLINE / NOT FOUND", (*terminal_row)++, 0, 0x0C);
        return;
    }
    
    print_string("HDD Status: ONLINE (Primary Master)", (*terminal_row)++, 0, 0x0A);
    print_string("Addressing: LBA28 PIO Mode", (*terminal_row)++, 0, 0x0F);
    print_string("Capacity  : Connected HDD detected", (*terminal_row)++, 0, 0x0F);
}

// Функция чтения одного LBA сектора (512 байт) в буфер памяти
void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    // 1. Ждем освобождения диска от предыдущих команд
    while (inb(0x1F7) & 0x80);

    // 2. Выставляем параметры LBA28 в порты контроллера
    outb(0x1F2, 1);                      // Читаем ровно 1 сектор
    outb(0x1F3, (uint8_t)lba);           // LBA биты 0-7
    outb(0x1F4, (uint8_t)(lba >> 8));    // LBA биты 8-15
    outb(0x1F5, (uint8_t)(lba >> 16));   // LBA биты 16-23
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // Включаем режим LBA и передаем биты 24-27

    // 3. Посылаем команду на чтение секторов (0x20)
    outb(0x1F7, 0x20);

    // 4. Ждем завершения операции (очистка BUSY и выставление DRQ)
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    // 5. Вычитываем 256 слов (512 байт) из порта данных в буфер
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
}

// Функция записи одного LBA сектора (512 байт) из буфера memory на диск
void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    // ИСПРАВЛЕНИЕ/ЗАЩИТА: Не позволяем перезаписывать секторы 0-39, где лежит загрузчик и код самого ядра KrexOS
    if (lba < 40) {
        return; 
    }

    // 1. Ждем, пока диск освободится
    while (inb(0x1F7) & 0x80);

    // 2. Выставляем параметры LBA28
    outb(0x1F2, 1);                      // Записываем ровно 1 сектор
    outb(0x1F3, (uint8_t)lba);           // LBA биты 0-7
    outb(0x1F4, (uint8_t)(lba >> 8));    // LBA биты 8-15
    outb(0x1F5, (uint8_t)(lba >> 16));   // LBA биты 16-23
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // Включаем режим LBA и передаем биты 24-27

    // 3. Посылаем команду на запись секторов (0x30)
    outb(0x1F7, 0x30);

    // 4. Ждем, когда диск будет готов принять данные (очистка BUSY, поднятие DRQ)
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    // 5. Записываем 256 слов (512 байт) из буфера в порт данных контроллера
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);
    }
}