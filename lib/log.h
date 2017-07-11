#ifndef LOG_H_
#define LOG_H_

#define LOG_UART    _BV(0)
#define LOG_SSD1306 _BV(1)
#define LOG_I2C     _BV(2)
#define LOG_BUF     _BV(3)

void log_init(uint8_t mask);

void log_enable(uint8_t mask);
void log_disable(uint8_t mask);

void log_putc(const char c);
void log_puts(const char* str);
void log_puts_P(const char* str);


#ifdef LOG_USE_PRINTF
void log_printf(const char *fmt, ... );
#endif

void log_put_uint8_hex(uint8_t val);
void log_put_uint16_hex(uint16_t val);
void log_put_uint32_hex(uint32_t val);

void log_put_uint8(uint8_t val);
void log_put_uint16(uint16_t val);

void log_put_int8(int8_t val);
void log_put_int16(int8_t val);

void log_put_ascii(uint8_t val);

inline void log_nl() { log_putc('\n'); };

#ifdef LOG_USE_BUF
uint8_t log_buf_count_lines();
uint8_t log_buf_line_pos(uint8_t n);
#endif

#define LOG_BUF_LEN 256

#ifdef LOG_USE_SSD1306
#define LOG_BUF_LINE_WIDTH SSD1306_LINE_WIDTH
#else
#define LOG_BUF_LINE_WIDTH 20
#endif

#endif
