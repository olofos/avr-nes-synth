#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "fat32.h"
#include "ssd1306.h"
#include "i2c-master.h"
#include "io-bridge.h"
#include "log.h"
#include "menu.h"

#define MENU_HEIGHT 8

#define MENU_NEXT 0x01
#define MENU_PREV 0x02
#define MENU_FORW 0x04
#define MENU_BACK 0x08

static const uint8_t PROGMEM menu_propmt[] = { 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00, 0x00 };

struct menu_ops_t
{
    void (*find_entry)(struct menu_info_t *menu_info, uint8_t num);
    void (*print_next_entry)(struct menu_info_t *menu_info);
    void (*loop_begin)(struct menu_info_t *menu_info);
    void (*loop_end)(struct menu_info_t *menu_info);
};

//// PGM menu functions

static void menu_pgm_find_entry(struct menu_info_t *menu_info, uint8_t num);
static void menu_pgm_print_next_entry(struct menu_info_t *menu_info);
static void menu_pgm_loop_begin(struct menu_info_t *menu_info);
static void menu_pgm_loop_end(struct menu_info_t *menu_info);

const struct menu_ops_t menu_pgm_ops = {
    .find_entry = menu_pgm_find_entry,
    .print_next_entry = menu_pgm_print_next_entry,
    .loop_begin = menu_pgm_loop_begin,
    .loop_end = menu_pgm_loop_end
};

static uint8_t menu_pgm_index;


void menu_pgm_init(const char* const entries[], uint8_t num_items, struct menu_info_t *menu_info)
{
    menu_info->num_items = num_items;
    menu_info->data = (void*) entries;
    menu_info->top = 0;
    menu_info->current_option = 0;
    menu_info->ops = &menu_pgm_ops;
}

static void menu_pgm_find_entry(struct menu_info_t *menu_info, uint8_t num)
{
    menu_pgm_index = num;
}

static void menu_pgm_print_next_entry(struct menu_info_t *menu_info)
{
    const char *p = pgm_read_word((const char * const *)menu_info->data + menu_pgm_index);

    char c;
    uint8_t i = 0;

    while((c = pgm_read_byte(p++)))
    {
        ssd1306_text_putc(c);
        i++;
    }

    while(i < SSD1306_LINE_WIDTH-1)
    {
        ssd1306_text_putc(' ');
        i++;
    }

    menu_pgm_index++;
}

static void menu_pgm_loop_begin(struct menu_info_t *menu_info)
{
}

static void menu_pgm_loop_end(struct menu_info_t *menu_info)
{
}


//// FAT32 menu functions

static void menu_fat32_find_entry(struct menu_info_t *menu_info, uint8_t num);
static void menu_fat32_print_next_entry(struct menu_info_t *menu_info);
static void menu_fat32_loop_begin(struct menu_info_t *menu_info);
static void menu_fat32_loop_end(struct menu_info_t *menu_info);

const struct menu_ops_t menu_fat32_ops = {
    .find_entry = menu_fat32_find_entry,
    .print_next_entry = menu_fat32_print_next_entry,
    .loop_begin = menu_fat32_loop_begin,
    .loop_end = menu_fat32_loop_end
};

void menu_fat32_init(const char* name_p, struct menu_info_t *menu_info)
{
    menu_info->data = (void *)name_p;

    char filename[8];
    strncpy_P(filename, menu_info->data, 8);

    fat32_open_root_dir();
    fat32_open_file(filename, MENU_EXT);
    log_puts("Reading menu\n");

    fat32_seek(0);

    char numstr[2];
    fat32_read(numstr, 2);

    menu_info->num_items = 10*(numstr[0] - '0') + (numstr[1] - '0');

    log_puts("Number of menu items: ");
    log_put_uint8(menu_info->num_items);
    log_nl();

    fat32_close_file();

    menu_info->top = 0;
    menu_info->current_option = 0;
    menu_info->ops = &menu_fat32_ops;
}

static void menu_fat32_find_entry(struct menu_info_t *menu_info, uint8_t num)
{
    fat32_seek(0);

    fat32_skip_until('\n');

    for(uint8_t i = 0; i < menu_info->top; i++)
    {
        fat32_skip_until('\n');
    }
}

static void menu_fat32_print_next_entry(struct menu_info_t *menu_info)
{
    char c;
    for(uint8_t j = 0; j < SSD1306_LINE_WIDTH-1; j++)
    {
        fat32_read(&c, 1);
        ssd1306_text_putc(c);
    }
    fat32_skip_until('\n');
}

static void menu_fat32_loop_begin(struct menu_info_t *menu_info)
{
    fat32_open_root_dir();

    char filename[8];
    strncpy_P(filename, menu_info->data, 8);

    fat32_open_file(filename, MENU_EXT);
}

