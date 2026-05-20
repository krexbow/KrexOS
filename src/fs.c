#include "fs.h"
#include "ata.h" // Твой драйвер жесткого диска (ata_read_sector / ata_write_sector)

// Статический буфер, чтобы не перегружать стек ядра при чтении секторов диска
static uint8_t fs_buffer[512];

// Локальная функция сравнения строк (так как стандартной strcmp у нас нет)
int str_match(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return 0;
        i++;
    }
    return (s1[i] == '\0' && s2[i] == '\0');
}

// 1. Создание нового файла
int fs_create_file(const char* name, const char* content) {
    vfs_file_t new_file;
    
    // Чистим структуру нулями перед заполнением
    for (int i = 0; i < 512; i++) {
        ((uint8_t*)&new_file)[i] = 0;
    }
    
    // Копируем имя файла
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        new_file.name[i] = name[i];
        i++;
    }
    new_file.name[i] = '\0';
    
    // Копируем текст внутрь файла
    i = 0;
    while (content[i] != '\0' && i < 475) {
        new_file.data[i] = content[i];
        i++;
    }
    new_file.data[i] = '\0';
    new_file.size = i;

    // Ищем свободное место в нашей зоне диска
    for (uint32_t sec = FS_START_SECTOR; sec < (FS_START_SECTOR + MAX_FILES); sec++) {
        ata_read_sector(sec, fs_buffer);
        vfs_file_t* slot = (vfs_file_t*)fs_buffer;
        
        // Проверяем, нет ли уже файла с точно таким же именем
        if (str_match(slot->name, name)) {
            return 0; // Ошибка: Дубликат!
        }

        // Если первый байт имени равен нулю, значит сектор свободен
        if (slot->name[0] == '\0') {
            ata_write_sector(sec, (uint8_t*)&new_file);
            return 1; // Файл успешно записан на диск!
        }
    }
    return 0; // Ошибка: Закончилось место (все 10 слотов заняты)
}

// 2. Чтение файла по имени
int fs_read_file(const char* name, char* out_buffer) {
    for (uint32_t sec = FS_START_SECTOR; sec < (FS_START_SECTOR + MAX_FILES); sec++) {
        ata_read_sector(sec, fs_buffer);
        vfs_file_t* file = (vfs_file_t*)fs_buffer;
        
        // Если имя совпало, копируем данные в буфер шелла
        if (str_match(file->name, name)) {
            int i = 0;
            while (i < file->size) {
                out_buffer[i] = file->data[i];
                i++;
            }
            out_buffer[i] = '\0'; // Гарантируем корректный конец строки
            return 1; // Файл найден и прочитан
        }
    }
    return 0; // Файл не найден
}

// 3. Удаление файла (Мгновенное освобождение сектора занулением имени)
int fs_delete_file(const char* name) {
    for (uint32_t sec = FS_START_SECTOR; sec < (FS_START_SECTOR + MAX_FILES); sec++) {
        ata_read_sector(sec, fs_buffer);
        vfs_file_t* file = (vfs_file_t*)fs_buffer;
        
        if (str_match(file->name, name)) {
            file->name[0] = '\0'; // Стираем имя. Сектор теперь помечен как пустой
            file->size = 0;
            ata_write_sector(sec, fs_buffer); // Обновляем сектор на физическом диске
            return 1; // Успешно удалено
        }
    }
    return 0; // Файл для удаления не найден
}

// 4. Вывод списка файлов (Интегрировано под прокрутку экрана в kernel.c)
extern void fs_print_wrapper(const char* str, int* row);

void fs_list_files_internal(int* current_row) {
    fs_print_wrapper("--- Files on Drive C: ---", current_row);
    int count = 0;
    
    for (uint32_t sec = FS_START_SECTOR; sec < (FS_START_SECTOR + MAX_FILES); sec++) {
        ata_read_sector(sec, fs_buffer);
        vfs_file_t* file = (vfs_file_t*)fs_buffer;
        
        // Если первый символ не \0 — файл живой, выводим его имя
        if (file->name[0] != '\0') {
            fs_print_wrapper(file->name, current_row);
            count++;
        }
    }
    
    if (count == 0) {
        fs_print_wrapper("(empty)", current_row);
    }
}