#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/twi.h>

#include "config.h"
#include "io.h"
#include "i2c-master.h"

#ifdef SCL_CLOCK

void i2c_master_init(void)
{
    // no prescaler
    TWSR = 0;

    // must be > 10 for stable operation
    TWBR = ((F_CPU/SCL_CLOCK)-16)/2;

#if (F_CPU / SCL_CLOCK) <= 36
#warning "I2C clock frequancy is too high"
#endif
}


static inline void i2c_wait_for_complete(void)
{
    while(!(TWCR & _BV(TWINT)))
        ;
}

static inline void i2c_wait_for_stop(void)
{
    while(TWCR & _BV(TWSTO))
        ;
}


static uint8_t i2c_send_start(void)
{
    uint8_t   twst;

    // send START condition
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

    // wait until transmission completed
    i2c_wait_for_complete();

    // check value of TWI Status Register
    twst = TW_STATUS;

    if((twst != TW_START) && (twst != TW_REP_START))
        return I2C_ERROR;

    return I2C_OK;
}

static uint8_t i2c_send_address(uint8_t address, uint8_t rw)
{
    uint8_t   twst;

    // send device address
    TWDR = (address << 1) | rw;
    TWCR = _BV(TWINT) | _BV(TWEN);

    // wail until transmission completed and ACK/NACK has been received
    i2c_wait_for_complete();

    // check value of TWI Status Register
    twst = TW_STATUS;

    switch(twst)
    {
    case TW_MT_SLA_NACK:
    case TW_MR_DATA_NACK:
        // device busy, send stop condition to terminate write operation
        i2c_stop();
        return I2C_BUSY;
    case TW_MT_SLA_ACK:
    case TW_MR_SLA_ACK:
        return I2C_OK;
    default:
        return I2C_ERROR;
    }
}

uint8_t i2c_start(uint8_t address, uint8_t rw)
{
    if(i2c_send_start() != I2C_OK)
    {
        return I2C_ERROR;
    }

    return i2c_send_address(address, rw);
}


uint8_t i2c_start_wait(uint8_t address, uint8_t rw)
{
    uint8_t ret;

    for(;;)
    {
        ret = i2c_send_start();

        if(ret == I2C_OK)
        {
            ret = i2c_send_address(address, rw);

            if(ret != I2C_BUSY)
            {
                break;
            }
        }
    }

    return ret;
}


uint8_t i2c_rep_start(uint8_t address, uint8_t rw)
{
    return i2c_start(address , rw);
}


void i2c_stop(void)
{
    // send stop condition
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

    // wait until stop condition is executed and bus released
    i2c_wait_for_stop();
}


uint8_t i2c_write_byte(uint8_t data)
{
    uint8_t   twst;

    // send data to the previously addressed device
    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);

    // wait until transmission completed
    i2c_wait_for_complete();

    // check value of TWI Status Register
    twst = TW_STATUS;

    switch(twst)
    {
    case TW_MT_DATA_ACK:
        return I2C_OK;
    case TW_MT_DATA_NACK:
        return I2C_DONE;
    default:
        return I2C_ERROR;
    }
}


uint16_t i2c_write(const uint8_t *data, uint16_t len)
{
    uint8_t status = I2C_OK;

    while((len > 0) && (status == I2C_OK))
    {
        status = i2c_write_byte(*data++);
        len--;
    }

    return len;
}

uint16_t i2c_write_P(const uint8_t *data, uint16_t len)
{
    uint8_t status = I2C_OK;

    while((len > 0) && (status == I2C_OK))
    {
        status = i2c_write_byte(pgm_read_byte(data++));
        len--;
    }

    return len;
}


uint8_t i2c_read_ack(void)
{
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
    i2c_wait_for_complete();

    return TWDR;
}

uint8_t i2c_read_nak(void)
{
    TWCR = _BV(TWINT) | _BV(TWEN);
    i2c_wait_for_complete();

    return TWDR;
}

#endif
