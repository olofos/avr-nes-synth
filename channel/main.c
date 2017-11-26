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


#define timer1_start() do { TCCR1B |=  _BV(CS11); } while(0)
#define timer1_stop()  do { TCCR1B &= ~_BV(CS11); } while(0)

#define CHAN_SQ1   0
#define CHAN_SQ2   1
#define CHAN_TRI   2
#define CHAN_NOISE 3

volatile uint8_t wave_buf[32] __attribute__ ((aligned (0x100))) = {
    0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};

typedef struct
{
    union
    {
        uint16_t period;                     // Square + Noise + Triangle
        struct
        {
            uint8_t period_lo;
            uint8_t period_hi;
        };
    };

    volatile uint8_t enabled;                // Square + Noise + Triangle
    uint8_t length_counter_halt_flag;        // Square + Noise + Triangle

#if 0 // moved to register
    volatile uint8_t length_counter;         // Square + Noise + Triangle

    union {
        volatile uint8_t volume;             // Square + Noise
        volatile uint8_t linear_counter;     // Triangle
    };
#endif

    union {
        uint8_t env_reload_flag;             // Square + Noise
        uint8_t linear_counter_reload_flag;  // Triangle
    };

    union {
        uint8_t env_reload_value;            // Square + Noise
        uint8_t linear_counter_reload_value; // Triangle
    };

    uint8_t env_const_flag;                  // Square + Noise
    uint8_t env_divider;                     // Square + Noise
    uint8_t env_volume;                      // Square + Noise

#if 0 // moved to register & GPIOR0
    union {
        volatile uint8_t duty_cycle;         // Square
        volatile uint8_t shift_mode;         // Noise
    };
#endif

#if 0 // moved to register
    uint16_t shift_register;             // Noise
#endif

    uint8_t sweep_div_period;                // Square
    uint8_t sweep_div_counter;               // Square
    uint8_t sweep_shift;
    uint8_t sweep_neg_flag;                  // Square
    uint8_t sweep_reload_flag;               // Square
    uint8_t sweep_enabled;                   // Square

} channel_t;

channel_t channel;




