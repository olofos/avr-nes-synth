/************************************************************************
 * Main controller
 *
 * The main playing loop reads data from the SD card into two circular
 * buffer (one with addresses and one with data values), and handles
 * user input
 *
 * Playback is performed using two timers: 
 *
 * - Timer 1 fires at 240 Hz
 *   and outputs the frame clock. At every four frames it starts timer 2
 *   to output data
 *
 * - Timer 2 fires at around 60 kHz. It outputs the data from the address 
 *   and value buffers to the bus, and stops itself once it finds the end 
 *   of frame marker
 ************************************************************************/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <stdlib.h>

#include "config.h"
#include "io.h"

#include "i2c-master.h"
#include "ssd1306.h"
#include "sd.h"
#include "fat32.h"
#include "log.h"

#include "cbuf.h"

#define BUTTON_PRESS_RIGHT 0x80
#define BUTTON_PRESS_UP    0x40
#define BUTTON_PRESS_DOWN  0x20
#define BUTTON_PRESS_LEFT  0x10

#define BUTTON_PRESS_MASK  0xF0


#define reg_address_LEN 128
#define reg_data_LEN 128

struct
{
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t buf[reg_address_LEN];
} reg_address;

struct
{
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t buf[reg_data_LEN];
} reg_data;

////////////////////////////////////////////////////////////////////////////////

static inline void channel_reset_hold();
static inline void channel_reset_release();

void io_init();
void spi_init();
void timers_init();

static inline void timer0_start();
static inline void timer0_stop();
static inline void timer2_start();
static inline void timer2_stop();

void error_led_loop();

void i2c_scan();

void clear_inputs();
uint8_t get_input();


////////////////////////////////////////////////////////////////////////////////

void bitmap(const char* filename);

////////////////////////////////////////////////////////////////////////////////

volatile uint8_t song_done;

void reset_channels();
uint8_t song_play(const char* filename);
uint8_t song_handle_inputs();

void song_open(const char* filename);
void song_stop();

void song_read_data();

#define SONG_STOP 0x01
#define SONG_NEXT 0x02
#define SONG_PREV 0x04

////////////////////////////////////////////////////////////////////////////////

struct menu_item_t
{
    char string[21];
};

struct menu_item_t menu[8];

static int8_t menu_current_option;
static uint8_t menu_top;
static uint8_t menu_num_items;

#define MENU_HEIGHT 8
#define MENU_REDRAW 0x01
#define MENU_PLAY   0x02

void menu_init();
void menu_loop();

void menu_redraw();
uint8_t menu_handle_input();
void menu_reload(uint8_t start);

void menu_read_info(uint8_t start, char *str, uint8_t *len);

////////////////////////////////////////////////////////////////////////////////


static inline void channel_reset_hold()
{
    disable_pullup(PIN_CH_RESET);
    set_output(PIN_CH_RESET);
    set_low(PIN_CH_RESET);
}

static inline void channel_reset_release()
{
    set_input(PIN_CH_RESET);
    enable_pullup(PIN_CH_RESET);
}


void io_init()
{
    set_outputs(PINS_BUS);
    set_output(PIN_DCLK);
    set_output(PIN_FCLK);

    set_low(PIN_DCLK);
    set_low(PIN_FCLK);

    set_output(PIN_MOSI);
    set_input(PIN_MISO);
    set_output(PIN_SCK);
    set_output(PIN_SD_CS);

    set_high(PIN_SD_CS);

    set_input(PIN_SD_CD);
    enable_pullup(PIN_SD_CD);

    set_input(PIN_IO_INT);
    set_output(PIN_LED);
}

void spi_init()
{
  // Enable SPI as master, MSB first, clock rate f_osc/16
    SPCR |= _BV(MSTR) | _BV(SPE) |  (0 << SPR1) | (1 << SPR0);

  // Clear double speed
  SPSR &= ~(1 << SPI2X);

  log_puts_P(PSTR("SPI initialised\n"));
}



void i2c_scan()
{
    log_puts("Scanning for I2C devices\n");

    uint8_t num = 0;

    for(uint8_t address = 0; address < 0x80; address++)
    {	
	uint8_t status = i2c_start(address, I2C_WRITE);

	if(status == I2C_OK)
	{
            i2c_stop();
            
	    log_puts("Device found at 0x");
            log_put_uint8_hex(address);
	    log_puts("\n");

	    num++;
	}
    }

    if(!num)
    {
	log_puts("No devices found\n");
    }
}



