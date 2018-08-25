#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#include <stdlib.h>
#include <stddef.h>

#include "config.h"
#include "registers.h"

#include "io.h"

#include "cbuf.h"

#include "nes_apu.h"

#define reg_address_LEN 32
#define reg_data_LEN 32

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


#if 0
volatile uint8_t frame_flag; // moved to GPIOR0
#endif


void (*write_reg)(uint8_t address, uint8_t val);
void (*frame_update)();


void io_init()
{
    set_outputs(PINS_DAC);

    set_inputs(PINS_BUS);
    disable_pullups(PINS_BUS);

    set_input(PIN_DCLK);
    disable_pullup(PIN_DCLK);

    set_input(PIN_FCLK);
    disable_pullup(PIN_FCLK);

    set_output(PIN_LED);

    set_input(PIN_CONF0);
    set_input(PIN_CONF1);

    enable_pullup(PIN_CONF0);
    enable_pullup(PIN_CONF1);
}

void timers_init()
{
    TCCR1A = 0;
    TCCR1B = _BV(WGM12);  // Mode 4, CTC on OCR1A
    TIMSK1 = _BV(OCIE1A); // Set interrupt on compare match
}

void interrupts_init()
{
    PCICR = _BV(PCIE1) | _BV(PCIE0); // Enable PCI0 and PCI1
    PCMSK0 = _BV(PCINT0);  // Enable DCLK interrupt
    PCMSK1 = _BV(PCINT13); // Enable FCLK interrupt
}

void read_conf()
{
    uint8_t conf = (is_high(PIN_CONF1) ? 2 : 0) | (is_high(PIN_CONF0) ? 1 : 0);

    channel_conf = (channel_conf & ~(_BV(CONF0_BIT) | _BV(CONF1_BIT))) | (is_high(PIN_CONF0) ? _BV(CONF0_BIT) : 0) | (is_high(PIN_CONF1) ? _BV(CONF1_BIT) : 0);

    switch(conf)
    {
    case CHAN_SQ1:
        write_reg = write_reg_sq1;
        frame_update = frame_update_sq;
        break;

    case CHAN_SQ2:
        write_reg = write_reg_sq2;
        frame_update = frame_update_sq;
        break;

    case CHAN_TRI:
        write_reg = write_reg_tri;
        frame_update = frame_update_tri;
        break;

    case CHAN_NOISE:
    default:
        write_reg = write_reg_noise;
        frame_update = frame_update_noise;

        channel_volume = 0;

        shift_register_hi = 0;
        shift_register_lo = 1;
        break;
    }
}

void blink_conf()
{
    uint8_t conf = (is_high(PIN_CONF1) ? 2 : 0) | (is_high(PIN_CONF0) ? 1 : 0);

    do
    {
        set_high(PIN_LED);
        _delay_ms(125);
        set_low(PIN_LED);
        _delay_ms(250);
    } while(conf--);
}

int main()
{
    io_init();
    timers_init();
    interrupts_init();

    cbuf_init(reg_address);
    cbuf_init(reg_data);

    read_conf();

    sei();

    blink_conf();

    for(;;)
    {
        while(!cbuf_empty(reg_data))
        {
            uint8_t address = cbuf_pop(reg_address);
            uint8_t val = cbuf_pop(reg_data);

            // TODO: Handle 0x4017 to change frame counter mode
            write_reg(address, val);
        }

        set_low(PIN_LED);

        if(frame_flag & _BV(FRAME_FLAG_BIT))
        {
            frame_flag &= ~_BV(FRAME_FLAG_BIT);
            frame_update();
        }
    }
}



#ifndef ASMINTERRUPT
ISR(TIMER1_COMPA_vect)
{
    set_high(PIN_LED);

    if(!(channel_conf & _BV(CONF1_BIT)))
    {
        // case CHAN_SQ1:
        // case CHAN_SQ2:

        channel_step = (channel_step+1) & 0x0F;

        if(channel_step < channel_duty_cycle)
        {
            PINS_DAC_PORT |= channel_volume;
        } else {
            PINS_DAC_PORT &= 0xF0;
        }
    } else {
        if(!(channel_conf & _BV(CONF0_BIT)))
        {
            // case CHAN_TRI:
            if((channel_linear_counter > 0) && (channel_length_counter > 0))
            {
                channel_step = (channel_step + 1) & 0x1F;

                uint8_t val = channel_step;

                if(channel_step & 0x10)
                {
                    val = ~channel_step;
                }

                PINS_DAC_PORT = (PINS_DAC_PORT & 0xF0) | (val & 0x0F);
            }
        } else {
            // case CHAN_NOISE:
            uint8_t feedback;

            if(channel_shift_mode & _BV(SHIFT_MODE_BIT))
            {
                feedback = ((shift_register_lo & _BV(0)) ^ ((shift_register_lo & _BV(6)) >> 6));
            } else {
                feedback = ((shift_register_lo & _BV(0)) ^ ((shift_register_lo & _BV(1)) >> 1));
            }

            // 16-bit shift right
            asm volatile(
                "lsr %0\t\n"
                "ror %1\t\n"
                : "=&r" (shift_register_hi), "=&r" (shift_register_lo):);

            if(feedback)
            {
                shift_register_hi |= 0x40;
            }

            if(!(shift_register_lo & _BV(0)))
            {
                PINS_DAC_PORT |= channel_volume;
            } else {
                PINS_DAC_PORT &= 0xF0;
            }
        }
    }

    set_low(PIN_LED);
}

ISR(PCINT1_vect)
{
    frame_flag |= _BV(FRAME_FLAG_BIT);
}

#endif

// High: address, Low: data

ISR(PCINT0_vect)
{
    if(is_high(PIN_DCLK))
    {
        cbuf_push(reg_address, PINS_BUS_PIN);
    } else {
        cbuf_push(reg_data, PINS_BUS_PIN);
    }
    set_high(PIN_LED);
}
