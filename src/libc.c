#include "libc.h"
#include "screen.h" // Подключаем экран, чтобы print_int могла использовать print_string

// --- ВЫВОД ЦЕЛЫХ ЧИСЕЛ НА ЭКРАН ---
void print_int(int num, int row, int col, char color) {
    char str[12]; // Буфер для символов (хватит для любого 32-битного числа)
    int i = 0;

    // Если нужно вывести чистый ноль
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        print_string(str, row, col, color);
        return;
    }

    // Разбираем число на цифры с конца (через остаток от деления на 10)
    while (num > 0) {
        int remainder = num % 10;
        str[i++] = remainder + '0'; // Переводим цифру в символ ASCII
        num = num / 10;
    }
    str[i] = '\0'; // Обязательно закрываем строку символом конца строки

    // Так как цифры записались задом наперед, переворачиваем строку
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    // Выводим готовое число через стандартный модуль экрана
    print_string(str, row, col, color);
}

// --- СРАВНЕНИЕ СТРОК ---
int str_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' || str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return 1;
}

// --- ПАРСИНГ ЧИСЛА ИЗ СТРОКИ ---
uint32_t parse_int(const char* str) {
    uint32_t res = 0;
    for (int i = 0; str[i] != '\0' && i < 10; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    return res;
}

// --- ДЕЛЕНИЕ СТРОКИ НА КОМАНДУ И АРГУМЕНТЫ ---
void split_command(const char* input, char* cmd, char* arg1, char* arg2) {
    int i = 0, j = 0;
    while (input[i] != '\0' && input[i] != ' ' && j < 31) {
        cmd[j++] = input[i++];
    }
    cmd[j] = '\0';
    while (input[i] == ' ') i++;
    j = 0;
    while (input[i] != '\0' && input[i] != ' ' && j < 31) {
        arg1[j++] = input[i++];
    }
    arg1[j] = '\0';
    while (input[i] == ' ') i++;
    j = 0;
    while (input[i] != '\0' && j < 63) {
        arg2[j++] = input[i++];
    }
    arg2[j] = '\0';
}