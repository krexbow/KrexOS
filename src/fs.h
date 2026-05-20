#ifndef FS_H
#define FS_H

// Ручное объявление типов, чтобы VS Code и GCC никогда не конфликтовали
typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

#define FS_START_SECTOR 1000
#define MAX_FILES 10

// Включаем жесткое выравнивание в 1 байт для идеальной укладки структуры в сектор
#pragma pack(push, 1)

typedef struct {
    char name[32];          // Имя файла (до 31 символа + \0)
    uint32_t size;          // Реальный размер текста в байтах
    uint8_t data[476];      // Содержимое файла (512 - 32 - 4 = 476 байт)
} vfs_file_t;

#pragma pack(pop)

// Интерфейс нашей файловой системы
int str_match(const char* s1, const char* s2);
int fs_create_file(const char* name, const char* content);
int fs_read_file(const char* name, char* out_buffer);
int fs_delete_file(const char* name);
void fs_list_files_internal(int* current_row);

#endif