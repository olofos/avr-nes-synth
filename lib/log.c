#include <avr/io.h>
#include <stdlib.h>

#include "config.h"

#ifdef LOG_USE_PRINTF
#include <stdarg.h>
#endif

#include "log.h"

#ifdef LOG_USE_UART
#include "uart.h"
#endif

#ifdef LOG_USE_SSD1306
#include "ssd1306.h"
#endif

#ifdef LOG_USE_I2C
#include "i2c-master.h"

void log_i2c_putc(const char c)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(0xFF);
    i2c_write_byte(c);
    i2c_stop();
}

void log_i2c_puts(const char *str)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(0xFF);

    while(*str)
    {
        i2c_write_byte(*str++);
    }

    i2c_stop();
}

void log_i2c_puts_P(const char *str)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(0xFF);

    uint8_t c;

    while((c = pgm_read_byte(str++)))
    {
        i2c_write_byte(c);
    }

    i2c_stop();
}

#endif

#ifdef LOG_USE_BUF

char log_buf[LOG_BUF_LEN];
uint8_t log_buf_pos;
uint8_t log_buf_line_len;

void log_buf_init()
{
    log_buf_pos = 0;
    log_buf_line_len = 0;
}

void log_buf_putc(const char c)
{
    if(c)
    {
        log_buf[log_buf_pos++] = c;

        if(c == '\n')
        {
            log_buf_line_len = 0;
        } else {
            log_buf_line_len++;
            if(log_buf_line_len >= LOG_BUF_LINE_WIDTH)
            {
                log_buf[log_buf_pos++] = '\n';
                log_buf_line_len = 0;
            }
        }
    }
}

uint8_t log_buf_count_lines()
{
    uint8_t pos = log_buf_pos;
    uint8_t len = LOG_BUF_LEN-1;
    uint8_t n = 1;

    while(!log_buf[pos])
    {
        len--;
        pos++;
    }

    for(uint8_t i = 0; i < len; i++)
    {
        if(log_buf[pos] == '\n')
        {
            n++;
        }
        pos++;
    }

    return n;
}

uint8_t log_buf_line_pos(uint8_t n)
{
    uint8_t pos = log_buf_pos;
    uint8_t len = LOG_BUF_LEN-1;
    uint8_t m = 0;

    while(!log_buf[pos])
    {
        pos++;
        len--;
    }

    for(uint8_t i = 0; i < len; i++)
    {
        if(n == m)
        {
            break;
        }

        if(log_buf[pos] == '\n')
        {
            m++;
        }
        pos++;
    }

    return pos;
}


#endif

static uint8_t log_mask;

void log_init(uint8_t mask)
{
    log_enable(mask);
}

void log_enable(uint8_t mask)
{
    log_mask |= mask;

#ifdef LOG_USE_UART
    if(mask & LOG_UART)
    {
    }

    if(mask & LOG_UART)
    {
	log_puts("Enabling log to UART\n");
    }

#endif

#ifdef LOG_USE_SSD1306
    if(mask & LOG_SSD1306)
    {
	ssd1306_console_init();
    }

    if(mask & LOG_SSD1306)
    {
	log_puts("Enabling log to SSD1306\n");
    }

#endif

#ifdef LOG_USE_I2C
    if(mask & LOG_I2C)
    {
    }

    if(mask & LOG_I2C)
    {
	log_puts("Enabling log to I2C\n");
    }
#endif

#ifdef LOG_USE_BUF
    if(mask & LOG_BUF)
    {
        log_buf_init();
    }

    if(mask & LOG_BUF)
    {
        log_puts("Enabling log to circular buffer\n");
    }
#endif
}

void log_disable(uint8_t mask)
{
#ifdef LOG_UART
    if(mask & LOG_UART)
    {
	log_puts("Disabling log to UART\n");
    }
#endif

#ifdef LOG_USE_SSD1306
    if(mask & LOG_SSD1306)
    {
	log_puts("Disabling log to OLED\n");
    }
#endif

#ifdef LOG_USE_I2C
    if(mask & LOG_I2C)
    {
	log_puts("Disabling log to I2C\n");
    }
#endif

#ifdef LOG_USE_BUF
    if(mask & LOG_BUF)
    {
        log_puts("Disabling log to circular buffer\n");
    }
#endif

    log_mask &= ~mask;
}

