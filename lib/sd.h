#ifndef SD_H_
#define SD_H_

uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc);

void sd_begin_sector(uint32_t sector);
void sd_end_sector(void);

uint8_t sd_sector_done(void);

uint32_t sd_read_uint32(void);
uint16_t sd_read_uint16(void);
uint8_t sd_read_uint8(void);

void sd_skip_bytes(uint16_t bytes);

void sd_debug_print_16_bytes(void);
void sd_init(void);

#endif
