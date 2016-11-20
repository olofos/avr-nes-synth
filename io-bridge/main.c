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

#define RECEIVE_MODE 0
#define RECEIVE_UART 1

static uint8_t receive_state;

void i2c_receive_start()
{
    receive_state = RECEIVE_MODE;
}

// First byte:
//   LEDS in bits 0-3 with (data & 0xF0) == 0
//   Uart mode if (data & 0xF0) != 0
// Remaining bytes:
//   To uart

void i2c_receive(uint8_t data)
{
    if(receive_state == RECEIVE_MODE)
    {
        if(!(data & 0xF0))
        {
            set_pin(PIN_LED1, data & _BV(0));
            set_pin(PIN_LED2, data & _BV(1));
            set_pin(PIN_LED3, data & _BV(2));
            set_pin(PIN_LED4, data & _BV(3));
        } else {
            receive_state = RECEIVE_UART;
        }
    } else {
        if(data)
            uart_putc(data);
    }
}

void i2c_receive_stop()
{
}

void i2c_transmit_start()
{
}


// Bit 0-3: Switch up
// Bit 4-7: Switch down
uint8_t i2c_transmit_data()
{
    static uint8_t prev_button_state;
    
    uint8_t button_up = button_state & ~prev_button_state;
    uint8_t button_down = ~button_state & prev_button_state;

    prev_button_state = button_state;

    uint8_t data = 0;

    if(button_up & _BV(PIN_SW1)) data |= _BV(0);
    if(button_up & _BV(PIN_SW2)) data |= _BV(1);
    if(button_up & _BV(PIN_SW3)) data |= _BV(2);
    if(button_up & _BV(PIN_SW4)) data |= _BV(3);

    if(button_down & _BV(PIN_SW1)) data |= _BV(4);
    if(button_down & _BV(PIN_SW2)) data |= _BV(5);
    if(button_down & _BV(PIN_SW3)) data |= _BV(6);
    if(button_down & _BV(PIN_SW4)) data |= _BV(7);

    set_low(PIN_IO_INT);

    return data;
}

void timers_init()
{
    TCCR0A = 0; // Normal mode, TOV flag set on 0xFF
    TIMSK0 = _BV(TOIE0); // Set interrupt on overflow
    TCCR0B = _BV(CS02); // CLK / 256
}

void io_init()
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


int main()
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
