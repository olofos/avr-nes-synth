/************************************************************************
 * Main controller
 *
 * The main playing loop reads data from the SD card into two circular
 * buffer (one with addresses and one with data values), and handles
 * user input
 *
 * Playback is performed using two timers: 
 *
 * - Timer 0 fires at 240 Hz
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
#include "io-bridge.h"
#include "menu.h"
#include "cbuf.h"


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

uint16_t global_timer;

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
    fat32_open_file(filename, "BIT");

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

    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10); // CTC mode, prescaler 1024
    TIMSK1 |= _BV(OCIE1A);
    OCR1A = 1747; // 8 Hz timer
    
    TCCR2A = _BV(WGM21); // CTC mode
    TCCR2B = 0;
    TIMSK2 |= _BV(OCIE2A); // Set interrupt on compare match
    OCR2A = 59; // Interrupt every ~ 33 us
}

void song_open(const char* filename)
{
    fat32_open_root_dir();
    
    if(fat32_open_file(filename, "BIN"))
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
    uint16_t start_time = global_timer;

    song_open(filename);
    song_read_data();
    set_low(PIN_LED);

    uint8_t ret;
    
    while(!(ret = song_handle_inputs()) && !song_done)
    {
        song_read_data();

        if(global_timer - start_time > 8 * 60 * 3)
        {
            song_done = 1  ;
        }
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


void category_read_info(uint8_t choice, char *catname, uint8_t *len)
{
    fat32_open_root_dir();
    fat32_open_file("CAT", "TXT");

    fat32_seek(0);

    fat32_read(0, 3); // Skip num cat + new line
    fat32_read(0, 30 * choice);

    ssd1306_text_start(0,1);
    
    char c;
    for(uint8_t i = 0; i < 20; i++)
    {
        fat32_read(&c, 1);
        ssd1306_text_putc(c);
    }
    ssd1306_text_end();

    fat32_read(catname, 6);

    char numstr[3];
    fat32_read(numstr, 3);

    *len = (numstr[1]-'0')*10 + (numstr[2]-'0');

    fat32_close_file();
}


void category_play(uint8_t choice)
{
    ssd1306_clear();
    ssd1306_puts("Playing:",0,0);

    char filename[8];
    uint8_t len;

    category_read_info(choice, filename, &len);

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

int main()
{
    io_init();
    i2c_master_init();

    timers_init();

    cbuf_init(reg_address);
    cbuf_init(reg_data);

    _delay_ms(300);

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

    uint8_t choice = 0;

    struct menu_info_t menu_info;
    menu_init("CAT", &menu_info);

    for(;;)
    {
        uint8_t res = menu_loop(&menu_info);
        choice = res & ~MENU_BACK_FLAG;
        category_play(choice);
    }
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

ISR(TIMER1_COMPA_vect) // Push out new data
{
    global_timer++;
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
