#include <avr/interrupt.h>
#include <util/twi.h>

#include "config.h"
#include "io.h"
#include "i2c-slave.h"

void i2c_slave_init(uint8_t address)
{
    // load address into TWI address register
    TWAR = address << 1;

    // set the TWCR to enable address matching and enable TWI, clear TWINT, enable TWI interrupt
    TWCR = _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWEN);
}


#define TWCR_ACK (_BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA))
#define TWCR_NACK (_BV(TWEN) | _BV(TWIE) | _BV(TWINT))

ISR(TWI_vect){
    switch(TW_STATUS)
    {
    case TW_SR_SLA_ACK:       // 0x60: SLA+W received, ACK returned
        if(i2c_receive_start())
        { // Receive data then send ACK
            TWCR = TWCR_ACK;
        } else { // Receive data then send NACK
            TWCR = TWCR_NACK;
        }
        break;

    case TW_SR_DATA_ACK:      // 0x80: data received, ACK returned
        if(i2c_receive(TWDR))
        { // Receive data then send ACK
            TWCR = TWCR_ACK;
        } else { // Receive data then send NACK
            TWCR = TWCR_NACK;
        }
        break;

    case TW_SR_DATA_NACK:     // 0x88: data received, NACK returned
        i2c_receive(TWDR);
                              // intentional fall through
    case TW_SR_STOP:          // 0xA0: stop or repeated start condition received while selected
        i2c_receive_stop();
        TWCR = TWCR_ACK;
        break;


    case TW_ST_SLA_ACK:       // 0xA8: SLA+R received, ACK returned
        i2c_transmit_start();
                              // intentional fall through
    case TW_ST_DATA_ACK:      // 0xB8: data transmitted, ACK received
    {
        uint8_t data;
        uint8_t ack = i2c_transmit_data(&data);
        TWDR = data;

        if(ack)
        {
            TWCR = TWCR_ACK;
        } else {
            TWCR = TWCR_NACK;
        }
        break;
    }

    case TW_ST_DATA_NACK:     // 0xC0: data transmitted, NACK received
    case TW_ST_LAST_DATA:     // 0xC8: last data byte transmitted, ACK received
    case TW_BUS_ERROR:        // 0x00: illegal start or stop condition
    default:
        TWCR = TWCR_ACK;
    }
}
