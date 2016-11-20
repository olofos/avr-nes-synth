// Based on https://github.com/SonalPinto/Arduino_SSD1306_OLED

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "config.h"
#include "i2c-master.h"
#include "ssd1306.h"

#include "logo-paw-64x64.h"


//~ DEFINES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Some defines for the SSD1306 controller driving a 128x64 resolution OLED display
// PART     - http://www.simplelabs.co.in/content/96-blue-i2c-oled-module
// DATASHEET  - https://www.adafruit.com/datasheets/SSD1306.pdf

// The Slave Address (SLA) of the OLED controller - SSD1306 - is 0x3C 
// The LSB is supposed to be the mode. Since we are only going to WRITE to the OLED, the LSB is going to be 0
// SLA (0x3C) + WRITE_MODE (0x00) =  0x78 (0b01111000)
#define OLED_I2C_ADDRESS   0x3C

// The SSD1306 datasheet (pg.20) says that a control byte has to be sent before sending a command
// Control byte consists of 
// bit 7    : Co   : Continuation bit - If 0, then it assumes all the next bytes are data (no more control bytes).
//        :    You can send a stream of data, ie: gRAM dump - if Co=0
//        :        For Command, you'd prolly wanna set this - one at a time. Hence, Co=1 for commands
//        :    For Data stream, Co=0 :)
// bit 6      : D/C# : Data/Command Selection bit, Data=1/Command=0
// bit [5-0]  : lower 6 bits have to be 0
#define OLED_CONTROL_BYTE_CMD_SINGLE  0x80
#define OLED_CONTROL_BYTE_CMD_STREAM  0x00
#define OLED_CONTROL_BYTE_DATA_STREAM 0x40

// Fundamental commands (pg.28)
#define OLED_CMD_SET_CONTRAST     0x81  // follow with 0x7F
#define OLED_CMD_DISPLAY_RAM      0xA4
#define OLED_CMD_DISPLAY_ALLON      0xA5
#define OLED_CMD_DISPLAY_NORMAL     0xA6
#define OLED_CMD_DISPLAY_INVERTED     0xA7
#define OLED_CMD_DISPLAY_OFF      0xAE
#define OLED_CMD_DISPLAY_ON       0xAF

// Addressing Command Table (pg.30)
#define OLED_CMD_SET_MEMORY_ADDR_MODE 0x20  // follow with 0x00 = HORZ mode = Behave like a KS108 graphic LCD
#define OLED_CMD_SET_COLUMN_RANGE   0x21  // can be used only in HORZ/VERT mode - follow with 0x00 + 0x7F = COL127
#define OLED_CMD_SET_PAGE_RANGE     0x22  // can be used only in HORZ/VERT mode - follow with 0x00 + 0x07 = PAGE7

// Hardware Config (pg.31)
#define OLED_CMD_SET_DISPLAY_START_LINE 0x40
#define OLED_CMD_SET_SEGMENT_REMAP    0xA1  
#define OLED_CMD_SET_MUX_RATIO      0xA8  // follow with 0x3F = 64 MUX
#define OLED_CMD_SET_COM_SCAN_MODE    0xC8  
#define OLED_CMD_SET_DISPLAY_OFFSET   0xD3  // follow with 0x00
#define OLED_CMD_SET_COM_PIN_MAP    0xDA  // follow with 0x12
#define OLED_CMD_NOP          0xE3  // NOP

// Timing and Driving Scheme (pg.32)
#define OLED_CMD_SET_DISPLAY_CLK_DIV  0xD5  // follow with 0x80
#define OLED_CMD_SET_PRECHARGE      0xD9  // follow with 0xF1
#define OLED_CMD_SET_VCOMH_DESELCT    0xDB  // follow with 0x30

// Charge Pump (pg.62)
#define OLED_CMD_SET_CHARGE_PUMP    0x8D  // follow with 0x14


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
    // 0xC0, - in case pins are south - default
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

#define CONSOLE_LINE_LENGTH 21


void ssd1306_init()
{
    // Begin the I2C comm with SSD1306's address (SLA+Write)
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_P(ssd1306_init_seq, sizeof(ssd1306_init_seq) / sizeof(ssd1306_init_seq[0]));
    i2c_stop();
}

const uint8_t ssd1306_begin_full_frame_seq[] PROGMEM =
{
    OLED_CONTROL_BYTE_CMD_STREAM,
    // column 0 to 127
    OLED_CMD_SET_COLUMN_RANGE,
    0x00,
    0x7F,
    // page 0 to 7
    OLED_CMD_SET_PAGE_RANGE,
    0x00,
    0x07,
    OLED_CMD_SET_DISPLAY_OFFSET,
    0
};

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

extern const uint8_t font[] PROGMEM;

static uint8_t current_col;
static uint8_t current_row;

