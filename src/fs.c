#include "fs.h"
#include "ata.h"

// Прототипы функций работы с экраном для расширенного вывода LS
extern void print_string(const char* str, int row, int col, char color);
extern void print_int(int num, int row, int col, char color);
extern void scroll(void);
extern void fs_print_wrapper(const char* str, int* row);

// Глобальные переменные состояния ФС (координаты текущей папки и путь)
char current_path[256] = "/";
uint32_t current_dir_sector = 0;   // LBA-сектор дескриптора текущей папки (0 — корень)
uint8_t current_dir_index = 0;     // Индекс внутри сектора (0-7, для корня не имеет значения)

static uint8_t fs_buffer[512];
static uint8_t bitmap_buffer[512];

// Вспомогательная функция сравнения строк
int str_match(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return 0;
        i++;
    }
    return (s1[i] == '\0' && s2[i] == '\0');
}

// Вспомогательная функция безопасного копирования строк
static void str_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Поиск свободного сектора в Bitmap
static uint32_t allocate_sector() {
    ata_read_sector(BITMAP_SECTOR, bitmap_buffer);
    
    // Индекс 0 и все индексы до DATA_START_SEC не используются как секторы данных
    for (uint32_t i = DATA_START_SEC; i <= FS_END_SECTOR; i++) {
        if (bitmap_buffer[i - FS_START_SECTOR] == 0) {
            bitmap_buffer[i - FS_START_SECTOR] = 1; // Помечаем как занятый
            ata_write_sector(BITMAP_SECTOR, bitmap_buffer);
            return i;
        }
    }
    return 0; // Нет свободных секторов
}

// Освобождение сектора в Bitmap
static void free_sector(uint32_t sector) {
    if (sector < DATA_START_SEC || sector > FS_END_SECTOR) return;
    ata_read_sector(BITMAP_SECTOR, bitmap_buffer);
    bitmap_buffer[sector - FS_START_SECTOR] = 0; // Освобождаем
    ata_write_sector(BITMAP_SECTOR, bitmap_buffer);
}

// 1. Создание/Запись файла (С изоляцией внутри текущей папки)
int fs_create_file(const char* name, const char* content) {
    // Сначала проверяем, нет ли уже файла/папки с таким именем В ТЕКУЩЕЙ директории
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && 
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                return 0; // Ошибка: Файл или папка уже существует!
            }
        }
    }

    // Ищем свободное место под дескриптор нового файла
    uint32_t desc_sector = 0;
    int desc_index = -1;
    
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        for (int f = 0; f < 8; f++) {
            if (!desc_array[f].used) {
                desc_sector = sec;
                desc_index = f;
                break;
            }
        }
        if (desc_index != -1) break;
    }

    if (desc_index == -1) return 0; // Ошибка: Максимум файлов в системе достигнут!

    // Считаем общую длину текста
    uint32_t content_len = 0;
    while (content[content_len] != '\0') content_len++;

    // Начинаем писать данные по секторам цепочкой
    uint32_t start_sector = 0;
    uint32_t current_sector = 0;
    uint32_t content_idx = 0;

    while (content_idx < content_len) {
        uint32_t new_sec = allocate_sector();
        if (new_sec == 0) {
            break; // Нехватка места на диске
        }

        if (start_sector == 0) {
            start_sector = new_sec;
        } else {
            // Связываем предыдущий сектор с текущим новым
            ata_read_sector(current_sector, fs_buffer);
            data_block_t* prev_block = (data_block_t*)fs_buffer;
            prev_block->next_sector = new_sec;
            ata_write_sector(current_sector, fs_buffer);
        }

        current_sector = new_sec;

        // Заполняем текущий сектор данными (до 508 байт)
        data_block_t new_block;
        new_block.next_sector = 0; // Конец цепочки по умолчанию
        for (int i = 0; i < 512; i++) ((uint8_t*)&new_block)[i] = 0; // Очистка

        int block_bytes = 0;
        while (content_idx < content_len && block_bytes < 508) {
            new_block.data[block_bytes] = content[content_idx];
            block_bytes++;
            content_idx++;
        }
        
        ata_write_sector(current_sector, (uint8_t*)&new_block);
    }

    // Записываем заполненный дескриптор файла обратно на диск
    ata_read_sector(desc_sector, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
    
    str_copy(desc_array[desc_index].name, name, 24);
    desc_array[desc_index].size = content_len;
    desc_array[desc_index].start_sector = start_sector;
    desc_array[desc_index].used = 1;
    desc_array[desc_index].is_dir = 0; // Это обычный файл
    desc_array[desc_index].parent_sector = current_dir_sector;
    desc_array[desc_index].parent_index = current_dir_index;

    ata_write_sector(desc_sector, fs_buffer);
    return 1;
}

