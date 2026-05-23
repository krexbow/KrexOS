#include "shell.h"
#include "screen.h"
#include "keyboard.h"
#include "libc.h"
#include "fs.h"
#include "ata.h"

// Статические буферы, нужные исключительно для парсинга и ввода команд
static char command_buffer[64];
static char cmd[32];
static char arg1[32];
static char arg2[64];

void fs_print_wrapper(const char* str, int* row) {
    if (*row >= 25) { 
        scroll(); 
        *row = 24; 
    }
    print_string(str, *row, 0, 0x0F);
    (*row)++;
}

void shell_start(const char* username) {
    clear_screen();
    print_string("KrexOS Kernel Interactive Shell [v0.0.1]", 0, 0, 0x0E); 
    print_string("Type 'HELP' for a list of commands.", 1, 0, 0x07);
    
    int user_len = 0;
    while (username[user_len] != '\0') user_len++;

    int terminal_row = 3;
    print_string(username, terminal_row, 0, 0x0A); 
    print_string("# ", terminal_row, user_len, 0x07);      
    
    int prefix_len = user_len + 2; 
    int cursor_col = prefix_len;       
    int cmd_index = 0;

    print_char('_', terminal_row, cursor_col, 0x0A);

    while (1) {
        asm volatile("hlt"); // Спим и ждем прерываний клавиатуры
        
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

                        print_string("  CREATE [name]        - Create file with multi-line text", terminal_row++, 0, 0x0F);
                        if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                        print_string("  EDIT [name]          - Edit existing or new file", terminal_row++, 0, 0x0F);
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
                        print_string(username, terminal_row, 14, 0x0F);
                    }
                    else if (str_compare(cmd, "DISK")) {
                        cmd_disk(&terminal_row);
                        terminal_row--; 
                    }
                    else if (str_compare(cmd, "CREATE")) {
                        if (arg1[0] == '\0') {
                            print_string("Usage: CREATE [filename]", terminal_row, 0, 0x0C);
                        } else {
                            print_string("Mode: Input text. Type 'END' on a new line to save.", terminal_row++, 0, 0x0E);
                            if (terminal_row >= 25) { scroll(); terminal_row = 24; }

                            // Буфер в безопасной RAM далеко за пределами ядра
                            char* big_buffer = (char*)0x50000; 
                            int big_idx = 0;
                            int writing = 1;

                            while (writing) {
                                print_string("> ", terminal_row, 0, 0x0A);
                                int input_col = 2;
                                int line_idx = 0;
                                char local_line[128];

                                print_char('_', terminal_row, input_col, 0x0A);

                                // Захватываем клавиатуру для чтения одной строки текста
                                while (1) {
                                    asm volatile("hlt");
                                    if (key_pressed != 0) {
                                        char c = key_pressed;
                                        key_pressed = 0;

                                        if (c == '\n') {
                                            print_char(' ', terminal_row, input_col, 0x07);
                                            local_line[line_idx] = '\0';
                                            terminal_row++;
                                            if (terminal_row >= 25) { scroll(); terminal_row = 24; }
                                            break;
                                        }
                                        else if (c == '\b') {
                                            if (input_col > 2) {
                                                print_char(' ', terminal_row, input_col, 0x07);
                                                input_col--;
                                                print_char(' ', terminal_row, input_col, 0x07);
                                                print_char('_', terminal_row, input_col, 0x0A);
                                                line_idx--;
                                            }
                                        }
                                        else {
                                            if (input_col < 79 && line_idx < 126) {
                                                local_line[line_idx++] = c;
                                                print_char(c, terminal_row, input_col, 0x0F);
                                                input_col++;
                                                print_char('_', terminal_row, input_col, 0x0A);
                                            }
                                        }
                                    }
                                }

                                // Если ввели "END", то прекращаем запись
                                if (str_compare(local_line, "END")) {
                                    writing = 0;
                                } else {
                                    // Переносим строку в большой буфер и дописываем \n
                                    for (int i = 0; local_line[i] != '\0'; i++) {
                                        big_buffer[big_idx++] = local_line[i];
                                    }
                                    big_buffer[big_idx++] = '\n';
                                }
                            }

                            big_buffer[big_idx] = '\0'; // Закрываем финальную строку

                            // Кидаем сформированный массив данных в твою цепочную ФС
                            if (fs_create_file(arg1, big_buffer)) {
                                print_string("Success: File created!", terminal_row, 0, 0x0A);
                            } else {
                                print_string("Error: No space or file already exists.", terminal_row, 0, 0x0C);
                            }
                        }
                    }
                    else if (str_compare(cmd, "EDIT")) {
                        if (arg1[0] == '\0') {
                            print_string("Usage: EDIT [filename]", terminal_row, 0, 0x0C);
                        } else {
                            // ИСПРАВЛЕНИЕ 1: Изолированный буфер памяти (0x60000 вместо 0x50000), 
                            // чтобы EDIT не затирал данные команды CREATE
                            char* big_buffer = (char*)0x60000;
                            int big_idx = 0;

                            // Полностью очищаем буфер перед вычиткой
                            for (int i = 0; i < 4096; i++) big_buffer[i] = 0;

                            // Пытаемся считать старые данные
                            int file_exists = fs_read_file(arg1, big_buffer);
                            
                            // Вычисляем длину считанного текста, чтобы ставить указатель в конец
                            while (big_buffer[big_idx] != '\0') {
                                big_idx++;
                            }

                            clear_screen();
                            // Статус-бар в инверсном цвете (0x30)
                            print_string("--- KrexOS Text Editor --- File: ", 0, 0, 0x30);
                            print_string(arg1, 0, 33, 0x30);
                            print_string(" --- Press ESC to Save and Exit ---", 0, 33 + 12, 0x30);

                            // Отрисовываем существующее содержимое файла на экране
                            int edit_row = 2;
                            int edit_col = 0;
                            int i = 0;
                            while (big_buffer[i] != '\0') {
                                char f_char = big_buffer[i];
                                if (f_char == '\n') {
                                    edit_row++;
                                    edit_col = 0;
                                    if (edit_row >= 25) { scroll(); edit_row = 24; }
                                } else {
                                    print_char(f_char, edit_row, edit_col, 0x0F);
                                    edit_col++;
                                    if (edit_col >= 80) {
                                        edit_col = 0;
                                        edit_row++;
                                        if (edit_row >= 25) { scroll(); edit_row = 24; }
                                    }
                                }
                                i++;
                            }

                            // Рисуем курсор там, где закончился текст файла
                            print_char('_', edit_row, edit_col, 0x0A);

                            int writing = 1;
                            while (writing) {
                                asm volatile("hlt"); // Ждем прерываний клавиатуры
                                
                                if (key_pressed != 0) {
                                    char c = key_pressed;
                                    key_pressed = 0;

                                    // ИСПРАВЛЕНИЕ 2: Корректный выход по клавише ESC
                                    if (c == 27) {
                                        print_char(' ', edit_row, edit_col, 0x07); // Стираем курсор
                                        writing = 0;
                                        break;
                                    }

                                    // Обработка ENTER
                                    if (c == '\n') {
                                        if (big_idx < 4094) {
                                            print_char(' ', edit_row, edit_col, 0x07); // Убираем старый курсор
                                            big_buffer[big_idx++] = '\n';
                                            
                                            edit_row++;
                                            edit_col = 0;
                                            if (edit_row >= 25) { 
                                                scroll(); 
                                                edit_row = 24; 
                                                print_string("--- KrexOS Text Editor --- File: ", 0, 0, 0x30);
                                                print_string(arg1, 0, 33, 0x30);
                                                print_string(" --- Press ESC to Save and Exit ---", 0, 33 + 12, 0x30);
                                            }
                                            print_char('_', edit_row, edit_col, 0x0A); // Новый курсор
                                        }
                                    }
                                    // ИСПРАВЛЕНИЕ 3: Переработанный, умный BACKSPACE
                                    else if (c == '\b') {
                                        if (big_idx > 0) {
                                            print_char(' ', edit_row, edit_col, 0x07); // Убираем курсор
                                            
                                            // Если удаляем перевод строки, возвращаем курсор наверх
                                            if (big_buffer[big_idx - 1] == '\n') {
                                                big_idx--;
                                                if (edit_row > 2) {
                                                    edit_row--;
                                                    
                                                    // Считаем положение курсора на предыдущей строке
                                                    int temp_idx = big_idx - 1;
                                                    int col_count = 0;
                                                    while (temp_idx >= 0 && big_buffer[temp_idx] != '\n') {
                                                        col_count++;
                                                        temp_idx--;
                                                    }
                                                    edit_col = col_count % 80;
                                                }
                                            } else {
                                                big_idx--; // Стираем символ из буфера памяти
                                                if (edit_col > 0) {
                                                    edit_col--;
                                                } else if (edit_row > 2) {
                                                    // Если стираем начало строки, которая перенеслась автоматически
                                                    edit_row--;
                                                    edit_col = 79;
                                                }
                                            }
                                            print_char(' ', edit_row, edit_col, 0x07); // Стираем символ с экрана
                                            print_char('_', edit_row, edit_col, 0x0A); // Ставим курсор на новую позицию
                                        }
                                    }
                                    // Обычные печатные символы
                                    else {
                                        if (edit_col < 79 && big_idx < 4094) {
                                            big_buffer[big_idx++] = c;
                                            print_char(c, edit_row, edit_col, 0x0F); 
                                            edit_col++;
                                            print_char('_', edit_row, edit_col, 0x0A); 
                                        }
                                        // Автоперенос строки на экране при достижении правого края (80 символов)
                                        else if (edit_col >= 79 && big_idx < 4094) {
                                            big_buffer[big_idx++] = c;
                                            print_char(c, edit_row, edit_col, 0x0F);
                                            
                                            edit_col = 0;
                                            edit_row++;
                                            if (edit_row >= 25) { 
                                                scroll(); 
                                                edit_row = 24; 
                                                print_string("--- KrexOS Text Editor --- File: ", 0, 0, 0x30);
                                                print_string(arg1, 0, 33, 0x30);
                                                print_string(" --- Press ESC to Save and Exit ---", 0, 33 + 12, 0x30);
                                            }
                                            print_char('_', edit_row, edit_col, 0x0A);
                                        }
                                    }
                                }
                            }
                            big_buffer[big_idx] = '\0';

                            // Сохранение: удаляем старый дескриптор и пишем новый файл
                            fs_delete_file(arg1);
                            clear_screen();
                            terminal_row = 1;

                            if (fs_create_file(arg1, big_buffer)) {
                                print_string("Success: File updated.", terminal_row, 0, 0x0A);
                            } else {
                                print_string("Error: Failed to update file.", terminal_row, 0, 0x0C);
                            }
                        }
                    }
                    else if (str_compare(cmd, "CAT")) {
                        if (arg1[0] == '\0') {
                            print_string("Usage: CAT [filename]", terminal_row, 0, 0x0C);
                        } else {
                            static char file_view_buf[4096];
                            if (fs_read_file(arg1, file_view_buf)) {
                                int i = 0;
                                int cat_col = 0;

                                while (file_view_buf[i] != '\0') {
                                    char f_char = file_view_buf[i];

                                    if (f_char == '\n') {
                                        terminal_row++;
                                        cat_col = 0;
                                        if (terminal_row >= 25) { 
                                            scroll(); 
                                            terminal_row = 24; 
                                        }
                                    } 
                                    else {
                                        print_char(f_char, terminal_row, cat_col, 0x0F);
                                        cat_col++;
                                        
                                        if (cat_col >= 80) {
                                            cat_col = 0;
                                            terminal_row++;
                                            if (terminal_row >= 25) { 
                                                scroll(); 
                                                terminal_row = 24; 
                                            }
                                        }
                                    }
                                    i++;
                                }
                                terminal_row--;
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
                print_string(username, terminal_row, 0, 0x0A);
                print_string("# ", terminal_row, user_len, 0x07);
                cursor_col = prefix_len; 
                cmd_index = 0;
                print_char('_', terminal_row, cursor_col, 0x0A);
            }
            else if (c == '\b') { // BACKSPACE
                if (cursor_col > prefix_len) {
                    print_char(' ', terminal_row, cursor_col, 0x07); 
                    cursor_col--; 
                    print_char(' ', terminal_row, cursor_col, 0x07); 
                    print_char('_', terminal_row, cursor_col, 0x0A); 
                    cmd_index--; 
                }
            }
            else { // Набор символов
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