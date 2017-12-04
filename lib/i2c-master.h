#ifndef I2C_H_
#define I2C_H_

#define I2C_OK    0
#define I2C_ERROR 1
#define I2C_BUSY  2
#define I2C_DONE  3

#define I2C_READ 1
#define I2C_WRITE 0

void i2c_master_init(void);

uint8_t i2c_start(uint8_t address, uint8_t rw);
uint8_t i2c_start_wait(uint8_t address, uint8_t rw);
uint8_t i2c_rep_start(uint8_t address, uint8_t rw);
void i2c_stop(void);

uint8_t i2c_write_byte(uint8_t data);
uint16_t i2c_write(const uint8_t *data, uint16_t len);
uint16_t i2c_write_P(const uint8_t *data, uint16_t len);

uint8_t i2c_read_ack(void);
uint8_t i2c_read_nak(void);


#endif
