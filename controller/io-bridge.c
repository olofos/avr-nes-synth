#include "config.h"
#include "io.h"
#include "i2c-master.h"
#include "io-bridge.h"
#include "main.h"

#define BUTTON_RAW_PRESS_RIGHT 0x80
#define BUTTON_RAW_PRESS_UP    0x40
#define BUTTON_RAW_PRESS_DOWN  0x20
#define BUTTON_RAW_PRESS_LEFT  0x10

#define BUTTON_RAW_RELEASE_RIGHT 0x08
#define BUTTON_RAW_RELEASE_UP    0x04
#define BUTTON_RAW_RELEASE_DOWN  0x02
#define BUTTON_RAW_RELEASE_LEFT  0x01

#define HOLD_TIME 3

uint8_t get_input()
{
    static uint16_t input_time;
    static uint8_t prev;
    
    uint8_t in = 0;
    if(is_high(PIN_IO_INT))
    {

        i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
        in = i2c_read_nak();
        i2c_stop();
    }

    if(in)
    {
        prev = in;
        input_time = global_timer;
    }

    if(in & BUTTON_RAW_PRESS_LEFT)
    {
        return BUTTON_PRESS_LEFT;
    }
    if(in & BUTTON_RAW_PRESS_RIGHT)
    {
        return BUTTON_PRESS_RIGHT;
    }
    if(in & BUTTON_RAW_PRESS_UP)
    {
        return BUTTON_PRESS_UP;
    }
    if(in & BUTTON_RAW_PRESS_DOWN)
    {
        return BUTTON_PRESS_DOWN;
    }

    if(!in)
    {
        if(global_timer - input_time > HOLD_TIME)
        {

            if(prev & BUTTON_RAW_PRESS_LEFT)
            {
                return BUTTON_HOLD_LEFT;
            }
            if(prev & BUTTON_RAW_PRESS_RIGHT)
            {
                return BUTTON_HOLD_RIGHT;
            }
            if(prev & BUTTON_RAW_PRESS_UP)
            {
                return BUTTON_HOLD_UP;
            }
            if(prev & BUTTON_RAW_PRESS_DOWN)
            {
                return BUTTON_HOLD_DOWN;
            }
        }
    }
    
    return 0;
}
