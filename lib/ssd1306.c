// Based on https://github.com/SonalPinto/Arduino_SSD1306_OLED

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "config.h"
#include "i2c-master.h"
#include "ssd1306.h"
#include "ssd1306-cmd.h"

#include "logo-paw-64x64.h"

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

extern const uint8_t font[] PROGMEM;

static uint8_t console_current_col;
static uint8_t console_current_row;

#define CONSOLE_LINE_LENGTH 21

const uint8_t ssd1306_init_seq[] PROGMEM = {
    // Tell the SSD1306 that a command stream is incoming
    OLED_CONTROL_BYTE_CMD_STREAM,

    // Follow instructions on pg.64 of the dataSheet for software configuration of the SSD1306
    // Turn the Display OFF
    OLED_CMD_DISPLAY_OFF,
    // Set mux ration tp select max number of rows - 64
    OLED_CMD_SET_MUX_RATIO,
    0x3F,
    // Set the display offset to 0
    OLED_CMD_SET_DISPLAY_OFFSET,
    0x00,
    // Display start line to 0
    OLED_CMD_SET_DISPLAY_START_LINE,
  
    // Mirror the x-axis. In case you set it up such that the pins are north.
    // 0xA0, // - in case pins are south - default
    OLED_CMD_SET_SEGMENT_REMAP,
    
    // Mirror the y-axis. In case you set it up such that the pins are north.
    // 0xC0, // - in case pins are south - default
    OLED_CMD_SET_COM_SCAN_MODE,
    
    // Default - alternate COM pin map
    OLED_CMD_SET_COM_PIN_MAP,
    0x12,
    // set contrast
    OLED_CMD_SET_CONTRAST,
    0x00,
  
    // Set display to enable rendering from GDDRAM (Graphic Display Data RAM)
    OLED_CMD_DISPLAY_RAM,
    // Normal mode!
    OLED_CMD_DISPLAY_NORMAL,
    // Default oscillator clock
    OLED_CMD_SET_DISPLAY_CLK_DIV,
    0x80,
    // Enable the charge pump
    OLED_CMD_SET_CHARGE_PUMP,
    0x14,
    // Set precharge cycles to high cap type
    OLED_CMD_SET_PRECHARGE,
    0x22,
    // Set the V_COMH deselect volatage to max
    OLED_CMD_SET_VCOMH_DESELCT,
    0x30,
    // Horizonatal addressing mode - same as the KS108 GLCD
    OLED_CMD_SET_MEMORY_ADDR_MODE,
    0x00,
    // Turn the Display ON
    OLED_CMD_DISPLAY_ON,
};


struct ssd1306_frame_t
{
    uint8_t col_start;
    uint8_t col_end;

    uint8_t row_start;
    uint8_t row_end;

    uint8_t offset;
};

static const struct ssd1306_frame_t full_frame = { .col_start = 0x00, .col_end = 0x7F , .row_start = 0x00, .row_end = 0x07, .offset = 0x00 };

void ssd1306_init()
{
    // Begin the I2C comm with SSD1306's address (SLA+Write)
    i2c_start_wait(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_P(ssd1306_init_seq, sizeof(ssd1306_init_seq) / sizeof(ssd1306_init_seq[0]));
    i2c_stop();
}

static void ssd1306_setup_frame(const struct ssd1306_frame_t frame)
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_CMD_STREAM);
    i2c_write_byte(OLED_CMD_SET_COLUMN_RANGE);
    i2c_write_byte(frame.col_start);
    i2c_write_byte(frame.col_end);

    i2c_write_byte(OLED_CMD_SET_PAGE_RANGE);
    i2c_write_byte(frame.row_start);
    i2c_write_byte(frame.row_end);

    i2c_write_byte(OLED_CMD_SET_DISPLAY_OFFSET);
    i2c_write_byte(frame.offset);
    i2c_stop();
}


void ssd1306_bitmap_begin()
{
    ssd1306_setup_frame(full_frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
}



void ssd1306_bitmap_end()
{
    i2c_stop();
}


void ssd1306_bitmap_P(const uint8_t *buf, uint8_t cols, uint8_t rows, uint8_t x, uint8_t y)
{
    const struct ssd1306_frame_t frame = {
        .col_start = x,
        .col_end = x + cols - 1,
        .row_start = y,
        .row_end = y + rows - 1,
        .offset = 0x00
    };

    ssd1306_setup_frame(frame);
    
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(buf, (uint16_t)cols * rows);
    i2c_stop();
}

void ssd1306_bitmap_full_P(const uint8_t *buf)
{
    ssd1306_setup_frame(full_frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(buf, 1024);
    i2c_stop();
}

void ssd1306_clear()
{
    ssd1306_setup_frame(full_frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    for(uint16_t i=0; i < 1024; i++)
    {
	i2c_write_byte(0);
    }
    i2c_stop();
}

void ssd1306_splash()
{
    ssd1306_clear();

    const struct ssd1306_frame_t frame = { .col_start = 32, .col_end = 95 , .row_start = 0, .row_end = 7, .offset = 0 };

    ssd1306_setup_frame(frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(paw_64x64, (uint16_t)64 * 8);
  
    i2c_stop();

}

// Text

void ssd1306_text_putc(char c)
{
    i2c_write_P(&font[5*c], 5);
    i2c_write_byte(0);
}


static inline void print_spaces(uint8_t n)
{
    for(uint8_t i = 0; i < 6*n; i++)
    {
	i2c_write_byte(0);
    }
}

inline void ssd1306_text_start(uint8_t x, uint8_t y)
{
    const struct ssd1306_frame_t frame = { .col_start = x, .col_end = 0x7F , .row_start = y, .row_end = y, .offset = 0 };
    ssd1306_setup_frame(frame);

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
}

inline void ssd1306_text_end()
{
    i2c_stop();
}


void ssd1306_puts(const char* str, uint8_t x, uint8_t y)
{
    ssd1306_text_start(x, y);

    char c;
    while((c = *str++))
    {
        ssd1306_text_putc(c);
    }

    ssd1306_text_end();
}

void ssd1306_putc(const char c, uint8_t x, uint8_t y)
{
    ssd1306_text_start(x, y);
    
    ssd1306_text_putc(c);

    ssd1306_text_end();
}

// Console

void ssd1306_console_init()
{
    ssd1306_clear();

    console_current_row = 7;
    console_current_col = 0;
}

static void console_line_start()
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

static void console_line_end()
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
