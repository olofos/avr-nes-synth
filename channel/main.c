#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#include <stdlib.h>
#include <stddef.h>

#include "config.h"
#include "io.h"

#include "cbuf.h"


#define timer1_start() do { TCCR1B |=  _BV(CS11); } while(0)
#define timer1_stop()  do { TCCR1B &= ~_BV(CS11); } while(0)

#define CHAN_SQ1   0
#define CHAN_SQ2   1
#define CHAN_TRI   2
#define CHAN_NOISE 3


typedef struct
{
    uint16_t period;                         // Square + Noise + Triangle
    volatile uint8_t enabled;                // Square + Noise + Triangle
    uint8_t length_counter_halt_flag;        // Square + Noise + Triangle
    volatile uint8_t length_counter;         // Square + Noise + Triangle

    union {
        volatile uint8_t volume;             // Square + Noise
        volatile uint8_t linear_counter;     // Triangle
    };

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

    union {
        volatile uint8_t duty_cycle;         // Square
        volatile uint8_t shift_mode;         // Noise
    };

    union {
        uint16_t sweep_period;               // Square
        uint16_t shift_register;             // Noise TODO: does this need volatile?
    };

    uint8_t sweep_divider;                   // Square
    uint8_t sweep_counter;                   // Square
    uint8_t sweep_neg_flag;                  // Square
    uint8_t sweep_reload_flag;               // Square
    uint8_t sweep_enabled;                   // Square
    
} channel_t;

channel_t channel;

const uint8_t length_lut[] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

const uint16_t noise_period_lut[] = {
    0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0a0,
    0x0ca, 0x0fe, 0x17c, 0x1fc, 0x2fa, 0x3f8, 0x7f2, 0xfe4
};

const uint8_t duty_tab[] = {2, 4, 8, 2};


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

uint8_t conf;
volatile uint8_t frame_flag;

void write_reg_sq1(uint8_t address, uint8_t val)
{
    switch(address)
    {
    case 0x00:
        /*
          0-3   volume / envelope decay rate
          4     envelope decay disable
          5     length counter clock disable / envelope decay looping enable (hold note)
          6-7   duty cycle type (unused on noise channel)
        */

        channel.env_reload_value = val & 0x0f;

        channel.env_const_flag = val & 0x10;
        channel.length_counter_halt_flag = val & 0x20;

        channel.duty_cycle = duty_tab[val >> 6];

        break;

        // 0x4001 & 0x4005
    case 0x01:
        /*
          0-2   right shift amount
          3     decrease / increase (1/0) wavelength
          4-6   sweep update rate
          7     sweep enable
        */

        break;

        // 0x4002 & 0x4006
    case 0x02:
        /*
          0-7   8 LSB of wavelength
        */
        channel.period = (channel.period & 0xFF00) | val;


        OCR1A = channel.period;
        break;

        // 0x4003 & 0x4007
    case 0x03:
        /*
          0-2   3 MS bits of wavelength (unused on noise channel)
          3-7   length counter load register
        */
        channel.period = (((uint16_t) (val & 0x07) ) << 8) | (channel.period & 0x00FF);

        channel.length_counter = length_lut[val >> 3];
        channel.env_reload_flag = 1;

        OCR1A = channel.period;
        if(channel.period < 8)
        {
            timer1_stop();
        } else {
            timer1_start();
        }
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
        // 0x4000 & 0x4004
    case 0x04:
        /*
          0-3   volume / envelope decay rate
          4     envelope decay disable
          5     length counter clock disable / envelope decay looping enable (hold note)
          6-7   duty cycle type (unused on noise channel)
        */

        channel.env_reload_value = val & 0x0f;

        channel.env_const_flag = val & 0x10;
        channel.length_counter_halt_flag = val & 0x20;

        channel.duty_cycle = duty_tab[val >> 6];

        break;

        // 0x4001 & 0x4005
    case 0x05:
        /*
          0-2   right shift amount
          3     decrease / increase (1/0) wavelength
          4-6   sweep update rate
          7     sweep enable
        */

        break;

        // 0x4002 & 0x4006
    case 0x06:
        /*
          0-7   8 LSB of wavelength
        */
        channel.period = (channel.period & 0xFF00) | val;
        OCR1A = channel.period;
        
        break;

        // 0x4003 & 0x4007
    case 0x07:

        /*
          0-2   3 MS bits of wavelength (unused on noise channel)
          3-7   length counter load register
        */
        channel.period = (((uint16_t) (val & 0x07) ) << 8) | (channel.period & 0x00FF);

        channel.length_counter = length_lut[val >> 3];
        channel.env_reload_flag = 1;

        OCR1A = channel.period;
        if(channel.period < 8)
        {
            timer1_stop();
        } else {
            timer1_start();
        }
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

        channel.length_counter_halt_flag = val >> 7;

        channel.linear_counter_reload_value = val & 0x7f;
        break;

    case 0x0A:
        /*
          0-7   8 LSB of wavelength
        */

        channel.period = (channel.period & 0xFF00) | val;
        break;

    case 0x0B:
        /*
          0-2   3 MS bits of wavelength (unused on noise channel)
          3-7   length counter load register
        */
        channel.period = (((uint16_t) (val & 0x07) ) << 8) | (channel.period & 0x00FF);
        channel.length_counter = length_lut[val >> 3];

        channel.linear_counter_reload_flag = 1;

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
        channel.shift_mode = val & 0x80;
        channel.period = noise_period_lut[val & 0x0F];

        OCR1A = channel.period;

        timer1_start(); // ???
        break;

    case 0x0F:
        /*
          4-7  length counter
         */
        channel.length_counter = length_lut[val >> 3];
        channel.env_reload_flag = 1;
        break;

    case 0x15:
        channel.enabled = val & 0x08;
        break;
    }
}

