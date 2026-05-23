#include "ata.h"
#include "fs.h"
#include "idt.h" 
#include "libc.h" // Подключаем нашу новую библиотеку

// --- НАСТРОЙКА ПОРТОВ PIC КОНТРОЛЛЕРА ---
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// Функция ремаппинга PIC, чтобы прерывания клавиатуры шли в IDT
void pic_remap(void) {
    // Инициализация ведущего и ведомого PIC
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    
    // Сдвигаем базовые векторы прерываний: IRQ 0-7 в 0x20-0x27, IRQ 8-15 в 0x28-0x2F
    outb(PIC1_DATA, 0x20); 
    outb(PIC2_DATA, 0x28);
    
    // Каскадирование PIC1 и PIC2
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    
    // Режим 8086
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    // Разрешаем только IRQ1 (клавиатура), остальные прерывания маскируем для стабильности
    outb(PIC1_DATA, 0xFD); // 0xFD = 11111101b
    outb(PIC2_DATA, 0xFF); // Маскируем всё на ведомом контроллере
}

// --- СИСТЕМНЫЕ ФУНКЦИИ И РАБОТА С ЭКРАНОМ ---
void clear_screen(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
}

// Функция плавного сдвига экрана вверх при переполнении строк
void scroll(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // Копируем строки со 2-й по 25-ю (индексы 1-24) на одну строку выше (индексы 0-23)
    for (int i = 0; i < 24 * 160; i++) {
        video_memory[i] = video_memory[i + 160];
    }

    // Затираем самую нижнюю (25-ю) строку пробелами, чтобы она была чистой
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

int str_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' || str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return 1;
}

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

// --- СТАТИЧЕСКАЯ ПАМЯТЬ ЯДРА ---
static char registered_user[32] = "";
static char registered_password[32] = "";
static char input_user[32];
static char input_password[32];
static char command_buffer[64];

static char cmd[32];
static char arg1[32];
static char arg2[64];

// Буфер клавиатуры для прерываний
static volatile char key_pressed = 0; 
static volatile int enter_triggered = 0;

void fs_print_wrapper(const char* str, int* row) {
    if (*row >= 25) { 
        scroll(); 
        *row = 24; 
    }
    print_string(str, *row, 0, 0x0F);
    (*row)++;
}

// --- ФУНКЦИЯ ОБРАБОТКИ ПРЕРЫВАНИЯ КЛАВИАТУРЫ (Вызывается из ASM) ---
void keyboard_handler_main(void) {
    uint8_t status = inb(0x64);
    
    if (status & 0x01) {
        uint8_t scancode = inb(0x60);
        
        if (!(scancode & 0x80)) { // Скан-код нажатия
            if (scancode == 0x1C) { // ENTER
                enter_triggered = 1;
                key_pressed = '\n';
            } 
            else if (scancode == 0x0E) { // BACKSPACE
                key_pressed = '\b';
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

                if (key != 0) {
                    key_pressed = key;
                }
            }
        }
    }
    // Отправляем сигнал EOI (End of Interrupt) контроллеру прерываний
    outb(0x20, 0x20);
}

// Функция ожидания нажатия ENTER (для экранов заставок)
void wait_for_enter(void) {
    enter_triggered = 0;
    key_pressed = 0; // Чистим старый ввод на входе
    while (!enter_triggered) {
        asm volatile("hlt"); // Останавливает процессор до следующего прерывания
    }
    enter_triggered = 0;
    key_pressed = 0; // Намертво гасим ENTER, чтобы он не пролетал в read_line
}

// --- СОВРЕМЕННАЯ ФУНКЦИЯ ПОСТРОЧНОГО ВВОДА НА ПРЕРЫВАНИЯХ ---
void read_line(char* buffer, int max_len, int row, int start_col, int mask_input) {
    int index = 0;
    int current_col = start_col;
    print_char('_', row, current_col, 0x0A);

    while (1) {
        asm volatile("hlt"); // Спим и ждем прерывания от клавиатуры
        
        if (key_pressed != 0) {
            char c = key_pressed;
            key_pressed = 0; // Сбрасываем символ, так как мы его забрали

            if (c == '\n') { // ENTER
                print_char(' ', row, current_col, 0x07);
                buffer[index] = '\0';
                break;
            } 
            else if (c == '\b') { // BACKSPACE
                if (index > 0) {
                    print_char(' ', row, current_col, 0x07);
                    current_col--;
                    print_char(' ', row, current_col, 0x07);
                    print_char('_', row, current_col, 0x0A);
                    index--;
                }
            } 
            else { // Набранный символ
                if (index < (max_len - 1) && current_col < 79) {
                    buffer[index] = c;
                    index++;
                    print_char(mask_input ? '*' : c, row, current_col, 0x0F);
                    current_col++;
                    print_char('_', row, current_col, 0x0A);
                }
            }
        }
    }
}

