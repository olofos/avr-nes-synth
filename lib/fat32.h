#ifndef FAT32_H_
#define FAT32_H_

void fat32_init();

void fat32_open_root_dir();

uint8_t fat32_open_file(const char *filename);
void fat32_close_file();

uint16_t fat32_read(void *raw_buf, uint16_t len);

void fat32_seek(uint16_t len);

void fat32_list_dir();

#endif
