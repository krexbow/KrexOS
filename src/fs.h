#ifndef FS_H
#define FS_H

typedef unsigned char uint8_t;
typedef unsigned int   uint32_t;

// Настройки разметки диска
#define FS_START_SECTOR   1000
#define BITMAP_SECTOR     1000  // 1 сектор под карту занятых блоков (хватит на 512 секторов)
#define INODE_START_SEC   1001  // Отсюда начинаются дескрипторы файлов
#define INODE_SECTORS     10    // 10 секторов * 8 дескрипторов = макс 80 файлов/папок!
#define DATA_START_SEC    1011  // Отсюда начинаются чистые секторы данных
#define FS_END_SECTOR     1100  // Конец нашей зоны файловой системы

#pragma pack(push, 1)

// Дескриптор файла или директории (ровно 64 байта). В один сектор влезает ровно 8 штук
typedef struct {
    char name[24];           // Имя файла/папки (до 23 символов + \0)
    uint32_t size;           // Общий размер файла в байтах (для папок равен 0)
    uint32_t start_sector;   // Номер первого сектора с данными на диске (0, если пустой)
    uint8_t used;            // Флаг: 1 - элемент существует, 0 - свободен/удален
    
    // --- Расширение для поддержки древовидной структуры папок ---
    uint8_t is_dir;          // Флаг: 0 - файл, 1 - директория (папка)
    uint32_t parent_sector;  // Сектор дескриптора родительской папки (0, если корень)
    uint8_t parent_index;    // Индекс дескриптора родительской папки внутри сектора (0-7)
    
    uint8_t reserved[25];    // Уменьшили паддинг с 31 до 25 байт. Общий размер остался строго 64 байта!
} file_descriptor_t;

// Структура сектора данных (512 байт)
typedef struct {
    uint32_t next_sector;    // Номер следующего сектора цепочки (0 - если это конец файла)
    uint8_t data[508];       // Полезные данные внутри этого сектора
} data_block_t;

#pragma pack(pop)

// Глобальные переменные состояния ФС (нужны для Шелла, чтобы знать где мы находимся)
extern char current_path[256];
extern uint32_t current_dir_sector;
extern uint8_t current_dir_index;

// Интерфейс файловой системы
int str_match(const char* s1, const char* s2);
int fs_create_file(const char* name, const char* content);
int fs_read_file(const char* name, char* out_buffer);
int fs_delete_file(const char* name);
void fs_list_files_internal(int* current_row);

// Функции для работы с подкаталогами (директориями)
int fs_mkdir(const char* name);
int fs_rmdir(const char* name);
int fs_cd(const char* name);

// Инициализация файловой системы (проверка разметки жесткого диска при старте)
void fs_init(void);

#endif