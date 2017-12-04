#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <stdlib.h>

#include "config.h"
#include "io.h"
#include "uart.h"
#include "i2c-slave.h"
#include "log.h"


volatile static uint8_t button_state;

#define RECEIVE_LEDS 0
#define RECEIVE_UART 1

#define TRANSMIT_BUTTONS 0
#define TRANSMIT_UART    1

volatile static uint8_t receive_state;
volatile static uint8_t transmit_state = TRANSMIT_BUTTONS;

uint8_t i2c_receive_start(void)
{
    receive_state = RECEIVE_LEDS;
    return  1;
}

// First byte:
//   LEDS in bits 0-3 with (data & 0xF0) == 0
//   Receive UART mode if data == 0x80
//   Transmit UART mode if data == 0x41
//   Transmit buttons mode if data == 0x40
// Remaining bytes:
//   To uart

uint8_t i2c_receive(uint8_t data)
{
    if(receive_state == RECEIVE_LEDS)
    {
        if(!(data & 0xF0))
        {
            set_pin(PIN_LED1, data & _BV(0));
            set_pin(PIN_LED2, data & _BV(1));
            set_pin(PIN_LED3, data & _BV(2));
            set_pin(PIN_LED4, data & _BV(3));
        } else {
            if(data & 0x80)
            {
                receive_state = RECEIVE_UART;
            }

            if(data & 0x40)
            {
                transmit_state = data & 0x01;
            }
        }
    } else {
        // if(data)
        // {
        //     if(data == '\n')
        //     {
        //         uart_putc('\r');
        //     }

            uart_putc(data);
        // }
    }
    return  1;
}

void i2c_receive_stop(void)
{
}


volatile static uint8_t transmit_start;

void i2c_transmit_start(void)
{
    if(transmit_state == TRANSMIT_UART)
    {
        transmit_start = 1;
    }
}


// Bit 0-3: Switch up
// Bit 4-7: Switch down
uint8_t i2c_transmit_data(uint8_t *data)
{
    if(transmit_state == TRANSMIT_BUTTONS)
    {
        static uint8_t prev_button_state;

        uint8_t button_up = button_state & ~prev_button_state;
        uint8_t button_down = ~button_state & prev_button_state;

        prev_button_state = button_state;

        uint8_t d = 0;

        if(button_up & _BV(PIN_SW1)) d |= _BV(0);
        if(button_up & _BV(PIN_SW2)) d |= _BV(1);
        if(button_up & _BV(PIN_SW3)) d |= _BV(2);
        if(button_up & _BV(PIN_SW4)) d |= _BV(3);

        if(button_down & _BV(PIN_SW1)) d |= _BV(4);
        if(button_down & _BV(PIN_SW2)) d |= _BV(5);
        if(button_down & _BV(PIN_SW3)) d |= _BV(6);
        if(button_down & _BV(PIN_SW4)) d |= _BV(7);

        set_low(PIN_IO_INT);

        *data = d;

        return 0;
    } else {
        if(transmit_start)
        {
            uint8_t len = cbuf_len(uart_rx_cbuf);
            *data = len;
            transmit_start = 0;
            return len > 0;
        } else {
            *data = cbuf_pop(uart_rx_cbuf);
            return !cbuf_empty(uart_rx_cbuf);
        }
    }
}

void timers_init(void)
{
    TCCR0A = 0; // Normal mode, TOV flag set on 0xFF
    TIMSK0 = _BV(TOIE0); // Set interrupt on overflow
    TCCR0B = _BV(CS02); // CLK / 256
}

void io_init(void)
{
    set_outputs(PINS_LED);

    set_output(PIN_IO_INT);

    set_high(PIN_LED1);
    set_high(PIN_LED2);
    set_high(PIN_LED3);
    set_high(PIN_LED4);

    set_inputs(PINS_SW);
    enable_pullups(PINS_SW);
}

//extern volatile uint8_t i2c_ret;
int main(void)
{
    io_init();
    uart_init();
    log_init(LOG_UART);
    sei();

    log_puts_P(PSTR("Initialising I2C slave\n"));
    i2c_slave_init(I2C_IO_BRIDGE_ADDRESS);

    timers_init();

    for(;;)
    {
        if(!cbuf_empty(uart_rx_cbuf))
        {
            set_low(PIN_LED1);
        } else {
            set_high(PIN_LED1);
        }

        // if(i2c_ret != 0)
        // {
        //     uart_putc(i2c_ret);
        //     i2c_ret = 0;
        // }
    }
}

// See http://www.compuphase.com/electronics/debouncing.htm

ISR(TIMER0_OVF_vect)
{
    static uint8_t cnt0, cnt1;
    uint8_t delta, changed, sample;

    sample = PINS_SW_PIN & PINS_SW_MASK;

    delta = sample ^ button_state;
    cnt1 = (cnt1 ^ cnt0) & delta;
    cnt0 = ~cnt0 & delta;

    changed = delta & ~(cnt0 | cnt1);

    button_state ^= changed;

    if(changed)
    {
        set_high(PIN_IO_INT);
    }
}