static uint8_t frame_counter;
void frame_update_sq()
{
    if(frame_counter++ & 0x01) // clock length counters and sweep units
    {
	if(!channel.length_counter_halt_flag && channel.length_counter > 0)
	{
	    channel.length_counter--;
	}

	// TODO: clock sweep counter
    }


    // clock envelopes

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

    if(!channel.length_counter)
    {
        channel.volume = 0;
    } else if(channel.env_const_flag) {
	channel.volume = channel.env_reload_value;
    } else {
	channel.volume = channel.env_volume;
    }
}

void frame_update_noise()
{
    if(frame_counter++ & 0x01) // clock length counters and sweep units
    {
	if(!channel.length_counter_halt_flag && channel.length_counter > 0)
	{
	    channel.length_counter--;
	}

	// TODO: clock sweep counter
    }


    // clock envelopes

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

    if(!channel.length_counter)
    {
        channel.volume = 0;
    } else if(channel.env_const_flag) {
	channel.volume = channel.env_reload_value;
    } else {
	channel.volume = channel.env_volume;
    }
}


void frame_update_tri()
{
    if(frame_counter++ & 0x01) // clock length counters and sweep units
    {
	if(!channel.length_counter_halt_flag && channel.length_counter > 0)
	{
	    channel.length_counter--;
	}
    }

    // clock triangle's linear counter
    
    if(channel.linear_counter_reload_flag)
    {
	channel.linear_counter = channel.linear_counter_reload_value;
    } else if(channel.linear_counter > 0) {
	channel.linear_counter--;
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
//    set_outputs(PINS_DAC);

    set_inputs(PINS_BUS);
    disable_pullups(PINS_BUS);

    set_input(PIN_DCLK);
    disable_pullup(PIN_DCLK);
    
    set_input(PIN_FCLK);
    disable_pullup(PIN_FCLK);

    set_output(PIN_LED);

    set_input(PIN_CONF0);
    set_input(PIN_CONF1);

    DDRC = 0x1F;

    enable_pullup(PIN_CONF0);
    enable_pullup(PIN_CONF1);

    //PINS_DAC_DDR |= 0x0F;
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

int main()
{

    io_init();
    timers_init();
    interrupts_init();

    cbuf_init(reg_address);
    cbuf_init(reg_data);

    conf = (is_high(PIN_CONF1) ? 2 : 0) | (is_high(PIN_CONF0) ? 1 : 0);

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

        channel.shift_register = 0x0001;
        break;
    }

    sei();

    for(uint8_t n = 0; n <= conf; n++)
    {
        set_high(PIN_LED);
        _delay_ms(125);
        set_low(PIN_LED);
        _delay_ms(250);
    }

    sei();    

    for(;;)
    {
        while(!cbuf_empty(reg_data))
        {
            uint8_t address = cbuf_pop(reg_address);
            uint8_t val = cbuf_pop(reg_data);

            write_reg(address, val);
        }
        
        if(frame_flag)
        {
            frame_flag = 0;
            frame_update();
        }
    }
}

ISR(TIMER1_COMPA_vect)
{
    set_high(PIN_LED);

    static uint8_t step = 0;


    switch(conf)
    {
    case CHAN_SQ1:
    case CHAN_SQ2:
    {
        step = (step+1) & 0x0F;
        
        if(step < channel.duty_cycle)
        {
            PINS_DAC_PORT |= channel.volume;
        } else {
            PINS_DAC_PORT &= 0xF0;
        }
    }
    break;

    case CHAN_TRI:
    {
        if((channel.linear_counter > 0) && (channel.length_counter > 0))
        {
            step = (step + 1) & 0x1F;

            uint8_t val = step;
        
            if(step & 0x10)
            {
                val = ~step;
            }

            PINS_DAC_PORT = (PINS_DAC_PORT & 0xF0) | (val & 0x0F);
        }
    }
    break;

    case CHAN_NOISE:
    default:
    {
        uint8_t feedback;

        
        if(channel.shift_mode)
        {
            feedback = ((channel.shift_register & _BV(0)) ^ ((channel.shift_register & _BV(6)) >> 6));
        } else {
            feedback = ((channel.shift_register & _BV(0)) ^ ((channel.shift_register & _BV(1)) >> 1));
        }

        channel.shift_register >>= 1;

        if(feedback)
        {
            channel.shift_register |= 0x4000;
        }

        
        if(!(channel.shift_register & _BV(0)))
        {
            PINS_DAC_PORT |= channel.volume;
        } else {
            PINS_DAC_PORT &= 0xF0;
        }
    }
    break;
    }

    set_low(PIN_LED);
}


// High: address, Low: data

ISR(PCINT0_vect)
{
    if(is_high(PIN_DCLK))
    {
        cbuf_push(reg_address, PINS_BUS_PIN);
    } else {
        cbuf_push(reg_data, PINS_BUS_PIN);
    }
}


// TODO: Use flag in GPIOR0?

ISR(PCINT1_vect)
{
    frame_flag = 1;

//    toggle(PIN_LED);
}
