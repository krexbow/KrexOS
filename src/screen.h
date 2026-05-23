#ifndef SCREEN_H
#define SCREEN_H

void clear_screen(void);
void scroll(void);
void print_string(const char* str, int row, int col, char color);
void print_char(char c, int row, int col, char color);

#endif