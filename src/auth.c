#include "auth.h"
#include "ui.h"        // Подключаем наш новый UI движок
#include "keyboard.h"
#include "libc.h"

static char registered_user[32] = "";
static char registered_password[32] = "";
static char input_user[32];
static char input_password[32];

const char* auth_run_system(void) {
    // ЭКРАН 2: РЕГИСТРАЦИЯ
    ui_draw_window_setup("KrexOS ACCOUNT REGISTRATION");
    
    ui_draw_field_label("Create Username: ", 6);
    read_line(registered_user, 32, 6, 22, 0);
    
    ui_draw_field_label("Create Password: ", 8);
    read_line(registered_password, 32, 8, 22, 1);
    
    ui_show_status("Registration successful! Press ENTER to Login...", 0);
    wait_for_enter();

    // ЭКРАН 3: ЛОГИН
    while (1) {
        ui_draw_window_setup("KrexOS SYSTEM LOGIN");
        
        ui_draw_field_label("Enter Username: ", 6);
        read_line(input_user, 32, 6, 21, 0);
        
        ui_draw_field_label("Enter Password: ", 8);
        read_line(input_password, 32, 8, 21, 1);

        if (str_compare(input_user, registered_user) && str_compare(input_password, registered_password)) {
            ui_show_status("Access Granted! Loading Shell...", 0);
            for(volatile long long d = 0; d < 20000000; d++);
            break;
        } else {
            ui_show_status("INVALID CREDENTIALS! Press ENTER to try again.", 1); // Передаем 1 (ошибка)
            wait_for_enter();
        }
    }

    return registered_user;
}