void bitmap(const char* filename)
{
    log_puts("Opening \"");
    log_puts(filename);
    log_puts("\"\n");
    
    fat32_open_root_dir();
    fat32_open_file(filename);

    uint8_t buf[128];

    ssd1306_bitmap_begin();
    
    while(fat32_read(&buf, 128))
    {
	for(uint8_t j = 0; j < 16; j++)
	{
	    for(uint8_t i = 0; i < 8; i++)
	    {
		uint8_t b0 = (buf[j +   0] & _BV(7-i)) ? 1 : 0;
		uint8_t b1 = (buf[j +  16] & _BV(7-i)) ? 1 : 0;
		uint8_t b2 = (buf[j +  32] & _BV(7-i)) ? 1 : 0;
		uint8_t b3 = (buf[j +  48] & _BV(7-i)) ? 1 : 0;
		uint8_t b4 = (buf[j +  64] & _BV(7-i)) ? 1 : 0;
		uint8_t b5 = (buf[j +  80] & _BV(7-i)) ? 1 : 0;
		uint8_t b6 = (buf[j +  96] & _BV(7-i)) ? 1 : 0;
		uint8_t b7 = (buf[j + 112] & _BV(7-i)) ? 1 : 0;
	    
		i2c_write_byte((b7 << 7) | (b6 << 6) | (b5 << 5) | (b4 << 4) | (b3 << 3) | (b2 << 2) | (b1 << 1) | (b0 << 0));
	    }
	}
        _delay_ms(1);
    }
    ssd1306_bitmap_end();
    fat32_close_file();
}


static inline void timer0_start()
{
    TCCR0B |= _BV(CS02); // Prescaler 256
}

static inline void timer0_stop()
{
    TCCR0B &= ~_BV(CS02);
}

static inline void timer2_start()
{
    TCCR2B |= _BV(CS21); // Prescaler 8
}

static inline void timer2_stop()
{
    TCCR2B &= ~_BV(CS21);
}

void timers_init()
{
    TCCR0A = _BV(WGM01); // CTC mode
    TCCR0B = 0;
    TIMSK0 |= _BV(OCIE0A); // Set interrupt on compare match
    OCR0A = 232; // 14318180 Hz / 256 / (232 + 1) = 240 Hz

    TCCR2A = _BV(WGM21); // CTC mode
    TCCR2B = 0;
    TIMSK2 |= _BV(OCIE2A); // Set interrupt on compare match
    OCR2A = 59; // Interrupt every ~ 33 us
}

void song_open(const char* filename)
{
    fat32_open_root_dir();
    
    if(fat32_open_file(filename))
    {
        song_done = 0;

        log_puts("Playing \"");
        log_puts(filename);
        log_puts("\"\n");

        timer0_start();
    } else {
        log_puts("Could not open file \"");
        log_puts(filename);
        log_puts("\"\n");

        error_led_loop();
    }
}

void song_stop()
{
    timer0_stop();
    fat32_close_file();
}


void error_led_loop()
{
    for(;;)
    {
        set_high(PIN_LED);
        _delay_ms(125);
        set_low(PIN_LED);
        _delay_ms(125);

        set_high(PIN_LED);
        _delay_ms(125);
        set_low(PIN_LED);
        _delay_ms(125);

        set_high(PIN_LED);
        _delay_ms(125);
        set_low(PIN_LED);
        
        _delay_ms(375);
    }
}

void song_read_data()
{   
    while(!cbuf_full(reg_data))
    {
        uint8_t data[2];

        fat32_read(data, 2); // TODO: read more than two bytes at a time!

        if(cbuf_empty(reg_data))
        {
            set_high(PIN_LED);
        }


        if(data[0] == 0xFE)
        {
            fat32_read(data, 2); // TODO: read more than two bytes at a time!
            uint16_t dest = ((uint16_t)data[0]<<8) | data[1];
            log_puts("Loop to 0x");
            log_put_uint16_hex(dest);
            log_puts("\n");
            fat32_seek(dest);
        } else {
            cbuf_push(reg_address, data[0]);
            cbuf_push(reg_data, data[1]);
        }
    }
}

