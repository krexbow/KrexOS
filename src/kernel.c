// Функция очистки всего экрана (заполняет всё пробелами)
void clear_screen(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    // На экране 80x25 = 2000 символов. Каждый символ — 2 байта. Всего 4000 байт.
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2] = ' ';       // Символ пробела
        video_memory[i * 2 + 1] = 0x07;   // Серый цвет на черном фоне (стандартный)
    }
}

// Функция для вывода текста в терминальном стиле (в самый верх экрана)
void print_string(const char* str, int row, int col, char color) {
    volatile char* video_memory = (volatile char*)0xB8000;
    int offset = (row * 80 + col) * 2;
    
    int i = 0;
    while (str[i] != '\0') {
        video_memory[offset] = str[i];
        video_memory[offset + 1] = color;
        offset += 2;
        i++;
    }
}

void kernel_main(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // --- 1. ТВОЙ КАСТОМНЫЙ ЭКРАН ЗАГРУЗКИ ---
    // (Код, который затирает надпись "Booting from Floppy...")
    int offset1 = 1120; 
    char c1 = 0x0A; // Ярко-зелёный

    video_memory[offset1]      = 'W';  video_memory[offset1 + 1]  = c1;
    video_memory[offset1 + 2]  = 'e';  video_memory[offset1 + 3]  = c1;
    video_memory[offset1 + 4]  = 'l';  video_memory[offset1 + 5]  = c1;
    video_memory[offset1 + 6]  = 'c';  video_memory[offset1 + 7]  = c1;
    video_memory[offset1 + 8]  = 'o';  video_memory[offset1 + 9]  = c1;
    video_memory[offset1 + 10] = 'm';  video_memory[offset1 + 11] = c1;
    video_memory[offset1 + 12] = 'e';  video_memory[offset1 + 13] = c1;
    video_memory[offset1 + 14] = ' ';  video_memory[offset1 + 15] = c1;
    video_memory[offset1 + 16] = 't';  video_memory[offset1 + 17] = c1;
    video_memory[offset1 + 18] = 'o';  video_memory[offset1 + 19] = c1;
    video_memory[offset1 + 20] = ' ';  video_memory[offset1 + 21] = c1;
    video_memory[offset1 + 22] = 'K';  video_memory[offset1 + 23] = c1;
    video_memory[offset1 + 24] = 'r';  video_memory[offset1 + 25] = c1;
    video_memory[offset1 + 26] = 'e';  video_memory[offset1 + 27] = c1;
    video_memory[offset1 + 28] = 'x';  video_memory[offset1 + 29] = c1;
    video_memory[offset1 + 30] = 'O';  video_memory[offset1 + 31] = c1;
    video_memory[offset1 + 32] = 'S';  video_memory[offset1 + 33] = c1;
    video_memory[offset1 + 34] = ' ';  video_memory[offset1 + 35] = c1;
    video_memory[offset1 + 36] = 'v';  video_memory[offset1 + 37] = c1;
    video_memory[offset1 + 38] = '0';  video_memory[offset1 + 39] = c1;
    video_memory[offset1 + 40] = '.';  video_memory[offset1 + 41] = c1;
    video_memory[offset1 + 42] = '0';  video_memory[offset1 + 43] = c1;
    video_memory[offset1 + 44] = '.';  video_memory[offset1 + 45] = c1;
    video_memory[offset1 + 46] = '1';  video_memory[offset1 + 47] = c1;
    video_memory[offset1 + 48] = '!';  video_memory[offset1 + 49] = c1;

    int offset2 = 1280; 
    char c2 = 0x0F; // Ярко-белый

    video_memory[offset2]      = 'P';  video_memory[offset2 + 1]  = c2;
    video_memory[offset2 + 2]  = 'o';  video_memory[offset2 + 3]  = c2;
    video_memory[offset2 + 4]  = 'w';  video_memory[offset2 + 5]  = c2;
    video_memory[offset2 + 6]  = 'e';  video_memory[offset2 + 7]  = c2;
    video_memory[offset2 + 8]  = 'r';  video_memory[offset2 + 9]  = c2;
    video_memory[offset2 + 10] = 'e';  video_memory[offset2 + 11] = c2;
    video_memory[offset2 + 12] = 'd';  video_memory[offset2 + 13] = c2;
    video_memory[offset2 + 14] = ' ';  video_memory[offset2 + 15] = c2;
    video_memory[offset2 + 16] = 'b';  video_memory[offset2 + 17] = c2;
    video_memory[offset2 + 18] = 'y';  video_memory[offset2 + 19] = c2;
    video_memory[offset2 + 20] = ' ';  video_memory[offset2 + 21] = c2;
    video_memory[offset2 + 22] = 'K';  video_memory[offset2 + 23] = c2;
    video_memory[offset2 + 24] = 'r';  video_memory[offset2 + 25] = c2;
    video_memory[offset2 + 26] = 'e';  video_memory[offset2 + 27] = c2;
    video_memory[offset2 + 28] = 'x';  video_memory[offset2 + 29] = c2;
    video_memory[offset2 + 30] = 'b';  video_memory[offset2 + 31] = c2;
    video_memory[offset2 + 32] = 'o';  video_memory[offset2 + 33] = c2;
    video_memory[offset2 + 34] = 'w';  video_memory[offset2 + 35] = c2;


    // --- 2. ЗАДЕРЖКА ~5 СЕКУНД ---
    // volatile не дает компилятору выкинуть пустой цикл при оптимизации.
    // Если задержка покажется слишком быстрой или долгой, просто измени конечное число.
    volatile unsigned long long delay;
    for (delay = 0; delay < 3000000000ULL; delay++) {
        // Процессор просто считает до трех миллиардов
    }


    // --- 3. ПЕРЕХОД В ТЕРМИНАЛ ---
    clear_screen(); // Стираем к чертям приветствие загрузки

    // Выводим шапку нашей консоли на строке 0
    print_string("KrexOS Kernel Interactive Shell [v0.0.1]", 0, 0, 0x0E); // Жёлтый текст
    print_string("Type 'help' for a list of commands.", 1, 0, 0x07);        // Серый текст
    
    // Приглашение к вводу на строке 3
    print_string("krexos# _", 3, 0, 0x0A); // Зелёное приглашение с курсором

    while (1) {
        // Мертвая петля. Экран застынет в режиме терминала.
    }
}

