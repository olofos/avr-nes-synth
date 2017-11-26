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
 * - Timer 1 fires at 8 Hz and increments global_timer
 *
 * - Timer 2 fires at around 60 kHz. It outputs the data from the address
 *   and value buffers to the bus, and stops itself once it finds the end
 *   of frame marker
 ************************************************************************/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

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

#include "ssd1306-internal.h"
#include "ssd1306-cmd.h"
#include "logo-paw-64x64.h"
#include "logo-paw-48x48.h"
#include <string.h>

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

#define XSTR(s) STR(s)
#define STR(s) #s

static const char title_str[] PROGMEM = "NESSynth " XSTR(MAJOR_VERSION) "." XSTR(MINOR_VERSION);
static const uint8_t copyright_c[] PROGMEM = { 0x3C, 0x42, 0x99, 0xA5, 0xA5, 0x81, 0x42, 0x3C };
static const char copyright[] PROGMEM = COPYRIGHT_YEAR " " AUTHOR;

static const char menu_name_cat[8] PROGMEM = "CAT";

static const char menu_main_entry_0[] PROGMEM = "Songs sorted by game";
static const char menu_main_entry_1[] PROGMEM = "Play playlist";
static const char menu_main_entry_2[] PROGMEM = "Edit playlist";
static const char menu_main_entry_3[] PROGMEM = "Log";
static const char menu_main_entry_4[] PROGMEM = "About";

#define MENU_MAIN_SONGS_SORTED_BY_GAME 0
#define MENU_MAIN_PLAY_PLAYLIST 1
#define MENU_MAIN_EDIT_PLAYLIST 2
#define MENU_MAIN_LOG 3
#define MENU_MAIN_ABOUT 4

static const char* const menu_main_entries[] PROGMEM = {
    menu_main_entry_0,
    menu_main_entry_1,
    menu_main_entry_2,
    menu_main_entry_3,
    menu_main_entry_4,
};

static const char menu_playlist_edit_entry_0[] PROGMEM = "Add Track";
static const char menu_playlist_edit_entry_1[] PROGMEM = "Delete Track";
static const char menu_playlist_edit_entry_2[] PROGMEM = "Reorder Tracks";

#define MENU_PLAYLIST_EDIT_ADD_TRACK 0
#define MENU_PLAYLIST_EDIT_DELETE_TRACK 1
#define MENU_PLAYLIST_EDIT_REORDER_TRACKS 2

static const char* const menu_playlist_edit_entries[] PROGMEM = {
    menu_playlist_edit_entry_0,
    menu_playlist_edit_entry_1,
    menu_playlist_edit_entry_2
};


struct menu_info_t menu_main_info;
struct menu_info_t menu_game_info;
struct menu_info_t menu_playlist_info;
struct menu_info_t menu_playlist_edit_info;


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
            log_nl();

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


            toggle(PIN_LED);

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

    for(uint8_t n = 0; n <= 0x17; n++)
    {
        cbuf_push(reg_address, n);
        cbuf_push(reg_data, 0);
    }

    sei();

    timer2_start();
}

