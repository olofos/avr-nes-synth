#ifndef I2C_H_
#define I2C_H_

#include <avr/pgmspace.h>

#define I2C_OK 0
#define I2C_ERROR 1


#define I2C_READ 1
#define I2C_WRITE 0


void i2c_slave_init(uint8_t address);

// To be implemented by the slave

extern void i2c_receive_start();
extern void i2c_receive(uint8_t);
extern void i2c_receive_stop();
extern void i2c_transmit_start();
extern uint8_t i2c_transmit_data();

#endif
