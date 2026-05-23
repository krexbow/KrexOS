#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include "libc.h" 


void ata_identify();
void cmd_disk(int* terminal_row);
void ata_read_sector(uint32_t lba, uint8_t* buffer); 
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif