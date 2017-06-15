#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "io.h"
#include "config.h"
#include "uart.h"

#ifdef BAUD

#include <util/setbaud.h>

uart_rx_cbuf_t uart_rx_cbuf;
uart_tx_cbuf_t uart_tx_cbuf;


void uart_init(void)
{
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif
    
    UCSR0B|= _BV(RXEN0) | _BV(TXEN0);   // enable receiver and transmitter
    UCSR0C|= _BV(UCSZ01) | _BV(UCSZ00); // asynchronous, 8bit, 1 stop bit, no parity

    UCSR0B |= _BV(RXCIE0); // enable Recieve Complete Interrupt

    cbuf_init(uart_rx_cbuf);
    cbuf_init(uart_tx_cbuf);
}

void uart_putc(char c)
{
    while(cbuf_full(uart_tx_cbuf))
        ;

    cbuf_push(uart_tx_cbuf, c);

    UCSR0B |= _BV(UDRIE0); // enable UDRE Interrupt
}


void uart_puts(const char *str)
{
    while(*str)
    {
	if(*str == '\n')
	    uart_putc('\r');
        uart_putc(*str++);
    }
}


void uart_putc_nowait(char c)
{
    if(cbuf_full(uart_tx_cbuf))
        cbuf_pop(uart_tx_cbuf);
    
    cbuf_push(uart_tx_cbuf, c);

    UCSR0B |= _BV(UDRIE0); // enable UDRE Interrupt
}


void uart_puts_nowait(const char *str)
{
    while(*str)
    {
	if(*str == '\n')
	    uart_putc_nowait('\r');
        uart_putc_nowait(*str++);
    }
}

void uart_puts_P(const char *progmem_s)
{
    char c;
    
    while((c = pgm_read_byte(progmem_s++)))
    {
	if(c == '\n')
	    uart_putc('\r');

        uart_putc(c);
    }
}

char uart_getc(void)
{
    while(cbuf_empty(uart_rx_cbuf))
        ;

    return cbuf_pop(uart_rx_cbuf);
}


ISR(USART_RX_vect) {
    // TODO: Check for overflow?

    if(cbuf_full(uart_rx_cbuf)) // Buffer full!
        cbuf_pop(uart_rx_cbuf);

    cbuf_push(uart_rx_cbuf, UDR0);
}

ISR(USART_UDRE_vect)
{
    if(cbuf_len(uart_tx_cbuf))
    {
        UDR0 = cbuf_pop(uart_tx_cbuf);
    } else {
        UCSR0B &= ~_BV(UDRIE0); // disable UDRE Interrupt
    }
}

#endif
