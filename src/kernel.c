// Чтение из порта клавиатуры
unsigned char inb(unsigned short port) {
    unsigned char result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Очистка экрана
void clear_screen(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
}

// Корректная функция вывода строки по координатам (строка, столбец)
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

// Вывод одиночного символа
void print_char(char c, int row, int col, char color) {
    volatile char* video_memory = (volatile char*)0xB8000;
    int offset = (row * 80 + col) * 2;
    video_memory[offset] = c;
    video_memory[offset + 1] = color;
}

// Сравнение строк
int str_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return (str1[i] == '\0' && str2[i] == '\0');
}

// Буферы данных пользователя
static char registered_user[32] = "";
static char registered_password[32] = "";

// Функция ввода строки (с фиксом затирания соседних символов)
void read_line(char* buffer, int max_len, int row, int start_col, int mask_input) {
    int index = 0;
    unsigned char last_scancode = 0;
    int current_col = start_col;

    print_char('_', row, current_col, 0x0A);

    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (scancode != last_scancode) {
                last_scancode = scancode;

                if (!(scancode & 0x80)) { // Key press
                    if (scancode == 0x1C) { // ENTER
                        print_char(' ', row, current_col, 0x07);
                        buffer[index] = '\0';
                        break;
                    } 
                    else if (scancode == 0x0E) { // BACKSPACE
                        if (index > 0) {
                            print_char(' ', row, current_col, 0x07);
                            current_col--;
                            print_char(' ', row, current_col, 0x07);
                            print_char('_', row, current_col, 0x0A);
                            index--;
                        }
                    } 
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
                        else if (scancode == 0x39) key = ' ';
                        else if (scancode == 0x02) key = '1';
                        else if (scancode == 0x03) key = '2';
                        else if (scancode == 0x04) key = '3';
                        else if (scancode == 0x05) key = '4';
                        else if (scancode == 0x06) key = '5';
                        else if (scancode == 0x07) key = '6';
                        else if (scancode == 0x08) key = '7';
                        else if (scancode == 0x09) key = '8';
                        else if (scancode == 0x0A) key = '9';
                        else if (scancode == 0x0B) key = '0';

                        if (key != 0 && index < (max_len - 1) && current_col < 79) {
                            buffer[index] = key;
                            index++;
                            print_char(mask_input ? '*' : key, row, current_col, 0x0F);
                            current_col++;
                            print_char('_', row, current_col, 0x0A);
                        }
                    }
                }
            }
        }
        asm volatile("nop");
    }
}

