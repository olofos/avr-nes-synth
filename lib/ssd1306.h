#ifndef SSD1306_h_
#define SSD1306_h_

#define SSD1306_LINE_WIDTH 21

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_splash(void);

void ssd1306_console_init(void);
void ssd1306_console_putc(const char c);
void ssd1306_console_puts(const char* str);
void ssd1306_console_puts_P(const char* str);

void ssd1306_bitmap_full_P(const uint8_t *buf);
void ssd1306_bitmap_P(const uint8_t *buf, uint8_t cols, uint8_t rows, uint8_t x, uint8_t y);

void ssd1306_bitmap_begin(void);
void ssd1306_bitmap_end(void);

void ssd1306_puts(const char* str, uint8_t x, uint8_t y);
void ssd1306_puts_P(const char* str, uint8_t x, uint8_t y);
void ssd1306_putc(const char c, uint8_t x, uint8_t y);

void ssd1306_text_start(uint8_t x, uint8_t y);
void ssd1306_text_end(void);
void ssd1306_text_putc(char c);

void ssd1306_puts_scroll(const char* str, uint8_t x, uint8_t y, int8_t w, uint8_t dx);

#endif
