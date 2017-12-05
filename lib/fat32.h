#ifndef FAT32_H_
#define FAT32_H_

void fat32_init(void);

void fat32_open_root_dir(void);

uint8_t fat32_open_file(const char *filename, const char *ext);
void fat32_close_file(void);

uint16_t fat32_read(void *raw_buf, uint16_t len);
uint16_t fat32_skip_until(uint8_t c);

void fat32_seek(uint16_t len);

void fat32_list_dir(void);

#endif
