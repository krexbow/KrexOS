#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Низкоуровневые функции портов
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Функции диска
void ata_identify();
void cmd_disk(int* terminal_row);
void ata_read_sector(uint32_t lba, uint8_t* buffer); // Наша новая функция!
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif

