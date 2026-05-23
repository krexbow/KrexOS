#ifndef LIBC_H
#define LIBC_H

// --- СТАНДАРТНЫЕ ТИПЫ ДАННЫХ ДЛЯ НАШЕЙ ОС ---
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

// --- ФУНКЦИИ ВВОДА-ВЫВОДА В ПОРТЫ (нужны для драйверов) ---
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Добавили word-функции (16 бит) из ata.h, переписав типы под твои uint16_t
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// --- ФУНКЦИИ ВВОДА-ВЫВОДА И СТРОК (Библиотека) ---
void print_int(int num, int row, int col, char color);
int str_compare(const char* str1, const char* str2);
uint32_t parse_int(const char* str);
void split_command(const char* input, char* cmd, char* arg1, char* arg2);

#endif