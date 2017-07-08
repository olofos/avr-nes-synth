#ifndef MENU_H_
#define MENU_H_

#define MENU_BACK_FLAG 0x80
#define MENU_EXT "MNU"

struct menu_ops_t;

struct menu_info_t
{
    uint8_t current_option;
    uint8_t top;
    uint8_t num_items;
    void *data;
    const struct menu_ops_t *ops;
};

uint8_t menu_loop(struct menu_info_t *menu_info);
void menu_pgm_init(const char* const entries[], uint8_t num_items, struct menu_info_t *menu_info);
void menu_fat32_init(const char* name_p, struct menu_info_t *menu_info);
void menu_eeprom_init(struct menu_info_t *menu_info);

#endif