// --- MAIN СИ-ЯДРА ---
void kernel_main(void) {
    // НАДЁЖНАЯ ЗАЩИТА И НАСТРОЙКА АППАРАТНОЙ ЧАСТИ
    asm volatile("cli"); // Выключаем прерывания перед манипуляциями с таблицами
    
    clear_screen();
    init_idt();          // Инициализируем IDT структуру
    pic_remap();         // ПЕРЕМАППИМ PIC (теперь прерывания пойдут куда нужно)
    
    asm volatile("sti"); // Включаем прерывания обратно. Процессор готов слушать клавиатуру!

    // Находим подключенный жесткий диск
    ata_identify();
    
    // Инициализируем/Размечаем файловую систему на жестком диске
    fs_init();
    
    // ЭКРАН 1: ЗАСТАВКА
    print_string("Welcome to KrexOS v0.0.1!", 10, 26, 0x0A); 
    print_string("Powered by Krexbow", 12, 29, 0x0F);       
    print_string("Press ENTER to start registration...", 16, 21, 0x0E); 

    wait_for_enter(); // Здесь прерывание от ENTER сработает штатно и разбудит ядро

    // ЭКРАН 2: РЕГИСТРАЦИЯ
    clear_screen();
    print_string("=== KrexOS ACCOUNT REGISTRATION ===", 2, 22, 0x0B);
    print_string("Create Username: ", 6, 5, 0x0F);
    read_line(registered_user, 32, 6, 22, 0);
    print_string("Create Password: ", 8, 5, 0x0F);
    read_line(registered_password, 32, 8, 22, 1);
    print_string("Registration successful! Press ENTER to Login...", 12, 5, 0x0A);
    
    wait_for_enter();

    // ЭКРАН 3: ЛОГИН
    while (1) {
        clear_screen();
        print_string("=== KrexOS SYSTEM LOGIN ===", 2, 26, 0x0B);
        print_string("Enter Username: ", 6, 5, 0x0F);
        read_line(input_user, 32, 6, 21, 0);
        print_string("Enter Password: ", 8, 5, 0x0F);
        read_line(input_password, 32, 8, 21, 1);

        if (str_compare(input_user, registered_user) && str_compare(input_password, registered_password)) {
            print_string("Access Granted! Loading Shell...", 11, 5, 0x0A);
            for(volatile long long d = 0; d < 20000000; d++);
            break;
        } else {
            print_string("INVALID CREDENTIALS! Press ENTER to try again.", 11, 5, 0x0C);
            wait_for_enter();
        }
    }

    // ЭКРАН 4: ИНТЕРАКТИВНЫЙ ШЕЛЛ
    clear_screen();
    print_string("KrexOS Kernel Interactive Shell [v0.0.1]", 0, 0, 0x0E); 
    print_string("Type 'HELP' for a list of commands.", 1, 0, 0x07);
    
    int user_len = 0;
    while (registered_user[user_len] != '\0') user_len++;

    int terminal_row = 3;
    print_string(registered_user, terminal_row, 0, 0x0A); 
    print_string("# ", terminal_row, user_len, 0x07);      
    
    int prefix_len = user_len + 2; 
    int cursor_col = prefix_len;       
    int cmd_index = 0;

    print_char('_', terminal_row, cursor_col, 0x0A);

    while (1) {
        asm volatile("hlt"); // Шелл просто "спит" и ждет пока сработает прерывание клавиатуры
        
        if (key_pressed != 0) {
            char c = key_pressed;
            key_pressed = 0;

            if (c == '\n') { // ENTER
                print_char(' ', terminal_row, cursor_col, 0x07); 
                command_buffer[cmd_index] = '\0'; 

                terminal_row++;
                if (terminal_row >= 25) { 
                    scroll(); 
                    terminal_row = 24; 
                }

                if (cmd_index > 0) {
                    for(int i = 0; i < 32; i++) { cmd[i] = 0; arg1[i] = 0; }
                    for(int i = 0; i < 64; i++) { arg2[i] = 0; }
                    
                    split_command(command_buffer, cmd, arg1, arg2);

                    if (str_compare(cmd, "HELP")) {
                        print_string("Available commands:", terminal_row++, 0, 0x0E);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }
                        
                        print_string("  HELP                 - Show this list", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }
                        
                        print_string("  CLEAR                - Clear screen", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }
                        
                        print_string("  WHOAMI               - Show current logged user", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  DISK                 - Show connected hardware drive info", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  CREATE [name] [text] - Create file with text on HDD", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  CAT [name]           - Display file content", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  DEL [name]           - Delete file from HDD", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  LS                   - List files on drive C", terminal_row, 0, 0x0F);
                    } 
                    else if (str_compare(cmd, "CLEAR")) {
                        clear_screen(); 
                        terminal_row = 0;
                    } 
                    else if (str_compare(cmd, "WHOAMI")) {
                        print_string("Logged in as: ", terminal_row, 0, 0x0B);
                        print_string(registered_user, terminal_row, 14, 0x0F);
                    }
                    else if (str_compare(cmd, "DISK")) {
                        cmd_disk(&terminal_row);
                        terminal_row--; // Корректируем индекс после вывода функции
                    }
                    else if (str_compare(cmd, "CREATE")) {
                        if (arg1[0] == '\0' || arg2[0] == '\0') {
                            print_string("Usage: CREATE [filename] [text]", terminal_row, 0, 0x0C);
                        } else {
                            if (fs_create_file(arg1, arg2)) {
                                print_string("Success: File created!", terminal_row, 0, 0x0A);
                            } else {
                                print_string("Error: No space or file already exists.", terminal_row, 0, 0x0C);
                            }
                        }
                    }
                    else if (str_compare(cmd, "CAT")) {
                        if (arg1[0] == '\0') {
                            print_string("Usage: CAT [filename]", terminal_row, 0, 0x0C);
                        } else {
                            static char file_view_buf[4096];
                            if (fs_read_file(arg1, file_view_buf)) {
                                print_string(file_view_buf, terminal_row, 0, 0x0F);
                            } else {
                                print_string("Error: File not found.", terminal_row, 0, 0x0C);
                            }
                        }
                    }
                    else if (str_compare(cmd, "DEL")) {
                        if (arg1[0] == '\0') {
                            print_string("Usage: DEL [filename]", terminal_row, 0, 0x0C);
                        } else {
                            if (fs_delete_file(arg1)) {
                                print_string("Success: File deleted.", terminal_row, 0, 0x0A);
                            } else {
                                print_string("Error: File not found.", terminal_row, 0, 0x0C);
                            }
                        }
                    }
                    else if (str_compare(cmd, "LS")) {
                        fs_list_files_internal(&terminal_row);
                        terminal_row--; 
                    }
                    else {
                        print_string("Unknown command: ", terminal_row, 0, 0x0C);
                        print_string(command_buffer, terminal_row, 17, 0x0C);
                    }
                    terminal_row++;
                }
                
                if (terminal_row >= 25) { 
                    scroll(); 
                    terminal_row = 24; 
                }
                print_string(registered_user, terminal_row, 0, 0x0A);
                print_string("# ", terminal_row, user_len, 0x07);
                cursor_col = prefix_len; 
                cmd_index = 0;
                print_char('_', terminal_row, cursor_col, 0x0A);
            }
            else if (c == '\b') { // BACKSPACE в шелле
                if (cursor_col > prefix_len) {
                    print_char(' ', terminal_row, cursor_col, 0x07); 
                    cursor_col--; 
                    print_char(' ', terminal_row, cursor_col, 0x07); 
                    print_char('_', terminal_row, cursor_col, 0x0A); 
                    cmd_index--; 
                }
            }
            else { // Набор обычных symbols
                if (cursor_col < 79 && cmd_index < 63) {
                    command_buffer[cmd_index] = c;
                    cmd_index++;
                    print_char(c, terminal_row, cursor_col, 0x0F); 
                    cursor_col++;
                    print_char('_', terminal_row, cursor_col, 0x0A); 
                }
            }
        }
    }
}