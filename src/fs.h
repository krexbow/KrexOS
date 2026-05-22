#ifndef FS_H
#define FS_H

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

// Настройки разметки диска
#define FS_START_SECTOR   1000
#define BITMAP_SECTOR     1000  // 1 сектор под карту занятых блоков (хватит на 512 секторов)
#define INODE_START_SEC   1001  // Отсюда начинаются дескрипторы файлов
#define INODE_SECTORS     10    // 10 секторов * 8 дескрипторов = макс 80 файлов!
#define DATA_START_SEC    1011  // Отсюда начинаются чистые секторы данных
#define FS_END_SECTOR     1100  // Конец нашей зоны файловой системы

#pragma pack(push, 1)

// Дескриптор файла (ровно 64 байта). В один сектор влезает ровно 8 штук
typedef struct {
    char name[24];           // Имя файла (до 23 символов + \0)
    uint32_t size;           // Общий размер файла в байтах
    uint32_t start_sector;   // Номер первого сектора с данными на диске (0, если пустой)
    uint8_t used;            // Флаг: 1 - файл существует, 0 - свободен/удален
    uint8_t reserved[31];    // Паддинг для выравнивания до 64 байт
} file_descriptor_t;

// Структура сектора данных (512 байт)
typedef struct {
    uint32_t next_sector;    // Номер следующего сектора цепочки (0 - если это конец файла)
    uint8_t data[508];       // Полезные данные внутри этого сектора
} data_block_t;

#pragma pack(pop)

// Интерфейс файловой системы
int str_match(const char* s1, const char* s2);
int fs_create_file(const char* name, const char* content);
int fs_read_file(const char* name, char* out_buffer);
int fs_delete_file(const char* name);
void fs_list_files_internal(int* current_row);

// Инициализация файловой системы (проверка разметки жесткого диска при старте)
void fs_init(void);

#endif