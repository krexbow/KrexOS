#include "fs.h"
#include "ata.h"

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

// 1. Создание/Запись файла (с поддержкой цепочек секторов)
int fs_create_file(const char* name, const char* content) {
    // Сначала проверяем, нет ли уже файла с таким именем
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && str_match(desc_array[f].name, name)) {
                return 0; // Ошибка: Файл уже существует!
            }
        }
    }

    // Ищем свободное место под дескриптор файла
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

    if (desc_index == -1) return 0; // Ошибка: Максимум файлов достигнут!

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
            // Если место кончилось посреди записи — это фиаско, но пока упростим
            break; 
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
        new_block.next_sector = 0; // Пока что это конец цепочки
        for (int i = 0; i < 512; i++) ((uint8_t*)&new_block)[i] = 0; // Чистим блок

        int block_bytes = 0;
        while (content_idx < content_len && block_bytes < 508) {
            new_block.data[block_bytes] = content[content_idx];
            block_bytes++;
            content_idx++;
        }
        
        ata_write_sector(current_sector, (uint8_t*)&new_block);
    }

    // Теперь записываем сам дескриптор файла на диск
    ata_read_sector(desc_sector, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
    
    int i = 0;
    while (name[i] != '\0' && i < 23) {
        desc_array[desc_index].name[i] = name[i];
        i++;
    }
    desc_array[desc_index].name[i] = '\0';
    desc_array[desc_index].size = content_len;
    desc_array[desc_index].start_sector = start_sector;
    desc_array[desc_index].used = 1;

    ata_write_sector(desc_sector, fs_buffer);
    return 1;
}

// 2. Чтение файла по цепочке секторов
int fs_read_file(const char* name, char* out_buffer) {
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && str_match(desc_array[f].name, name)) {
                
                uint32_t current_sector = desc_array[f].start_sector;
                uint32_t out_idx = 0;

                // Читаем секторы один за другим, пока next_sector не станет равен 0
                while (current_sector != 0) {
                    uint8_t local_block_buf[512];
                    ata_read_sector(current_sector, local_block_buf);
                    data_block_t* block = (data_block_t*)local_block_buf;

                    int i = 0;
                    // Извлекаем до 508 байт полезных данных из сектора
                    while (out_idx < desc_array[f].size && i < 508) {
                        out_buffer[out_idx] = block->data[i];
                        out_idx++;
                        i++;
                    }
                    
                    current_sector = block->next_sector; // Переходим по указателю на следующий сектор
                }
                out_buffer[out_idx] = '\0'; // Строковый терминатор
                return 1;
            }
        }
    }
    return 0; // Файл не найден
}

// 3. Полное удаление файла и очистка всех его секторов в Bitmap
int fs_delete_file(const char* name) {
    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && str_match(desc_array[f].name, name)) {
                
                uint32_t current_sector = desc_array[f].start_sector;
                
                // Идем по цепочке блоков и освобождаем их в Bitmap
                while (current_sector != 0) {
                    uint8_t local_block_buf[512];
                    ata_read_sector(current_sector, local_block_buf);
                    data_block_t* block = (data_block_t*)local_block_buf;
                    
                    // Безопасно сохраняем указатель на следующий сектор перед затиранием
                    uint32_t next = block->next_sector;
                    free_sector(current_sector); // Сбрасываем в 0 в Bitmap
                    current_sector = next;
                }

                // Стираем сам дескриптор файла
                desc_array[f].used = 0;
                desc_array[f].name[0] = '\0';
                desc_array[f].size = 0;
                desc_array[f].start_sector = 0;

                ata_write_sector(sec, fs_buffer);
                return 1;
            }
        }
    }
    return 0;
}

// 4. Вывод списка файлов для Шелла
extern void fs_print_wrapper(const char* str, int* row);

void fs_list_files_internal(int* current_row) {
    fs_print_wrapper("--- Files on Drive C (VFS 2.0 FAT-like) ---", current_row);
    int count = 0;

    for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
        ata_read_sector(sec, fs_buffer);
        file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
        
        for (int f = 0; f < 8; f++) {
            if (desc_array[f].used && desc_array[f].name[0] != '\0') {
                fs_print_wrapper(desc_array[f].name, current_row);
                count++;
            }
        }
    }

    if (count == 0) {
        fs_print_wrapper("(empty)", current_row);
    }
}

// 5. Автоматическое форматирование / инициализация сырого диска
void fs_init(void) {
    // Читаем самый первый сектор дескрипторов файлов
    ata_read_sector(INODE_START_SEC, fs_buffer);
    file_descriptor_t* desc_array = (file_descriptor_t*)fs_buffer;
    
    // Проверяем «сырость» диска. Если первый дескриптор полностью нулевой,
    // значит диск еще ни разу не размечался в нашей ОС.
    if (desc_array[0].used == 0 && desc_array[0].start_sector == 0 && desc_array[0].name[0] == '\0') {
        
        // 1. Очищаем и создаем пустую карту секторов (Bitmap)
        for (int i = 0; i < 512; i++) {
            bitmap_buffer[i] = 0;
        }
        ata_write_sector(BITMAP_SECTOR, bitmap_buffer);
        
        // 2. Очищаем все секторы, зарезервированные под дескрипторы файлов (Inodes)
        for (int i = 0; i < 512; i++) {
            fs_buffer[i] = 0;
        }
        for (uint32_t sec = INODE_START_SEC; sec < DATA_START_SEC; sec++) {
            ata_write_sector(sec, fs_buffer);
        }
    }
}