// 2. Чтение файла по цепочке секторов (Только из текущей папки)
int fs_read_file(const char* name, char* out_buffer) {
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && 
                !desc_array[f].is_dir && // Проверяем, что это не папка
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                
                uint32_t current_sector = desc_array[f].start_sector;
                uint32_t out_idx = 0;

                // Читаем секторы один за другим, пока next_sector не станет равен 0
                while (current_sector != 0) {
                    uint8_t local_block_buf[512];
                    ata_read_sector(current_sector, local_block_buf);
                    data_block_t* block = (data_block_t*)local_block_buf;

                    int i = 0;
                    // Извлекаем до 508 байт полезных данных
                    while (out_idx < desc_array[f].size && i < 508) {
                        out_buffer[out_idx] = block->data[i];
                        out_idx++;
                        i++;
                    }
                    
                    current_sector = block->next_sector; // Шаг по цепочке
                }
                out_buffer[out_idx] = '\0'; // Строковый терминатор
                return 1;
            }
        }
    }
    return 0; // Файл не найден
}

// 3. Удаление файла или пустой папки в текущей директории
int fs_delete_file(const char* name) {
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && 
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                
                // Если мы пытаемся удалить директорию, сначала проверяем, пуста ли она
                if (desc_array[f].is_dir) {
                    for (uint32_t sub_sec = INODE_START_SEC; sub_sec < DATA_START_SEC; sub_sec++) {
                        uint8_t verification_buffer[512];
                        ata_read_sector(sub_sec, verification_buffer);
                        file_descriptor_t* sub_desc = (file_descriptor_t*)verification_buffer;
                        for (int sub_f = 0; sub_f < 8; sub_f++) {
                            // Если внутри ФС есть хоть один элемент, ссылающийся на эту папку как на родителя
                            if (sub_desc[sub_f].used && 
                                sub_desc[sub_f].parent_sector == sec && 
                                sub_desc[sub_f].parent_index == f) {
                                return 0; // Ошибка: Директория не пуста!
                            }
                        }
                    }
                }

                // Идем по цепочке блоков (для файлов) и освобождаем их в Bitmap
                uint32_t current_sector = desc_array[f].start_sector;
                while (current_sector != 0) {
                    uint8_t local_block_buf[512];
                    ata_read_sector(current_sector, local_block_buf);
                    data_block_t* block = (data_block_t*)local_block_buf;
                    
                    uint32_t next = block->next_sector;
                    free_sector(current_sector); // Сбрасываем в Bitmap
                    current_sector = next;
                }

                // Стираем запись в дескрипторе
                desc_array[f].used = 0;
                desc_array[f].name[0] = '\0';
                desc_array[f].size = 0;
                desc_array[f].start_sector = 0;
                desc_array[f].is_dir = 0;

                ata_write_sector(sec, fs_buffer);
                return 1;
            }
        }
    }
    return 0; // Не найдено
}

// 4. Вывод содержимого ТОЛЬКО текущей директории
void fs_list_files_internal(int* current_row) {
    fs_print_wrapper("--- Directory contents ---", current_row);
    int count = 0;

    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            // Фильтруем: выводим записи, у которых родитель — текущая папка
            if (desc_array[f].used && 
                desc_array[f].parent_sector == current_dir_sector && 
                desc_array[f].parent_index == current_dir_index) {
                
                if (*current_row >= 25) { 
                    scroll(); 
                    *current_row = 24; 
                }

                if (desc_array[f].is_dir) {
                    // Вывод ПАПКИ: Желтый цвет (0x0E), добавляем маркер '/' и метку <DIR>
                    char dir_visual_name[26];
                    int len = 0;
                    while (desc_array[f].name[len] != '\0') {
                        dir_visual_name[len] = desc_array[f].name[len];
                        len++;
                    }
                    dir_visual_name[len] = '/';
                    dir_visual_name[len+1] = '\0';

                    print_string(dir_visual_name, *current_row, 0, 0x0E);
                    print_string("  <DIR>", *current_row, 20, 0x0E);
                } else {
                    // Вывод ФАЙЛА: Белый цвет (0x0F) и размер в байтах
                    print_string(desc_array[f].name, *current_row, 0, 0x0F);
                    print_string("  [", *current_row, 20, 0x07);
                    print_int(desc_array[f].size, *current_row, 23, 0x0A);
                    print_string(" bytes]", *current_row, 30, 0x07);
                }

                (*current_row)++;
                count++;
            }
        }
    }

    if (count == 0) {
        fs_print_wrapper("(directory is empty)", current_row);
    }
}

// 5. Создание новой директории (MKDIR)
int fs_mkdir(const char* name) {
    // Проверяем, уникально ли имя в текущей папке
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && 
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                return 0; // Имя уже занято
            }
        }
    }

    // Ищем пустой Inode для новой папки
    uint32_t desc_sector = 0;
    int desc_index = -1;

    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        for (int f = 0; f < 8; f++) {
            if (!desc_array[f].used) {
                desc_sector = sec;
                desc_index = f;
                break;
            }
        }
        if (desc_index != -1) break;
    }

    if (desc_index == -1) return 0; // Диск переполнен дескрипторами

    // Читаем сектор, куда будем писать, и заполняем дескриптор каталога
    ata_read_sector(desc_sector, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;

    str_copy(desc_array[desc_index].name, name, 24);
    desc_array[desc_index].size = 0;
    desc_array[desc_index].start_sector = 0; // Данные в секторах папка не хранит
    desc_array[desc_index].used = 1;
    desc_array[desc_index].is_dir = 1;       // Выставляем флаг папки!
    desc_array[desc_index].parent_sector = current_dir_sector;
    desc_array[desc_index].parent_index = current_dir_index;

    ata_write_sector(desc_sector, fs_buffer);
    return 1;
}

