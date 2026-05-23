#include "screen.h"

void clear_screen(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
}

void scroll(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // Копируем строки со 2-й по 25-ю на одну строку выше
    for (int i = 0; i < 24 * 160; i++) {
        video_memory[i] = video_memory[i + 160];
    }

    // Затираем самую нижнюю строку пробелами
    for (int i = 24 * 160; i < 25 * 160; i += 2) {
        video_memory[i] = ' ';     
        video_memory[i + 1] = 0x07; 
    }
}

void print_string(const char* str, int row, int col, char color) {
    if (row < 0 || row >= 25 || col < 0 || col >= 80) return;
    volatile char* video_memory = (volatile char*)0xB8000;
    int offset = (row * 80 + col) * 2;
    int i = 0;
    while (str[i] != '\0') {
        if ((col + i) >= 80) break; 
        video_memory[offset] = str[i];
        video_memory[offset + 1] = color;
        offset += 2;
        i++;
    }
}

void print_char(char c, int row, int col, char color) {
    if (row < 0 || row >= 25 || col < 0 || col >= 80) return;
    volatile char* video_memory = (volatile char*)0xB8000;
    int offset = (row * 80 + col) * 2;
    video_memory[offset] = c;
    video_memory[offset + 1] = color;
}