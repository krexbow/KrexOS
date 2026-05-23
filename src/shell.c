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
                        print_string(username, terminal_row, 14, 0x0F);
                    }
                    else if (str_compare(cmd, "DISK")) {
                        cmd_disk(&terminal_row);
                        terminal_row--; 
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