void reset_channels()
{
    cli();
    cbuf_init(reg_address);
    cbuf_init(reg_data);

    for(uint8_t n = 0; n < 17; n++)
    {
        cbuf_push(reg_address, n);
        cbuf_push(reg_data, 0);
    }

    sei();

    timer2_start();
}

uint8_t song_play(const char* filename)
{
    song_open(filename);
    song_read_data();
    set_low(PIN_LED);

    uint8_t ret;
    
    while(!(ret = song_handle_inputs()) && !song_done)
    {
        song_read_data();
    }

    song_stop();
    reset_channels();

    if(!ret)
    {
        ret = SONG_NEXT;
    }
    
    return ret;
}

void clear_inputs()
{
    // Why is this needed???
    i2c_start(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_stop();

    i2c_start(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
    i2c_read_nak();
    i2c_stop();
}

uint8_t song_handle_inputs()
{
    uint8_t action = 0;
    switch(get_input())
    {
    case BUTTON_PRESS_LEFT:
        action = SONG_PREV;
        break;

    case BUTTON_PRESS_RIGHT:
        action = SONG_NEXT;
        break;

    case BUTTON_PRESS_UP:
    case BUTTON_PRESS_DOWN:
        action = SONG_STOP;
        break;
    }
    return action;
}

void category_play(const char catname[6], uint8_t len)
{
    char filename[12];
    for(uint8_t i = 0; i < 6; i++)
    {
        filename[i] = catname[i];
    }

    filename[8] = 'B';
    filename[9] = 'I';
    filename[10] = 'N';
    filename[11] = 0;
    

    uint8_t done = 0;
    int8_t num = 1;

    while(!done)
    {
        filename[6] = '0' + (num / 10);
        filename[7] = '0' + (num % 10);

        char track[] = "Track 67/9A";


        if(len < 10)
        {
            track[6] = '0' + num;
            track[7] = '/';
            track[8] = '0' + len;
            track[9] = ' ';
            track[10] = ' ';
        } else {
            if(num < 10)
            {
                track[6] = ' ';
            } else {
                track[6] = '0' + (num/10);
            }
            track[7] = '0' + (num%10);

            track[9] = '0' + (len/10);
            track[10] = '0' + (len%10);
        }

        ssd1306_puts(track, 0, 2);


        uint8_t ret = song_play(filename);

        switch(ret)
        {
        case SONG_NEXT:
            num++;
            if(num > len)
            {
                num = 1;
            }
            break;

        case SONG_PREV:
            num--;
            if(num <= 0)
            {
                num = len;
            }
            break;

        case SONG_STOP:
            done = 1;
            break;
        }
    }
}


uint8_t get_input()
{
    uint8_t in = 0;
    if(is_high(PIN_IO_INT))
    {

        i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
        in = i2c_read_nak();
        i2c_stop();
    }
    return in;
}

void menu_init()
{
    fat32_open_root_dir();
    fat32_open_file("CAT     TXT");

    char numstr[2];
    fat32_read(numstr, 2);
    fat32_read(0, 1); // Skip new line

    menu_num_items = 10*(numstr[0] - '0') + (numstr[1] - '0');

    log_puts("Number of menu items: ");
    log_put_uint8(menu_num_items);
    log_nl();

    for(uint8_t i = 0; i < MENU_HEIGHT; i++)
    {
        if(i < menu_num_items)
        {
            fat32_read(menu[i].string, 20);
            menu[i].string[20] = 0;

            fat32_read(0, 10); // Slip category (6) + num items (3) + nl (1)
        } else {
            menu[i].string[0] = 0;
        }
    }

    fat32_close_file();

    menu_top = 0;
    
    ssd1306_clear();
}

void menu_reload(uint8_t start)
{
    fat32_open_root_dir();
    fat32_open_file("CAT     TXT");

    fat32_read(0, 3); // Skip num cat + new line
    fat32_read(0, 30 * start);

    for(uint8_t i = 0; i < MENU_HEIGHT; i++)
    {
        if(i < menu_num_items)
        {
            fat32_read(menu[i].string, 20);
            menu[i].string[20] = 0;

            fat32_read(0, 10); // Skip category (6) + num items (3) + nl (1)
        } else {
            menu[i].string[0] = 0;
        }
    }

    fat32_close_file();
}

void menu_read_info(uint8_t start, char *str, uint8_t *len)
{
    fat32_open_root_dir();
    fat32_open_file("CAT     TXT");

    fat32_read(0, 3); // Skip num cat + new line
    fat32_read(0, 30 * start);
    fat32_read(0, 20); // Skip name

    fat32_read(str, 6);

    char numstr[3];
    fat32_read(numstr, 3);

    *len = (numstr[1]-'0')*10 + (numstr[2]-'0');

    fat32_close_file();
}

void menu_redraw()
{
    for(uint8_t i = 0; i < MENU_HEIGHT; i++)
    {
        if(menu_current_option == menu_top + i)
        {
            ssd1306_puts("\x10", 0, i);
        } else {
            ssd1306_puts(" ", 0, i);
        }

        ssd1306_puts(menu[i].string, 8, i);
    }
}

uint8_t menu_handle_input()
{
    uint8_t action = 0;
    
    switch(get_input())
    {
    case BUTTON_PRESS_UP:
        menu_current_option--;
        if(menu_current_option < 0)
        {
            menu_current_option = 0;
        }
        if(menu_current_option < menu_top)
        {
            menu_top--;
            menu_reload(menu_top);
        }
        action = MENU_REDRAW;
        break;

    case BUTTON_PRESS_DOWN:
        menu_current_option++;
        if(menu_current_option > menu_num_items-1)
        {
            menu_current_option = menu_num_items-1;
        }
        if(menu_current_option > menu_top + MENU_HEIGHT - 1)
        {
            menu_top++;
            menu_reload(menu_top);
        }
        action = MENU_REDRAW;
        break;

    case BUTTON_PRESS_RIGHT:
    case BUTTON_PRESS_LEFT:
        action = MENU_PLAY;
        break;

    default:
        break;
    }

    return action;
}

void menu_loop()
{
    menu_redraw();
    
    for(;;)
    {
        uint8_t action = menu_handle_input();

        if(action == MENU_REDRAW)
        {
            menu_redraw();
        } else if(action == MENU_PLAY) {
            char catname[6];
            uint8_t num;

            menu_read_info(menu_current_option, catname, &num);
            
            log_puts("Play: \"");
            for(uint8_t i = 0; i < 6; i++)
            {
                log_putc(catname[i]);
            }
            log_puts("\"\n");
            ssd1306_clear();

            ssd1306_puts("Playing:",0,0);
            ssd1306_puts(menu[menu_current_option-menu_top].string,0,1);
            
            category_play(catname, num);

            menu_redraw();
        }
    }
}





int main()
{
    io_init();
    i2c_master_init();

    timers_init();

    cbuf_init(reg_address);
    cbuf_init(reg_data);

    sei();

    ssd1306_init();
    ssd1306_splash();

    log_init(LOG_I2C);
    spi_init();

    i2c_scan();

    if(is_high(PIN_SD_CD))
    {
        log_puts_P(PSTR("SD card detected\n"));
    } else {
        log_puts_P(PSTR("No SD card detected\n"));

        error_led_loop();
    }
    sd_init();
    fat32_init();

//    clear_inputs();

    reset_channels();

    menu_init();
    menu_loop();
}

uint8_t frame_counter;

ISR(TIMER0_COMPA_vect) // Frame clock
{
    uint8_t cnt = frame_counter + 1;
    set_pin(PIN_FCLK, cnt & 1);

    if(!(cnt & 0x03))
    {
        // Next frame
        if(!song_done)
        {
            timer2_start();
        }
    }

    frame_counter = cnt;
}

ISR(TIMER2_COMPA_vect) // Push out new data
{
    static uint8_t n = 0;

    if(cbuf_empty(reg_data))
    {
        timer2_stop();
    } else {
        if(!(n++ & 0x01))
        {
            uint8_t address = cbuf_pop(reg_address);
            if(address == 0xFF)
            {
                timer2_stop();
                cbuf_pop(reg_data);
                n++;
                song_done = 1;
            } else if(address == 0xF1) {
                timer2_stop();
                cbuf_pop(reg_data);
                n++;
            } else {
                PINS_BUS_PORT = address;
                set_high(PIN_DCLK);
            }
        } else {
            uint8_t value = cbuf_pop(reg_data);
            PINS_BUS_PORT = value;
            set_low(PIN_DCLK);
        }
    }
}
