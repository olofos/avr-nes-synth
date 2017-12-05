#ifndef UART_H_
#define UART_H_

#include "cbuf.h"
#include "config.h"

#ifndef uart_rx_cbuf_LEN
#define uart_rx_cbuf_LEN 64
#endif

#ifndef uart_tx_cbuf_LEN
#define uart_tx_cbuf_LEN 64
#endif

typedef struct
{
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t buf[uart_rx_cbuf_LEN];
} uart_rx_cbuf_t;


typedef struct
{
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t buf[uart_tx_cbuf_LEN];
} uart_tx_cbuf_t;

extern uart_rx_cbuf_t uart_rx_cbuf;
extern uart_tx_cbuf_t uart_tx_cbuf;


void uart_init(void);

void uart_putc(char data);
void uart_puts(const char *str);
void uart_puts_P(const char *progmem_s);

void uart_putc_nowait(char data);
void uart_puts_nowait(const char *str);


char uart_getc(void);

inline uint8_t uart_available(void)
{
    return cbuf_len(uart_rx_cbuf);
}

#endif
