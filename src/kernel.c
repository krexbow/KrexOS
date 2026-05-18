unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void clear_screen(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
}

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

void print_char(char c, int row, int col, char color) {
    volatile char* video_memory = (volatile char*)0xB8000;
    int offset = (row * 80 + col) * 2;
    video_memory[offset] = c;
    video_memory[offset + 1] = color;
}

// Наша собственная функция сравнения строк (замена стандартной strcmp)
int str_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return 0; // Не совпадают
        }
        i++;
    }
    // Если обе строки закончились одновременно — они равны
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 1; 
    }
    return 0;
}

void kernel_main(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // --- 1. ЭКРАН ЗАГРУЗКИ ---
    int offset1 = 1120; char c1 = 0x0A;
    video_memory[offset1] = 'W'; video_memory[offset1 + 2] = 'e'; video_memory[offset1 + 4] = 'l'; video_memory[offset1 + 6] = 'c'; video_memory[offset1 + 8] = 'o'; video_memory[offset1 + 10] = 'm'; video_memory[offset1 + 12] = 'e'; video_memory[offset1 + 14] = ' '; video_memory[offset1 + 16] = 't'; video_memory[offset1 + 18] = 'o'; video_memory[offset1 + 20] = ' '; video_memory[offset1 + 22] = 'K'; video_memory[offset1 + 24] = 'r'; video_memory[offset1 + 25] = 0x0A; video_memory[offset1 + 26] = 'e'; video_memory[offset1 + 28] = 'x'; video_memory[offset1 + 30] = 'O'; video_memory[offset1 + 32] = 'S'; video_memory[offset1 + 34] = ' '; video_memory[offset1 + 36] = 'v'; video_memory[offset1 + 38] = '0'; video_memory[offset1 + 40] = '.'; video_memory[offset1 + 42] = '0'; video_memory[offset1 + 44] = '.'; video_memory[offset1 + 46] = '1'; video_memory[offset1 + 48] = '!';
    for(int i=0; i<50; i+=2) video_memory[offset1+i+1]=c1;
    
    int offset2 = 1280; char c2 = 0x0F;
    video_memory[offset2] = 'P'; video_memory[offset2 + 2] = 'o'; video_memory[offset2 + 4] = 'w'; video_memory[offset2 + 6] = 'e'; video_memory[offset2 + 8] = 'r'; video_memory[offset2 + 10] = 'e'; video_memory[offset2 + 12] = 'd'; video_memory[offset2 + 14] = ' '; video_memory[offset2 + 16] = 'b'; video_memory[offset2 + 17] = c2; video_memory[offset2 + 18] = 'y'; video_memory[offset2 + 20] = ' '; video_memory[offset2 + 22] = 'K'; video_memory[offset2 + 24] = 'r'; video_memory[offset2 + 26] = 'e'; video_memory[offset2 + 28] = 'x'; video_memory[offset2 + 30] = 'b'; video_memory[offset2 + 32] = 'o'; video_memory[offset2 + 34] = 'w';
    for(int i=0; i<36; i+=2) video_memory[offset2+i+1]=c2;

    // --- 2. ЗАДЕРЖКА ---
    volatile unsigned long long delay;
    for (delay = 0; delay < 3000000000ULL; delay++);

    // --- 3. ИНИЦИАЛИЗАЦИЯ ШЕЛЛА ---
    clear_screen();
    print_string("KrexOS Kernel Interactive Shell [v0.0.1]", 0, 0, 0x0E); 
    print_string("Type commands here. Driver: Polling Mode.", 1, 0, 0x07);   
    print_string("krexos# ", 3, 0, 0x0A); 

    int cursor_col = 8; 
    int terminal_row = 3;

    // Буфер для хранения вводимой команды (максимум 64 символа)
    char command_buffer[64];
    int cmd_index = 0;

    // --- 4. БЕСКОНЕЧНЫЙ ЦИКЛ ОПРОСА КЛАВИАТУРЫ ---
    unsigned char last_scancode = 0;

    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60); 

            if (scancode != last_scancode) {
                last_scancode = scancode;

                if (!(scancode & 0x80)) {
                    
                    // ОБРАБОТКА ENTER — ВРЕМЯ КУШАТЬ КОМАНДЫ!
                    if (scancode == 0x1C) {
                        print_char(' ', terminal_row, cursor_col, 0x07); // Трём старый курсор
                        
                        // Добавляем ноль-терминатор в конец буфера, чтобы получилась полноценная строка
                        command_buffer[cmd_index] = '\0'; 

                        terminal_row++; // Переходим на строку вывода результата
                        if (terminal_row > 22) { clear_screen(); terminal_row = 2; }

                        // ---- ПАРСЕР КОМАНД ----
                        if (cmd_index == 0) {
                            // Если ничего не ввели и нажали Enter — просто пропускаем строку
                        } 
                        else if (str_compare(command_buffer, "HELP")) {
                            print_string("Available commands:", terminal_row, 0, 0x0E);
                            terminal_row++;
                            print_string("  HELP  - Show this list", terminal_row, 0, 0x0F);
                            terminal_row++;
                            print_string("  CLEAR - Clear interactive shell screen", terminal_row, 0, 0x0F);
                            terminal_row++;
                            print_string("  ABOUT - Information about KrexOS", terminal_row, 0, 0x0F);
                        } 
                        else if (str_compare(command_buffer, "CLEAR")) {
                            clear_screen();
                            terminal_row = 1; // Сбрасываем строки наверх
                        } 
                        else if (str_compare(command_buffer, "ABOUT")) {
                            print_string("KrexOS v0.0.1. Created by Krexbow in 2026.", terminal_row, 0, 0x0B);
                        } 
                        else {
                            // Если ввели неизвестную дичь
                            print_string("KrexOS: command not found: ", terminal_row, 0, 0x0C); // Красный текст
                            print_string(command_buffer, terminal_row, 27, 0x0C);
                        }

                        // Готовим следующую строку ввода
                        terminal_row++;
                        if (terminal_row > 24) { clear_screen(); terminal_row = 2; }
                        
                        print_string("krexos# ", terminal_row, 0, 0x0A);
                        cursor_col = 8;
                        cmd_index = 0; // Обнуляем буфер для новой команды
                        print_char('_', terminal_row, cursor_col, 0x0A);
                    }
                    
                    // ОБРАБОТКА BACKSPACE
                    else if (scancode == 0x0E) {
                        if (cursor_col > 8) {
                            print_char(' ', terminal_row, cursor_col, 0x07); 
                            cursor_col--; 
                            print_char(' ', terminal_row, cursor_col, 0x07); 
                            print_char('_', terminal_row, cursor_col, 0x0A); 
                            
                            // Убираем символ из буфера команд
                            cmd_index--; 
                        }
                    }
                    
                    // ОБЫЧНЫЕ БУКВЫ
                    else {
                        char key = 0;
                        if (scancode == 0x1E) key = 'A';
                        else if (scancode == 0x30) key = 'B';
                        else if (scancode == 0x2E) key = 'C';
                        else if (scancode == 0x20) key = 'D';
                        else if (scancode == 0x12) key = 'E';
                        else if (scancode == 0x21) key = 'F';
                        else if (scancode == 0x22) key = 'G';
                        else if (scancode == 0x23) key = 'H';
                        else if (scancode == 0x17) key = 'I';
                        else if (scancode == 0x24) key = 'J';
                        else if (scancode == 0x25) key = 'K';
                        else if (scancode == 0x26) key = 'L';
                        else if (scancode == 0x32) key = 'M';
                        else if (scancode == 0x31) key = 'N';
                        else if (scancode == 0x18) key = 'O';
                        else if (scancode == 0x19) key = 'P';
                        else if (scancode == 0x10) key = 'Q';
                        else if (scancode == 0x13) key = 'R';
                        else if (scancode == 0x1F) key = 'S';
                        else if (scancode == 0x14) key = 'T';
                        else if (scancode == 0x16) key = 'U';
                        else if (scancode == 0x2F) key = 'V';
                        else if (scancode == 0x11) key = 'W';
                        else if (scancode == 0x2D) key = 'X';
                        else if (scancode == 0x15) key = 'Y';
                        else if (scancode == 0x2C) key = 'Z';
                        else if (scancode == 0x39) key = ' '; // Пробел

                        // Если букву распознали и в буфере еще есть место (меньше 63 символов)
                        if (key != 0 && cursor_col < 79 && cmd_index < 63) {
                            
                            // Сохраняем букву в наш буфер
                            command_buffer[cmd_index] = key;
                            cmd_index++;

                            print_char(key, terminal_row, cursor_col, 0x0F); 
                            cursor_col++;
                            print_char('_', terminal_row, cursor_col, 0x0A); 
                        }
                    }
                }
            }
        }
        for (volatile int i = 0; i < 1000; i++);
    }
}

