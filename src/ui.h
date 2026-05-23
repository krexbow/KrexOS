#ifndef UI_H
#define UI_H

// Показывает красивый приветственный экран (ЭКРАН 1)
void ui_show_splash(void);

// Очищает экран и рисует стандартную шапку/заголовок для окон
void ui_draw_window_setup(const char* title);

// Выводит поле ввода (например, "Enter Username: ")
void ui_draw_field_label(const char* label, int row);

// Показывает статусное сообщение (успех/ошибка)
void ui_show_status(const char* message, int is_error);

#endif