#include "libc.h"

// Говорим компилятору, что функция вывода текста живет в kernel.c
// Сигнатура (параметры) в точности как у тебя на скриншоте!
extern void print_string(const char* str, int row, int col, char color);

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

    // Так как цифры записались задом наперед (из 125 получили "521"),
    // переворачиваем строку в нормальный вид
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    // Вызываем твою функцию из kernel.c и выводим готовое число!
    print_string(str, row, col, color);
}