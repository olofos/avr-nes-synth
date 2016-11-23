#ifndef MENU_H_
#define MENU_H_

#define MENU_BACK_FLAG 0x80

struct menu_info_t
{
    uint8_t current_option;
    uint8_t top;
    uint8_t num_items;
    char name[9];
};

uint8_t menu_loop(struct menu_info_t *menu_info);
void menu_init(const char* name, struct menu_info_t *menu_info);

#endif