// maximum play time for each song in units of 1/8 s
#define MAX_PLAY_TIME (8 * 60 * 3)

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

        if(global_timer - start_time > MAX_PLAY_TIME)
        {
            song_done = 1  ;
        }
    }

    reset_channels();
    _delay_ms(100); // Make sure channels have time to stop playing
    song_stop();

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
    fat32_open_file("CAT", MENU_EXT);

    fat32_seek(0);

    fat32_skip_until('\n'); // Skip number of lines + new line
    for(uint8_t i = 0; i < choice; i++)
    {
        fat32_skip_until('\n');
    }

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

    char filename[9];
    uint8_t len;

    category_read_info(choice, filename, &len);

    uint8_t done = 0;
    int8_t num = 1;

    while(!done)
    {
        filename[6] = '0' + (num / 10);
        filename[7] = '0' + (num % 10);
        filename[8] = 0;

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

void about_show()
{
    ssd1306_clear();

    ssd1306_puts_P(title_str,12,0);

    ssd1306_text_start(0,7);
    i2c_write_P(copyright_c, 8);
    ssd1306_text_end();

    ssd1306_puts_P(copyright, 12,7);

    while(!get_input())
        ;
}

void splash_show()
{
}

extern char log_buf[LOG_BUF_LEN];

void log_show()
{
    uint8_t num_lines = log_buf_count_lines();
    uint8_t first_line = num_lines - 8;
    uint8_t done = 0;

    // TODO: Don't redraw everything each time. Instead use the scrolling capabilities of SSD1306

    while(!done)
    {
        ssd1306_clear();

        for(uint8_t i = 0; i < 8; i++)
        {

            uint8_t pos = log_buf_line_pos(first_line + i);

            for(uint8_t j = 0; j < LOG_BUF_LINE_WIDTH; j++)
            {
                char c = log_buf[(uint8_t)(pos + j)];
                if(c == '\n')
                    break;

                ssd1306_putc(c, 6*j, i);
            }
        }

        uint8_t input;

        while(!(input = get_input()))
            ;

        switch(input)
        {
        case BUTTON_PRESS_LEFT:
        case BUTTON_PRESS_RIGHT:
            done = 1;
        break;

        case BUTTON_PRESS_UP:
        case BUTTON_HOLD_UP:
        if(first_line > 0)
        {
            first_line--;
        }
        break;

        case BUTTON_PRESS_DOWN:
        case BUTTON_HOLD_DOWN:
        if(first_line < num_lines - 8)
        {
            first_line++;
        }
        break;

        }
    }
}

static void menu_init()
{
    menu_pgm_init(menu_main_entries, sizeof(menu_main_entries)/sizeof(menu_main_entries[0]), &menu_main_info);
    menu_pgm_init(menu_playlist_edit_entries, sizeof(menu_playlist_edit_entries)/sizeof(menu_playlist_edit_entries[0]), &menu_playlist_edit_info);
    menu_fat32_init(menu_name_cat, &menu_game_info);
    menu_eeprom_init(&menu_playlist_info);
}


static void menu_play_category_loop()
{
    for(;;)
    {
        uint8_t res = menu_loop(&menu_game_info);
        if(res & MENU_BACK_FLAG)
                    break;

        category_play(res);
    }
}

static void menu_play_playlist_loop()
{
    for(;;)
    {
        uint8_t res = menu_loop(&menu_playlist_info);
        if(res & MENU_BACK_FLAG)
            break;

        char filename[8];
        eeprom_read_block(filename, (void*)(res*8), 8);

        song_play(filename);
    }
}

static void menu_edit_playlist_loop()
{
    for(;;)
    {
        uint8_t res = menu_loop(&menu_playlist_edit_info);
        if(res & MENU_BACK_FLAG)
            break;

        switch(res)
        {
        case MENU_PLAYLIST_EDIT_ADD_TRACK:
            for(;;)
            {
                res = menu_loop(&menu_game_info);
                if(res & MENU_BACK_FLAG)
                    break;

                char filename[8];
                uint8_t len;

                category_read_info(res, filename, &len);

                uint8_t track = 1;

                ssd1306_clear();
                ssd1306_text_start(0,0);
                for(uint8_t i = 0; i < 8; i++)
                {
                    ssd1306_text_putc(filename[i]);
                }
                ssd1306_text_end();

                int8_t done = 0;

                while(!done)
                {
                    char tens = '0' + track / 10;
                    char ones = '0' + track % 10;

                    ssd1306_putc(tens, 6*6,0);
                    ssd1306_putc(ones, 7*6,0);

                    uint8_t input;
                    while(!(input = get_input()))
                        ;

                    switch(input)
                    {
                    case BUTTON_PRESS_UP:
                    case BUTTON_HOLD_UP:
                        if(track > 1)
                            track--;
                        break;

                    case BUTTON_PRESS_DOWN:
                    case BUTTON_HOLD_DOWN:
                        if(track < len)
                            track++;
                        break;

                    case BUTTON_PRESS_RIGHT:
                        done = 1;
                        break;

                    case BUTTON_PRESS_LEFT:
                        done = -1;
                        break;

                    }
                }

                if(done > 0) // Add track
                {
                    filename[6] = '0' + track / 10;
                    filename[7] = '0' + track % 10;
                    if(menu_playlist_info.num_items < 128)
                    {
                        eeprom_write_block(filename, (void*)(8 * menu_playlist_info.num_items), 8);
                        menu_playlist_info.num_items++;
                    }
                }
            }
            break;


        case MENU_PLAYLIST_EDIT_DELETE_TRACK:
            for(;;)
            {
                res = menu_loop(&menu_playlist_info);
                if(res & MENU_BACK_FLAG)
                    break;

                for(uint8_t i = res; i < menu_playlist_info.num_items; i++)
                {
                    uint8_t s[8];
                    eeprom_read_block(s,(void*)((i+1)*8),8);
                    eeprom_update_block(s,(void*)(i*8),8);
                }

                menu_playlist_info.num_items--;
            }
            break;

        case MENU_PLAYLIST_EDIT_REORDER_TRACKS:
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

    sei();

    ssd1306_init();
//    ssd1306_splash();
    {
        ssd1306_clear();

        const struct ssd1306_frame_t frame = { .col_start = 0, .col_end = 63 , .row_start = 0, .row_end = 7, .offset = 0 };

        ssd1306_setup_frame(frame);

        i2c_start(OLED_I2C_ADDRESS, I2C_WRITE);
        i2c_write_byte(OLED_CONTROL_BYTE_DATA_STREAM);
        i2c_write_P(paw_64x64, (uint16_t)64 * 8);

        i2c_stop();

        ssd1306_puts("NESSynth",64+32-6*8/2,2);
        ssd1306_puts(XSTR(MAJOR_VERSION) "." XSTR(MINOR_VERSION),64+32-6*3/2,4);

//        ssd1306_clear();


        for(uint8_t i = 0; i < 60; i++)
        {
            ssd1306_puts_scroll("0123456789", 10, 1, 30, i);
            _delay_ms(125);
        }
    }
    _delay_ms(5000);

    log_init(LOG_I2C|LOG_BUF);
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

    menu_init();

    _delay_ms(1125);
    reset_channels();

    for(;;)
    {
        uint8_t res;

        res = menu_loop(&menu_main_info);

        switch(res)
        {
        case MENU_MAIN_SONGS_SORTED_BY_GAME:
            menu_play_category_loop();
            break;

        case MENU_MAIN_PLAY_PLAYLIST:
            menu_play_playlist_loop();
            break;

        case MENU_MAIN_EDIT_PLAYLIST:
            menu_edit_playlist_loop();
            break;

        case MENU_MAIN_LOG:
            log_show();
            break;

        case MENU_MAIN_ABOUT:
            about_show();
            break;


        default:
            ssd1306_splash();
            _delay_ms(500);
        }
    }
}

static uint8_t frame_counter;

ISR(TIMER0_COMPA_vect) // Frame clock fires at 240 Hz
{
    uint8_t cnt = frame_counter + 1;
    set_pin(PIN_FCLK, cnt & 1);

    if(!(cnt & 0x03))
    {
        // Push out new data at 60 Hz
        if(!song_done)
        {
            timer2_start();
        }
    }

    frame_counter = cnt;
}

ISR(TIMER1_COMPA_vect)
{
    global_timer++;
}

ISR(TIMER2_COMPA_vect) // Push out new data
{
    static uint8_t n = 0;

    if(cbuf_empty(reg_data))
    {
        timer2_stop(); // We are done for now
    } else {
        if(!(n++ & 0x01))
        {
            uint8_t address = cbuf_pop(reg_address);
            if(address == 0xFF) // End song
            {
                timer2_stop();
                cbuf_pop(reg_data);
                n++;
                song_done = 1;
            } else if(address == 0xF1) { // End frame
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