void log_putc(const char c)
{
#ifdef LOG_USE_UART
    if(log_mask & LOG_UART)
    {
	uart_putc(c);
    }
#endif

#ifdef LOG_USE_SSD1306
    if(log_mask & LOG_SSD1306)
    {
	ssd1306_console_putc(c);
    }
#endif

#ifdef LOG_USE_I2C
    if(log_mask & LOG_I2C)
    {
        log_i2c_putc(c);
    }
#endif

#ifdef LOG_USE_BUF
    if(log_mask & LOG_BUF)
    {
        log_buf_putc(c);
    }
#endif
}

void log_puts(const char* str)
{
#ifdef LOG_USE_UART
    if(log_mask & LOG_UART)
    {
	uart_puts(str);
    }
#endif

#ifdef LOG_USE_SSD1306
    if(log_mask & LOG_SSD1306)
    {
	ssd1306_console_puts(str);
    }
#endif

#ifdef LOG_USE_I2C
    if(log_mask & LOG_I2C)
    {
        log_i2c_puts(str);
    }
#endif

#ifdef LOG_USE_BUF
    if(log_mask & LOG_BUF)
    {
        for(uint8_t i = 0; str[i]; i++)
        {
            log_buf_putc(str[i]);
        }
    }
#endif
}

void log_puts_P(const char* str)
{
#ifdef LOG_USE_UART
    if(log_mask & LOG_UART)
    {
	uart_puts_P(str);
    }
#endif

#ifdef LOG_USE_SSD1306
    if(log_mask & LOG_SSD1306)
    {
	ssd1306_console_puts_P(str);
    }
#endif

#ifdef LOG_USE_I2C
    if(log_mask & LOG_I2C)
    {
        log_i2c_puts_P(str);
    }
#endif

#ifdef LOG_USE_BUF
    if(log_mask & LOG_BUF)
    {
        char c;
        while((c = pgm_read_byte(str++)))
        {
            log_buf_putc(c);
        }
    }
#endif

}



#ifdef LOG_USE_PRINTF

int vsnprintf (char *__s, size_t __n, const char *__fmt, va_list ap);


void log_printf(const char *fmt, ... )
{
    char buf[128]; // resulting string limited to 128 chars
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    log_puts(buf);
}
#endif

void log_put_uint8_hex(uint8_t val)
{
    if(val < 0x10)
    {
        log_puts("0");
    }

    char buf[16];
    itoa(val, buf, 16);
    log_puts(buf);
}

void log_put_uint16_hex(uint16_t val)
{
    log_put_uint8_hex((uint8_t) (val >> 8));
    log_put_uint8_hex((uint8_t) (val & 0xFF));
}

void log_put_uint32_hex(uint32_t val)
{
    log_put_uint8_hex((uint8_t) (val >> 24));
    log_put_uint8_hex((uint8_t) (val >> 16));
    log_put_uint8_hex((uint8_t) (val >> 8));
    log_put_uint8_hex((uint8_t) (val & 0xFF));
}

void log_put_uint8(uint8_t val)
{
    char buf[4];

    if(val < 100)
      log_putc(' ');
    if(val < 10)
      log_putc(' ');

    utoa(val, buf, 10);

    log_puts(buf);
}

void log_put_uint16(uint16_t val)
{
    char buf[6];

    if(val < 10000)
      log_putc(' ');
    if(val < 1000)
      log_putc(' ');
    if(val < 100)
      log_putc(' ');
    if(val < 10)
      log_putc(' ');

    utoa(val, buf, 10);
    log_puts(buf);
}

void log_put_int8(int8_t val)
{
    char buf[4];
    uint8_t neg = 0;

    if(val < 0)
    {
        neg = 1;
        val = -val;
    }

    if(val < 100)
      log_putc(' ');
    if(val < 10)
      log_putc(' ');

    if(neg)
    {
      log_putc('-');
    } else {
      log_putc(' ');
    }

    utoa(val, buf, 10);

    log_puts(buf);
}

void log_put_int16(int8_t val)
{
    char buf[6];
    uint8_t neg = 0;

    if(val < 0)
    {
        neg = 1;
        val = -val;
    }

    if(val < 10000)
      log_putc(' ');
    if(val < 1000)
      log_putc(' ');
    if(val < 100)
      log_putc(' ');
    if(val < 10)
      log_putc(' ');

    if(neg)
    {
      log_putc('-');
    } else {
      log_putc(' ');
    }

    utoa(val, buf, 10);

    log_puts(buf);
}


void log_put_ascii(uint8_t val)
{
    if((val >= 0x20) && (val <= 0x7E))
    {
        log_putc(val);
    } else {
        log_putc('.');
    }
}