// 5.5 Удаление пустой директории (RMDIR)
int fs_rmdir(const char* name) {
    uint32_t target_sector = 0;
    int target_index = -1;

    // 1. Ищем дескриптор указанной папки внутри ТЕКУЩЕЙ директории
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && desc_array[f].is_dir &&
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                
                target_sector = sec;
                target_index = f;
                break;
            }
        }
        if (target_index != -1) break;
    }

    // Если папка с таким именем не найдена в текущем пути
    if (target_index == -1) return 0; 

    // 2. Проверяем, пуста ли эта папка (сканируем всё на предмет детей)
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        uint8_t check_buffer[512];
        ata_read_sector(sec, check_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)check_buffer;
        
        for (int f = 0; f < 8; f++) {
            // Если есть элемент, чей родитель — удаляемая папка, отменяем операцию
            if (desc_array[f].used && 
                desc_array[f].parent_sector == target_sector && 
                desc_array[f].parent_index == target_index) {
                return 0; // Ошибка: Директория не пуста!
            }
        }
    }

    // 3. Удаляем папку (освобождаем дескриптор)
    ata_read_sector(target_sector, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
    
    desc_array[target_index].used = 0;
    desc_array[target_index].name[0] = '\0';
    desc_array[target_index].size = 0;
    desc_array[target_index].start_sector = 0;
    desc_array[target_index].is_dir = 0;

    ata_write_sector(target_sector, fs_buffer);
    return 1; // Успешно удалено
}

// 6. Смена текущего каталога (CD)
int fs_cd(const char* name) {
    // Вариант 1: Команда выхода на уровень вверх (CD ..)
    if (str_match(name, "..")) {
        if (current_dir_sector == 0) {
            return 1; // Мы уже в корне, выше некуда
        }
        
        // Читаем дескриптор текущей папки, чтобы узнать координаты её родителя
        ata_read_sector(current_dir_sector, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        uint32_t target_parent_sector = desc_array[current_dir_index].parent_sector;
        uint8_t target_parent_index = desc_array[current_dir_index].parent_index;
        
        current_dir_sector = target_parent_sector;
        current_dir_index = target_parent_index;
        
        // Пересчитываем красивую строку пути для отображения в Shell
        if (current_dir_sector == 0) {
            str_copy(current_path, "/", 256);
        } else {
            // Удаляем последний сегмент пути (отрезаем до предпоследнего '/')
            int len = 0;
            while (current_path[len] != '\0') len++;
            len--; // Стоим на завершающем '/'
            if (len > 0) {
                len--;
                while (len > 0 && current_path[len] != '/') len--;
                current_path[len + 1] = '\0'; // Обрезаем строку
            }
        }
        return 1;
    }

    // Вариант 2: Переход в подпапку по её имени
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && 
                desc_array[f].is_dir && 
                desc_array[f].parent_sector == current_dir_sector &&
                desc_array[f].parent_index == current_dir_index &&
                str_match(desc_array[f].name, name)) {
                
                // Переключаем глобальные координаты ФС на эту папку
                current_dir_sector = sec;
                current_dir_index = f;
                
                // Обновляем визуальную строку пути `current_path`
                int len = 0;
                while (current_path[len] != '\0') len++;
                
                int i = 0;
                while (name[i] != '\0' && (len + i) < 250) {
                    current_path[len + i] = name[i];
                    i++;
                }
                current_path[len + i] = '/';
                current_path[len + i + 1] = '\0';
                
                return 1; // Успешный переход
            }
        }
    }
    
    return 0; // Ошибка: Такой папки в текущем каталоге нет
}

// 7. Автоматическое форматирование / инициализация структуры диска
void fs_init(void) {
    // Сбрасываем виртуальный путь в корень при старте ядра
    str_copy(current_path, "/", 256);
    current_dir_sector = 0;
    current_dir_index = 0;

    ata_read_sector(INODE_START_SEC, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
    
    // Если диск абсолютно "чистый", форматируем его
    if (desc_array[0].used == 0 && desc_array[0].start_sector == 0 && desc_array[0].name[0] == '\0') {
        
        // 1. Сбрасываем Bitmap в 0
        for (int i = 0; i < 512; i++) bitmap_buffer[i] = 0;
        ata_write_sector(BITMAP_SECTOR, bitmap_buffer);
        
        // 2. Полностью вычищаем секторы дескрипторов (Inodes)
        for (int i = 0; i < 512; i++) fs_buffer[i] = 0;
        for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
            ata_write_sector(sec, fs_buffer);
        }
    }
}