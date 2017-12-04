#ifndef IO_BRIDGE_
#define IO_BRIDGE_

#define BUTTON_PRESS_RIGHT 1
#define BUTTON_PRESS_UP    2
#define BUTTON_PRESS_DOWN  3
#define BUTTON_PRESS_LEFT  4

#define BUTTON_RELEASE_RIGHT 5
#define BUTTON_RELEASE_UP    6
#define BUTTON_RELEASE_DOWN  7
#define BUTTON_RELEASE_LEFT  8

#define BUTTON_HOLD_RIGHT 9
#define BUTTON_HOLD_UP    10
#define BUTTON_HOLD_DOWN  11
#define BUTTON_HOLD_LEFT  12

void io_set_uart_mode(void);
void io_set_button_mode(void);
uint8_t get_input(void);

void io_uart_flush(void);
uint8_t io_uart_read_byte(void);
void io_uart_skip_bytes(uint8_t n);
void io_uart_write_byte(uint8_t c);


#endif