const uint8_t length_lut[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// Note: the lowest value is made a bit larger in order for the interrupt to have time to fire
const uint16_t noise_period_lut[16] = {
//  0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0a0,
    0x008, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0a0,
    0x0ca, 0x0fe, 0x17c, 0x1fc, 0x2fa, 0x3f8, 0x7f2, 0xfe4
};

const uint8_t duty_tab[4] = {2, 4, 8, 12};


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

static void write_reg_sq_reg_0(uint8_t val)
{
    /*
      0-3   volume / envelope decay rate
      4     envelope decay disable
      5     length counter clock disable / envelope decay looping enable (hold note)
      6-7   duty cycle type (unused on noise channel)
    */

    channel.env_reload_value = val & 0x0f;

    channel.env_const_flag = val & 0x10;
    channel.length_counter_halt_flag = val & 0x20;

    channel_duty_cycle = duty_tab[val >> 6];
}

static void write_reg_sq_reg_1(uint8_t val)
{
    /*
      0-2   right shift amount
      3     negative
      4-6   sweep period
      7     sweep enable
    */

    channel.sweep_enabled = val & 0x80;
    channel.sweep_neg_flag = val & 0x08;

    channel.sweep_div_period = ((val >> 4) & 0x07) + 1;
    channel.sweep_shift = val & 0x07;

    channel.sweep_reload_flag = 1;
}

static void write_reg_sq_reg_2(uint8_t val)
{
    /*
      0-7   8 LSB of wavelength
    */
    channel.period_lo = val;
    OCR1A = channel.period;
}

static void write_reg_sq_reg_3(uint8_t val)
{
    /*
      0-2   3 MS bits of wavelength (unused on noise channel)
      3-7   length counter load register
    */
    channel_length_counter = length_lut[val >> 3];
    channel.env_reload_flag = 1;

    channel.period_hi = val & 0x07;

    OCR1A = channel.period;
    if(channel.period < 8)
    {
        timer1_stop();
    } else {
        timer1_start();
    }
}


void write_reg_sq1(uint8_t address, uint8_t val)
{
    switch(address)
    {
    case 0x00:
        write_reg_sq_reg_0(val);
        break;

    case 0x01:
        write_reg_sq_reg_1(val);
        break;

    case 0x02:
        write_reg_sq_reg_2(val);
        break;

    case 0x03:
        write_reg_sq_reg_3(val);
        break;

    case 0x15:
        channel.enabled = val & 0x01;
        break;
    }
}


void write_reg_sq2(uint8_t address, uint8_t val)
{
    switch(address)
    {
    case 0x04:
        write_reg_sq_reg_0(val);
        break;

    case 0x05:
        write_reg_sq_reg_1(val);
        break;

    case 0x06:
        write_reg_sq_reg_2(val);
        break;

    case 0x07:
        write_reg_sq_reg_3(val);
        break;

    case 0x15:
        channel.enabled = val & 0x02;
        break;
    }
}

void write_reg_tri(uint8_t address, uint8_t val)
{
    switch(address)
    {
    case 0x08: // 0x4008
        /*
          0-6   linear counter load register
          7     length counter clock disable / linear counter start
        */

        channel.length_counter_halt_flag = val & 0x80;

        channel.linear_counter_reload_value = val & 0x7f;
        break;

    case 0x0A:
        /*
          0-7   8 LSB of wavelength
        */

        channel.period_lo = val;
        OCR1A = channel.period;
        break;

    case 0x0B:
        /*
          0-2   3 MS bits of wavelength (unused on noise channel)
          3-7   length counter load register
        */
        channel_length_counter = length_lut[val >> 3];
        channel.linear_counter_reload_flag = 1;

        channel.period_hi = val & 0x07;

        OCR1A = channel.period;

        if(channel.period < 4)
        {
            timer1_stop();
        } else {
            timer1_start();
        }

        break;

    case 0x15:
        channel.enabled = val & 0x04;
        break;
    }
}

void write_reg_noise(uint8_t address, uint8_t val)
{
    switch(address)
    {
    case 0x0C:
        /*
          0-3   volume / envelope decay rate
          4     envelope decay disable
          5     length counter clock disable / envelope decay looping enable (hold note)
        */
        channel.env_reload_value = val & 0x0f;

        channel.env_const_flag = val & 0x10;
        channel.length_counter_halt_flag = val & 0x20;
        break;

    case 0x0E:
        /*
          0-3 period from LUT
          7   mode
         */

        if(val & 0x80)
        {
            GPIOR0 |= _BV(SHIFT_MODE_BIT);
        } else {
            GPIOR0 &= ~_BV(SHIFT_MODE_BIT);
        }

        channel.period = noise_period_lut[val & 0x0F];

        OCR1A = channel.period;

        timer1_start(); // ???
        break;

    case 0x0F:
        /*
          4-7  length counter
         */
        channel_length_counter = length_lut[val >> 3];
        channel.env_reload_flag = 1;
        break;

    case 0x15:
        channel.enabled = val & 0x08;
        break;
    }
}

static uint8_t frame_counter;

static void frame_update_length_counter()
{
    if(!channel.length_counter_halt_flag && channel_length_counter > 0)
    {
        channel_length_counter--;
    }
}

static void frame_update_sweep()
{
    int16_t delta = channel.period >> channel.sweep_shift;

    if(channel.sweep_neg_flag)
    {
        if(!(GPIOR0 & _BV(CONF0_BIT)))
        {
            delta = ~delta; // Pulse 1 adds the ones' complement
        } else {
            delta = -delta; // Pulse 2 adds the two's complement
        }
    }

    uint16_t target_period = channel.period + delta;

    if(channel.sweep_div_counter > 0) {
        channel.sweep_div_counter--;
    } else {
        channel.sweep_div_counter = channel.sweep_div_period;

        if((target_period > 0x07FF) || (channel.period < 8))
        {
            // Mute the channel?
            // TODO: Introduce mute flag in GPIOR0?
            channel_volume = 0;
        } else if(channel.sweep_enabled) {
            channel.period = target_period;
            OCR1A = channel.period;
        }
    }

    if(channel.sweep_reload_flag)
    {
        channel.sweep_div_counter = channel.sweep_div_period;
        channel.sweep_reload_flag = 0;
    }
}

static void frame_update_envelope()
{
    if(channel.env_reload_flag)
    {
        channel.env_reload_flag = 0;

        channel.env_divider = channel.env_reload_value;
        channel.env_volume = 0x0F;
    } else if(!channel.env_divider) {
        channel.env_divider = channel.env_reload_value;

        if(channel.env_volume)
        {
            channel.env_volume--;
        } else if(channel.length_counter_halt_flag) {
            channel.env_volume = 0x0F;
        }

    } else {
        channel.env_divider--;
    }
}

void frame_update_sq()
{
    if(frame_counter++ & 0x01) // clock length counters and sweep units
    {
        frame_update_length_counter();
        frame_update_sweep();
    }

    frame_update_envelope();

    if(!channel_length_counter)
    {
        channel_volume = 0;
    } else if(channel.env_const_flag) {
        channel_volume = channel.env_reload_value;
    } else {
        channel_volume = channel.env_volume;
    }
}

void frame_update_noise()
{
    if(frame_counter++ & 0x01) // clock length counters
    {
        frame_update_length_counter();
    }

    frame_update_envelope();

    if(!channel_length_counter)
    {
        channel_volume = 0;
    } else if(channel.env_const_flag) {
        channel_volume = channel.env_reload_value;
    } else {
        channel_volume = channel.env_volume;
    }

    if(!channel_volume)
    {
        timer1_stop();
    }
}


void frame_update_tri()
{
    if(frame_counter++ & 0x01) // clock length counters
    {
        frame_update_length_counter();
    }

    // clock triangle's linear counter
    if(channel.linear_counter_reload_flag)
    {
        channel_linear_counter = channel.linear_counter_reload_value;
    } else if(channel_linear_counter > 0) {
        channel_linear_counter--;
    }

    if(!channel.length_counter_halt_flag)
    {
        channel.linear_counter_reload_flag = 0;
    }
}


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

    GPIOR0 = (GPIOR0 & ~(_BV(CONF0_BIT) | _BV(CONF1_BIT))) | (is_high(PIN_CONF0) ? _BV(CONF0_BIT) : 0) | (is_high(PIN_CONF1) ? _BV(CONF1_BIT) : 0);

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

        if(GPIOR0 & _BV(FRAME_FLAG_BIT))
        {
            GPIOR0 &= ~_BV(FRAME_FLAG_BIT);
            frame_update();
        }
    }
}



#ifndef ASMINTERRUPT
ISR(TIMER1_COMPA_vect)
{
    set_high(PIN_LED);

    if(!(GPIOR0 & _BV(CONF1_BIT)))
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
        if(!(GPIOR0 & _BV(CONF0_BIT)))
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

            if(GPIOR0 & _BV(SHIFT_MODE_BIT))
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
    GPIOR0 |= _BV(FRAME_FLAG_BIT);
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
