#include "ata.h"

// Глобальная переменная для хранения размера диска в мегабайтах
static uint32_t drive_size_mb = 0;
static int drive_online = 0;

// Инициализация и определение жесткого диска
void ata_identify() {
    // Выбираем Master-диск на Primary канале (0xA0)
    outb(0x1F6, 0xA0);
    
    // Сбрасываем порты секторов
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
    uint16_t info[256];
    for (int i = 0; i < 256; i++) {
        info[i] = inw(0x1F0);
    }
    
    // Количество LBA26/LBA28 секторов лежит в словах 60 и 61
    uint32_t sectors = info[60] | (((uint32_t)info[61]) << 16);
    
    // Считаем размер в Мегабайтах: (секторы * 512) / 1024 / 1024
    drive_size_mb = (sectors * 512) / (1024 * 1024);
    drive_online = 1;
}

// Функция-обработчик команды DISK в шелле
void cmd_disk(int* terminal_row) {
    (*terminal_row)++;
    
    if (drive_online) {
        // Выводим статус диска
        volatile char* video_memory = (volatile char*)0xB8000;
        
        // Пишем "Drive Status: "
        int offset = ((*terminal_row) * 80 + 0) * 2;
        const char* msg1 = "Drive Status: ";
        for(int i=0; msg1[i] != '\0'; i++) {
            video_memory[offset] = msg1[i]; video_memory[offset+1] = 0x0F; offset+=2;
        }
        // Пишем "ONLINE" зеленым
        const char* msg2 = "ONLINE";
        for(int i=0; msg2[i] != '\0'; i++) {
            video_memory[offset] = msg2[i]; video_memory[offset+1] = 0x0A; offset+=2;
        }
        
        (*terminal_row)++;
        // Пишем "Drive Size: "
        offset = ((*terminal_row) * 80 + 0) * 2;
        const char* msg3 = "Drive Size:   ";
        for(int i=0; msg3[i] != '\0'; i++) {
            video_memory[offset] = msg3[i]; video_memory[offset+1] = 0x0F; offset+=2;
        }
        
        // Выводим размер диска (для простоты хардкодим вывод числа, зная что у нас 64MB)
        // Если размер другой, можно будет дописать функцию перевода int в string
        if (drive_size_mb == 64) {
            const char* msg_size = "64 MB";
            for(int i=0; msg_size[i] != '\0'; i++) {
                video_memory[offset] = msg_size[i]; video_memory[offset+1] = 0x0E; offset+=2;
            }
        } else {
            const char* msg_unk = "DETECTED";
            for(int i=0; msg_unk[i] != '\0'; i++) {
                video_memory[offset] = msg_unk[i]; video_memory[offset+1] = 0x0E; offset+=2;
            }
        }
    } else {
        // Если диска нет
        volatile char* video_memory = (volatile char*)0xB8000;
        int offset = ((*terminal_row) * 80 + 0) * 2;
        const char* msg_offline = "Drive Status: OFFLINE / NOT DETECTED";
        for(int i=0; msg_offline[i] != '\0'; i++) {
            video_memory[offset] = msg_offline[i]; video_memory[offset+1] = 0x0C; offset+=2;
        }
    }
}

// Наша новая функция чтения сектора (LBA28)
void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    while (inb(0x1F7) & 0x80);

    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    outb(0x1F7, 0x20);

    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
}

// Функция записи одного LBA сектора (512 байт) из буфера memory на диск
void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    // 1. Ждем, пока диск освободится
    while (inb(0x1F7) & 0x80);

    // 2. Выставляем параметры LBA28
    outb(0x1F2, 1);                      // Записываем ровно 1 сектор
    outb(0x1F3, (uint8_t)lba);           // LBA биты 0-7
    outb(0x1F4, (uint8_t)(lba >> 8));    // LBA биты 8-15
    outb(0x1F5, (uint8_t)(lba >> 16));   // LBA биты 16-23
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // 3. Посылаем команду ЗАПИСИ (0x30)
    outb(0x1F7, 0x30);

    // 4. Ждем, пока диск поднимет флаг DRQ (Data Request), показывая, что он готов принимать байты
    while (inb(0x1F7) & 0x80);        // Ждем BSY = 0
    while (!(inb(0x1F7) & 0x08));     // Ждем DRQ = 1

    // 5. Выплескиваем 256 слов (512 байт) из нашего буфера прямо в порт данных 0x1F0
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);          // outw шлет 16 бит (слово) в порт
    }

    // 6. Снова ждем окончания операции самим диском
    while (inb(0x1F7) & 0x80);
}