void ssd1306_console_init()
{
    ssd1306_clear();

    current_row = 7;
    current_col = 0;

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_CMD_STREAM);
    i2c_write_byte(OLED_CMD_SET_COLUMN_RANGE);
    i2c_write_byte(0x01);
    i2c_write_byte(0x7E);
    i2c_write_byte(OLED_CMD_SET_PAGE_RANGE);
    i2c_write_byte(0x00);
    i2c_write_byte(0x07);

    i2c_write_byte(OLED_CMD_SET_DISPLAY_OFFSET);
    i2c_write_byte(0);
    i2c_stop();

}

static inline void print_char(uint8_t c)
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

static void print_line_start()
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_CMD_STREAM);
    i2c_write_byte(OLED_CMD_SET_COLUMN_RANGE);
    i2c_write_byte(0x01 + current_col * 6);
    i2c_write_byte(0x7F);

    i2c_write_byte(OLED_CMD_SET_PAGE_RANGE);
    i2c_write_byte(current_row);
    i2c_write_byte(current_row);

    i2c_write_byte(OLED_CMD_SET_DISPLAY_OFFSET);
    i2c_write_byte((current_row + 1) * 8);

    i2c_stop();


    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
}

static void print_line_end()
{
    print_spaces(CONSOLE_LINE_LENGTH - current_col);
    i2c_stop();
}

static void print_line(const char *str, uint8_t len)
{   
    for(uint8_t i = 0; i < len; i++)
    {
	print_char(str[i]);
    }

    current_col += len;
}


static void print_line_P(const char *str, uint8_t len)
{   
    for(uint8_t i = 0; i < len; i++)
    {
	print_char(pgm_read_byte(&str[i]));
    }

    current_col += len;
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

	while(str[len] && (str[len] != '\n') && (len < CONSOLE_LINE_LENGTH - current_col))
	{
	    len++;
	}

	print_line_start();
	print_line(str, len);
	print_line_end();
	
	str += len;
	
	if(*str == '\n')
	{
	    str++;
	    current_row++;
	    current_col=0;
	} else {
	    if(current_col >= CONSOLE_LINE_LENGTH)
	    {
		current_row++;
		current_col = 0;
	    }
	}
    }    
}


void ssd1306_console_puts_P(const char* str)
{
    while(pgm_read_byte(str))
    {
	uint8_t len = 0;
	uint8_t c;
	while((c = pgm_read_byte(&str[len])) && (c != '\n') && (len < CONSOLE_LINE_LENGTH - current_col))
	{
	    len++;
	}

	print_line_start();
	print_line_P(str, len);
	print_line_end();
	
	str += len;
	
	if(c == '\n')
	{
	    str++;
	    current_row++;
	    current_col=0;
	} else {
	    if(current_col >= CONSOLE_LINE_LENGTH)
	    {
		current_row++;
		current_col = 0;
	    }
	}
    }    
}

void ssd1306_bitmap_begin()
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_P(ssd1306_begin_full_frame_seq, ARRAY_SIZE(ssd1306_begin_full_frame_seq));

    // FLIP X AND Y AXIS
    i2c_write_byte(OLED_CMD_SET_COM_SCAN_MODE-8);
    i2c_write_byte(OLED_CMD_SET_SEGMENT_REMAP-1);

    i2c_stop();

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
}



void ssd1306_bitmap_end()
{
    i2c_stop();
}


void ssd1306_bitmap_P(const uint8_t *buf, uint8_t cols, uint8_t rows, uint8_t x, uint8_t y)
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_CMD_STREAM);
    i2c_write_byte(OLED_CMD_SET_COLUMN_RANGE);
    i2c_write_byte(x);
    i2c_write_byte(x+cols-1);

    i2c_write_byte(OLED_CMD_SET_PAGE_RANGE);
    i2c_write_byte(y);
    i2c_write_byte(y+rows-1);

    i2c_write_byte(OLED_CMD_SET_DISPLAY_OFFSET);
    i2c_write_byte(0);
    i2c_stop();

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(buf, (uint16_t)cols * rows);
    i2c_stop();
}


void ssd1306_bitmap_full_P(const uint8_t *buf)
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_P(ssd1306_begin_full_frame_seq, ARRAY_SIZE(ssd1306_begin_full_frame_seq));
    i2c_stop();

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(buf, 1024);
    i2c_stop();
}

void ssd1306_clear()
{
    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_P(ssd1306_begin_full_frame_seq, ARRAY_SIZE(ssd1306_begin_full_frame_seq));
    i2c_stop();

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

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_CMD_STREAM);
    i2c_write_byte(OLED_CMD_SET_COLUMN_RANGE);
    i2c_write_byte(32);
    i2c_write_byte(95);

    i2c_write_byte(OLED_CMD_SET_PAGE_RANGE);
    i2c_write_byte(0);
    i2c_write_byte(7);

    i2c_write_byte(OLED_CMD_SET_DISPLAY_OFFSET);
    i2c_write_byte(0);
    i2c_stop();

    i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
    i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
    i2c_write_P(paw_64x64, (uint16_t)64 * 8);
  
    i2c_stop();

}