void kernel_main(void) {
    clear_screen();
    
    // --- ИСПРАВЛЕННЫЙ ВЫВОД ЗАСТАВКИ (ЧЕРЕЗ КОРРЕКТНЫЕ ФУНКЦИИ БЕЗ СДВИГОВ ПАМЯТИ) ---
    print_string("Welcome to KrexOS v0.0.1!", 10, 26, 0x0A); // Яркий зеленый
    print_string("Powered by Krexbow", 12, 29, 0x0F);       // Яркий белый
    
    // Вывод приглашения нажать ENTER точно по центру внизу заставки
    print_string("Press ENTER to start registration...", 16, 21, 0x0E); // Желтый

    // Ожидание нажатия ENTER перед регистрацией
    unsigned char last_scancode = 0;
    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (scancode != last_scancode) {
                last_scancode = scancode;
                if (scancode == 0x1C) { // Ждем ENTER (0x1C)
                    break;
                }
            }
        }
        asm volatile("nop");
    }

    // --- 1. ЭКРАН РЕГИСТРАЦИИ ---
    clear_screen();
    print_string("=== KrexOS ACCOUNT REGISTRATION ===", 2, 22, 0x0B);
    
    print_string("Create Username: ", 6, 5, 0x0F);
    read_line(registered_user, 32, 6, 22, 0);

    print_string("Create Password: ", 8, 5, 0x0F);
    read_line(registered_password, 32, 8, 22, 1);

    print_string("Registration successful! Press ENTER to Login...", 12, 5, 0x0A);
    
    last_scancode = 0;
    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (scancode != last_scancode) {
                last_scancode = scancode;
                if (scancode == 0x1C) break;
            }
        }
        asm volatile("nop");
    }

    // --- 2. ЭКРАН ВХОДА (LOGIN) ---
    char input_user[32];
    char input_password[32];

    while (1) {
        clear_screen();
        print_string("=== KrexOS SYSTEM LOGIN ===", 2, 26, 0x0B);

        print_string("Enter Username: ", 6, 5, 0x0F);
        read_line(input_user, 32, 6, 21, 0);

        print_string("Enter Password: ", 8, 5, 0x0F);
        read_line(input_password, 32, 8, 21, 1);

        if (str_compare(input_user, registered_user) && str_compare(input_password, registered_password)) {
            print_string("Access Granted! Loading Shell...", 11, 5, 0x0A);
            for(volatile long long d=0; d<20000000; d++);
            break;
        } else {
            print_string("INVALID CREDENTIALS! Press ENTER to try again.", 11, 5, 0x0C);
            last_scancode = 0;
            while (1) {
                if (inb(0x64) & 0x01) {
                    unsigned char scancode = inb(0x60);
                    if (scancode != last_scancode) {
                        last_scancode = scancode;
                        if (scancode == 0x1C) break;
                    }
                }
            }
        }
    }

    // --- 3. ИНТЕРАКТИВНЫЙ ШЕЛЛ ---
    clear_screen();
    print_string("KrexOS Kernel Interactive Shell [v0.0.1]", 0, 0, 0x0E); 
    print_string("Type 'HELP' for a list of commands.", 1, 0, 0x07);
    
    int user_len = 0;
    while (registered_user[user_len] != '\0') {
        user_len++;
    }

    int terminal_row = 3;
    print_string(registered_user, terminal_row, 0, 0x0A); 
    print_string("# ", terminal_row, user_len, 0x07);       
    
    int prefix_len = user_len + 2; 
    int cursor_col = prefix_len;       
    int cmd_index = 0;
    char command_buffer[64];
    last_scancode = 0;

    print_char('_', terminal_row, cursor_col, 0x0A);

    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60); 

            if (scancode != last_scancode) {
                last_scancode = scancode;

                if (!(scancode & 0x80)) { 
                    
                    if (scancode == 0x1C) { // ENTER В ШЕЛЛЕ
                        print_char(' ', terminal_row, cursor_col, 0x07); 
                        command_buffer[cmd_index] = '\0'; 

                        terminal_row++;
                        if (terminal_row > 23) { clear_screen(); terminal_row = 1; }

                        if (cmd_index > 0) {
                            if (str_compare(command_buffer, "HELP")) {
                                print_string("Available commands:", terminal_row, 0, 0x0E);
                                terminal_row++;
                                print_string("  HELP  - Show this list", terminal_row, 0, 0x0F);
                                terminal_row++;
                                print_string("  CLEAR - Clear screen", terminal_row, 0, 0x0F);
                                terminal_row++;
                                print_string("  WHOAMI- Show current logged user", terminal_row, 0, 0x0F);
                            } 
                            else if (str_compare(command_buffer, "CLEAR")) {
                                clear_screen(); 
                                terminal_row = 0;
                            } 
                            else if (str_compare(command_buffer, "WHOAMI")) {
                                print_string("Logged in as: ", terminal_row, 0, 0x0B);
                                print_string(registered_user, terminal_row, 14, 0x0F);
                            }
                            else {
                                print_string("KrexOS: command not found: ", terminal_row, 0, 0x0C);
                                print_string(command_buffer, terminal_row, 27, 0x0C);
                            }
                            terminal_row++;
                        }

                        if (terminal_row > 23) { clear_screen(); terminal_row = 1; }
                        
                        print_string(registered_user, terminal_row, 0, 0x0A);
                        print_string("# ", terminal_row, user_len, 0x07);
                        
                        cursor_col = prefix_len; 
                        cmd_index = 0;
                        print_char('_', terminal_row, cursor_col, 0x0A);
                    }
                    
                    else if (scancode == 0x0E) { // BACKSPACE В ШЕЛЛЕ
                        if (cursor_col > prefix_len) {
                            print_char(' ', terminal_row, cursor_col, 0x07); 
                            cursor_col--; 
                            print_char(' ', terminal_row, cursor_col, 0x07); 
                            print_char('_', terminal_row, cursor_col, 0x0A); 
                            cmd_index--; 
                        }
                    }
                    
                    else { // Ввод текста
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
                        else if (scancode == 0x39) key = ' ';

                        if (key != 0 && cursor_col < 79 && cmd_index < 63) {
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
        asm volatile("nop");
    }
}

