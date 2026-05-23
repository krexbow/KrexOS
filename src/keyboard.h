#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "libc.h" 


extern volatile char key_pressed;
extern volatile int enter_triggered;

void pic_remap(void);
void keyboard_handler_main(void);
void wait_for_enter(void);
void read_line(char* buffer, int max_len, int row, int start_col, int mask_input);

#endif