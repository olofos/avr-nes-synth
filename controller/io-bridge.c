#include <util/delay.h>

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

void io_set_uart_mode(void)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(IO_BRIDGE_TRANSMIT_UART);
    i2c_stop();
}

void io_set_button_mode(void)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(IO_BRIDGE_TRANSMIT_BUTTONS);
    i2c_stop();
}


void io_uart_write_byte(uint8_t c)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_WRITE);
    i2c_write_byte(IO_BRIDGE_RECEIVE_UART);
    i2c_write_byte(c);
    i2c_stop();
}



uint8_t io_uart_read_byte(void)
{
    for(;;)
    {
        i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
        uint8_t len = i2c_read_nak();
        i2c_stop();

        _delay_us(20);

        if(len > 0)
        {
            i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
            i2c_read_ack();
            uint8_t val = i2c_read_nak();
            i2c_stop();

            return val;
        }
    }
}

void io_uart_skip_bytes(uint8_t n)
{
    while(n)
    {
        io_uart_read_byte();
        _delay_us(100);
        n--;
    }
}

#if 0
void io_uart_skip_bytes(uint8_t n)
{
    while(n > 0)
    {
        i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
        uint8_t len = i2c_read_nak();
        i2c_stop();

        if(len > n)
        {
            len = n;
        }

        _delay_us(50);

        if(len > 0)
        {
            set_high(PIN_LED);
            i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
            set_low(PIN_LED);
            while(len > 1)
            {
                i2c_read_ack();
                n--;
                len--;
            }

            i2c_read_nak();
            i2c_stop();
        }

    }
}
#endif

void io_uart_flush(void)
{
    i2c_start_wait(I2C_IO_BRIDGE_ADDRESS, I2C_READ);
    uint8_t len = i2c_read_nak();
    i2c_stop();
    io_uart_skip_bytes(len);
}


uint8_t get_input(void)
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