static void menu_fat32_loop_end(struct menu_info_t *menu_info)
{
    fat32_close_file();
}


//// EEPROM menu functions

static void menu_eeprom_find_entry(struct menu_info_t *menu_info, uint8_t num);
static void menu_eeprom_print_next_entry(struct menu_info_t *menu_info);
static void menu_eeprom_loop_begin(struct menu_info_t *menu_info);
static void menu_eeprom_loop_end(struct menu_info_t *menu_info);

const struct menu_ops_t menu_eeprom_ops = {
    .find_entry = menu_eeprom_find_entry,
    .print_next_entry = menu_eeprom_print_next_entry,
    .loop_begin = menu_eeprom_loop_begin,
    .loop_end = menu_eeprom_loop_end
};

static uint8_t *menu_eeprom_ptr;

void menu_eeprom_init(struct menu_info_t *menu_info)
{

    uint8_t i;

    for(i = 0; i < 128; i++)
    {
        uint8_t c = 0xFF;

        for(uint8_t j = 0; j < 8; j++)
        {
            c &= eeprom_read_byte((const uint8_t*) (8*i + j));
        }

        if(c == 0xFF)
        {
            break;
        }
    }

    menu_info->num_items = i;

    menu_eeprom_ptr = 0;
    menu_info->top = 0;
    menu_info->current_option = 0;
    menu_info->ops = &menu_eeprom_ops;
}

static void menu_eeprom_find_entry(struct menu_info_t *menu_info, uint8_t num)
{
    menu_eeprom_ptr = (void*) (8 * num);
}

static void menu_eeprom_print_next_entry(struct menu_info_t *menu_info)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        char c = eeprom_read_byte(menu_eeprom_ptr++);
        ssd1306_text_putc(c);
    }
}

static void menu_eeprom_loop_begin(struct menu_info_t *menu_info)
{
}

static void menu_eeprom_loop_end(struct menu_info_t *menu_info)
{
}




//// Generic menu functions

void menu_redraw(struct menu_info_t *menu_info)
{
    menu_info->ops->find_entry(menu_info, menu_info->top);

    for(uint8_t i = 0; i < MENU_HEIGHT; i++)
    {
        ssd1306_text_start(0,i);

        if(menu_info->current_option == menu_info->top + i)
        {
            i2c_write_P(menu_propmt, 8);

        } else {
            for(uint8_t j = 0; j < 8; j++)
            {
            i2c_write_byte(0);
            }
        }

        if(i < menu_info->num_items)
        {
            menu_info->ops->print_next_entry(menu_info);
        } else {
            for(uint8_t j = 0; j < SSD1306_LINE_WIDTH-1; j++)
            {
                ssd1306_text_putc(' ');
            }
        }

        ssd1306_text_end();
    }
}

uint8_t menu_handle_input(void)
{
    switch(get_input())
    {
    case BUTTON_PRESS_UP:
    case BUTTON_HOLD_UP:
        return MENU_PREV;

    case BUTTON_PRESS_DOWN:
    case BUTTON_HOLD_DOWN:
        return MENU_NEXT;

    case BUTTON_PRESS_RIGHT:
        return MENU_FORW;

    case BUTTON_PRESS_LEFT:
        return MENU_BACK;
    }

    return 0;
}

void menu_prev(struct menu_info_t *menu_info)
{
    if(menu_info->current_option > 0)
    {
        menu_info->current_option--;
    }
    if(menu_info->current_option < menu_info->top)
    {
        menu_info->top--;
    }
}

void menu_next(struct menu_info_t *menu_info)
{
    if(menu_info->current_option < menu_info->num_items - 1)
    {
        menu_info->current_option++;
    }
    if(menu_info->current_option > menu_info->top + MENU_HEIGHT - 1)
    {
        menu_info->top++;
    }
}

uint8_t menu_loop(struct menu_info_t *menu_info)
{
    ssd1306_clear();

    if(menu_info->current_option >= menu_info->num_items)
    {
        menu_info->current_option = menu_info->num_items - 1;
    }


    menu_info->ops->loop_begin(menu_info);

    menu_redraw(menu_info);

    uint8_t done = 0;
    uint8_t res;

    while(!done)
    {
        switch(menu_handle_input())
        {
        case MENU_PREV:
            menu_prev(menu_info);
            menu_redraw(menu_info);
            break;

        case MENU_NEXT:
            menu_next(menu_info);
            menu_redraw(menu_info);
            break;

        case MENU_FORW:
            res = menu_info->current_option;
            done = 1;
            break;

        case MENU_BACK:
            res = menu_info->current_option | MENU_BACK_FLAG;
            done = 1;
            break;

        }
    }

    menu_info->ops->loop_end(menu_info);

    return res;
}
