#include "libc.h"
#include "keyboard.h"
#include "screen.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

volatile char key_pressed = 0; 
volatile int enter_triggered = 0;

void pic_remap(void) {
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    
    outb(PIC1_DATA, 0x20); 
    outb(PIC2_DATA, 0x28);
    
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    outb(PIC1_DATA, 0xFD); 
    outb(PIC2_DATA, 0xFF); 
}

void keyboard_handler_main(void) {
    uint8_t status = inb(0x64);
    
    if (status & 0x01) {
        uint8_t scancode = inb(0x60);
        
        if (!(scancode & 0x80)) { 
            if (scancode == 0x1C) { 
                enter_triggered = 1;
                key_pressed = '\n';
            } 
            else if (scancode == 0x0E) { 
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
                else if (scancode == 0x01) key = '\x1B';

                if (key != 0) {
                    key_pressed = key;
                }
            }
        }
    }
    outb(0x20, 0x20);
}

void wait_for_enter(void) {
    enter_triggered = 0;
    key_pressed = 0; 
    while (!enter_triggered) {
        asm volatile("hlt"); 
    }
    enter_triggered = 0;
    key_pressed = 0; 
}

void read_line(char* buffer, int max_len, int row, int start_col, int mask_input) {
    int index = 0;
    int current_col = start_col;
    print_char('_', row, current_col, 0x0A);

    while (1) {
        asm volatile("hlt"); 
        
        if (key_pressed != 0) {
            char c = key_pressed;
            key_pressed = 0; 

            if (c == '\n') { 
                print_char(' ', row, current_col, 0x07);
                buffer[index] = '\0';
                break;
            } 
            else if (c == '\b') { 
                if (index > 0) {
                    print_char(' ', row, current_col, 0x07);
                    current_col--;
                    print_char(' ', row, current_col, 0x07);
                    print_char('_', row, current_col, 0x0A);
                    index--;
                }
            } 
            else { 
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