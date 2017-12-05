// Console

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "config.h"
#include "i2c-master.h"
#include "ssd1306.h"
#include "ssd1306-cmd.h"
#include "ssd1306-internal.h"


static uint8_t console_current_col;
static uint8_t console_current_row;

#define CONSOLE_LINE_LENGTH 21


void ssd1306_console_init(void)
{
    ssd1306_clear();

    console_current_row = 7;
    console_current_col = 0;
}

static void console_line_start(void)
{
    const struct ssd1306_frame_t frame = {
        .col_start = 0x01 + console_current_col * 6,
        .col_end = 0x7F ,
        .row_start = console_current_row,
        .row_end = console_current_row,
        .offset = (console_current_row + 1) * 8
    };

    ssd1306_setup_frame(frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
}

static inline void print_spaces(uint8_t n)
{
    for(uint8_t i = 0; i < 6*n; i++)
    {
        i2c_write_byte(0);
    }
}

static void console_line_end(void)
{
    print_spaces(CONSOLE_LINE_LENGTH - console_current_col);
    i2c_stop();
}

static void console_line(const char *str, uint8_t len)
{
    console_line_start();

    for(uint8_t i = 0; i < len; i++)
    {
        ssd1306_text_putc(str[i]);
    }

    console_current_col += len;

    console_line_end();
}


static void console_line_P(const char *str, uint8_t len)
{
    console_line_start();

    for(uint8_t i = 0; i < len; i++)
    {
        ssd1306_text_putc(pgm_read_byte(&str[i]));
    }

    console_current_col += len;

    console_line_end();
}

void ssd1306_console_putc(const char c)
{
    char str[2];
    str[0] = c;
    str[1] = 0;
    ssd1306_console_puts(str);
}

void ssd1306_console_puts(const char* str)
{
    while(*str)
    {
        uint8_t len = 0;

        while(str[len] && (str[len] != '\n') && (len < CONSOLE_LINE_LENGTH - console_current_col))
        {
            len++;
        }

        console_line(str, len);

        str += len;

        if(*str == '\n')
        {
            str++;
            console_current_row++;
            console_current_col=0;
        } else if(console_current_col >= CONSOLE_LINE_LENGTH)
        {
            console_current_row++;
            console_current_col = 0;
        }
    }
}


void ssd1306_console_puts_P(const char* str)
{
    while(pgm_read_byte(str))
    {
        uint8_t len = 0;
        uint8_t c;
        while((c = pgm_read_byte(&str[len])) && (c != '\n') && (len < CONSOLE_LINE_LENGTH - console_current_col))
        {
            len++;
        }

        console_line_P(str, len);

        str += len;

        if(c == '\n')
        {
            str++;
            console_current_row++;
            console_current_col=0;
        } else {
            if(console_current_col >= CONSOLE_LINE_LENGTH)
            {
                console_current_row++;
                console_current_col = 0;
            }
        }
    }
}
