#include "ui.h"
#include "screen.h"
#include "keyboard.h"

void ui_show_splash(void) {
    clear_screen();
    
    // Рисуем декоративные рамки из дефисов
    print_string("--------------------------------------------------", 8, 15, 0x03); // Голубой
    print_string("Welcome to KrexOS v0.0.1!", 10, 26, 0x0A);                          // Зеленый
    print_string("Powered by Krexbow", 12, 29, 0x0F);                                // Белый
    print_string("Press ENTER to start registration...", 16, 21, 0x0E);              // Желтый
    print_string("--------------------------------------------------", 18, 15, 0x03);

    wait_for_enter();
}

void ui_draw_window_setup(const char* title) {
    clear_screen();
    
    // Высчитываем примерный центр для красивой рамки
    print_string("=== ", 2, 20, 0x0B);
    print_string(title, 2, 24, 0x0B);
    print_string(" ===", 2, 55, 0x0B);
}

void ui_draw_field_label(const char* label, int row) {
    // Все лейблы будут выравниваться по колонке 5
    print_string(label, row, 5, 0x0F);
}

void ui_show_status(const char* message, int is_error) {
    // Сами выбираем цвет в зависимости от флага ошибки
    char color = is_error ? 0x0C : 0x0A; // Красный или Зеленый
    print_string(message, 12, 5, color);
}