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

void menu_init(const char* name, struct menu_info_t *menu_info)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        menu_info->name[i] = name[i];
    }
    menu_info->name[8] = 0;
    
    fat32_open_root_dir();
    fat32_open_file(menu_info->name, "TXT");
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
}

static const uint8_t PROGMEM menu_propmt[] = { 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00, 0x00 };

void menu_redraw(struct menu_info_t *menu_info)
{
    fat32_seek(0);

    fat32_read(0, 3); // Skip num cat + new line

    for(uint8_t i = 0; i < menu_info->top; i++)
    {
        char c;
        do {
            fat32_read(&c, 1);
        } while(c != '\n');
    }

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
            char c;
            for(uint8_t j = 0; j < 20; j++)
            {
                fat32_read(&c, 1);
                ssd1306_text_putc(c);
            }
            do {
                fat32_read(&c, 1);
            } while(c != '\n');
        } else {
            for(uint8_t j = 0; j < 20; j++)
            {
                ssd1306_text_putc(' ');
            }          
        }

        ssd1306_text_end();
    }
}

uint8_t menu_handle_input()
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
    if(menu_info->current_option < menu_info->num_items-2)
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

    fat32_open_root_dir();
    fat32_open_file(menu_info->name, "TXT");
    
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

    fat32_close_file();

    return res;
}
