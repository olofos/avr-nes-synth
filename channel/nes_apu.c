#ifdef AVR
#include <avr/io.h>
#else
#define _BV(n) (1U << (n))
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#ifdef AVR
#include "registers.h"
#else
#define channel_shift_mode     channel.shift_mode
#define channel_conf           channel.conf
#define channel_duty_cycle     channel.duty_cycle
#define channel_length_counter channel.length_counter
#define channel_volume         channel.volume
#define channel_linear_counter channel.linear_counter
#endif

#include "config.h"
#include "nes_apu.h"

#ifdef AVR
#define channel_timer_start() do { TCCR1B |=  _BV(CS11); } while(0)
#define channel_timer_stop()  do { TCCR1B &= ~_BV(CS11); } while(0)
#define channel_timer_set_period(p) do { OCR1A = p; } while(0)
#else
void channel_timer_start(void);
void channel_timer_stop(void);
void channel_timer_set_period(uint16_t period);
#endif

volatile uint8_t wave_buf[32] __attribute__ ((aligned (0x100))) = {
    0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};

channel_t channel;

static const uint8_t length_lut[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// Note: the lowest value is made a bit larger in order for the interrupt to have time to fire
static const uint16_t noise_period_lut[16] = {
    0x008, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0a0,
    0x0ca, 0x0fe, 0x17c, 0x1fc, 0x2fa, 0x3f8, 0x7f2, 0xfe4
};

static const uint8_t duty_tab[4] = {2, 4, 8, 12};

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
    channel_timer_set_period(channel.period);
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

    channel_timer_set_period(channel.period);
    if(channel.period < 8)
    {
        channel_timer_stop();
    } else {
        channel_timer_start();
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
        channel_timer_set_period(channel.period);
        break;

    case 0x0B:
        /*
          0-2   3 MS bits of wavelength (unused on noise channel)
          3-7   length counter load register
        */
        channel_length_counter = length_lut[val >> 3];
        channel.linear_counter_reload_flag = 1;

        channel.period_hi = val & 0x07;

        channel_timer_set_period(channel.period);

        if(channel.period < 4)
        {
            channel_timer_stop();
        } else {
            channel_timer_start();
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
            channel_shift_mode |= _BV(SHIFT_MODE_BIT);
        } else {
            channel_shift_mode &= ~_BV(SHIFT_MODE_BIT);
        }

        channel.period = noise_period_lut[val & 0x0F];

        channel_timer_set_period(channel.period);

        channel_timer_start(); // ???
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
        if(!(channel_conf & _BV(CONF0_BIT)))
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
            channel_timer_set_period(channel.period);
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
    if(channel.frame_counter++ & 0x01) // clock length counters and sweep units
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
    if(channel.frame_counter++ & 0x01) // clock length counters
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
        channel_timer_stop();
    }
}


void frame_update_tri()
{
    if(channel.frame_counter++ & 0x01) // clock length counters
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
