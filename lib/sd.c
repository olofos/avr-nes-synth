#include <util/delay.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "io.h"
#include "sd.h"
#include "uart.h"
#include "log.h"



#ifdef PIN_SD_CS


typedef struct 
{
    uint16_t sector_bytes_left;
} sd_data_t;

sd_data_t sd_data;

uint8_t sd_sector_done()
{
    return sd_data.sector_bytes_left == 0;
}

void sd_begin_sector(uint32_t sector)
{
    set_low(PIN_SD_CS);

    sd_command( 17, sector, 0xFF);

    while(SPI_transfer(0xFF) != 0xFE)
        ;

    sd_data.sector_bytes_left = 512;
}

void sd_end_sector()
{
    while(sd_data.sector_bytes_left)
    {
        SPI_transfer(0xFF);
        sd_data.sector_bytes_left--;
    }

    // Skip CRC
    SPI_transfer(0xFF);
    SPI_transfer(0xFF);

    set_high(PIN_SD_CS);
}

uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    cmd |= 0x40;

    SPI_transfer(cmd);
    SPI_transfer(arg >> 24);
    SPI_transfer(arg >> 16);
    SPI_transfer(arg >> 8);
    SPI_transfer(arg);
    SPI_transfer(crc);

    uint8_t ret;
    uint8_t i = 0;
    do
    {
        ret = SPI_transfer(0xFF);
    } while((ret & 0x80) && (++i != 0xFF) );

    return ret;
}

uint32_t sd_read_uint32()
{
    uint8_t byte0 = SPI_transfer(0xFF);
    uint8_t byte1 = SPI_transfer(0xFF);
    uint8_t byte2 = SPI_transfer(0xFF);
    uint8_t byte3 = SPI_transfer(0xFF);

    sd_data.sector_bytes_left -= 4;
    return ((uint32_t) byte3 << 24) | ((uint32_t) byte2 << 16) | ((uint32_t) byte1 << 8) | byte0;
}

uint16_t sd_read_uint16()
{
    uint8_t byte0 = SPI_transfer(0xFF);
    uint8_t byte1 = SPI_transfer(0xFF);

    sd_data.sector_bytes_left -= 2;

    return (byte1 << 8) | byte0;
}

uint8_t sd_read_uint8()
{
    uint8_t byte0 = SPI_transfer(0xFF);

    sd_data.sector_bytes_left--;

    return byte0;
}

void sd_skip_bytes(uint16_t bytes)
{
    while(bytes--)
    {
        sd_read_uint8();
    }
}


void sd_debug_print_16_bytes()
{
    uint8_t buf[16];

    log_puts_P(PSTR("       "));
    for(uint8_t i = 0; i < 16; i++)
    {
        buf[i] = SPI_transfer(0xFF);
        log_put_uint8_hex(buf[i]);
        if(i & 0x01)
        {
            log_puts_P(PSTR(" "));
        }
    }
    log_puts_P(PSTR(" "));
    for(uint8_t i = 0; i < 16; i++)
    {
        log_put_ascii(buf[i]);
    }
    log_puts_P(PSTR("\n"));
}


void sd_init()
{
    set_high(PIN_SD_CS);

    // Idle for 80 clock cycles
    for(uint8_t i = 0; i < 10; i++)
    {
        SPI_transfer(0xFF);
    }

    set_low(PIN_SD_CS);

    while(sd_command( 0, 0x00000000, 0x95) != 0x01)
    {
        _delay_us(100);
    }
    
    sd_command( 8, 0x000001AA, 0x87);

    sd_read_uint32();

    for(uint8_t i = 0; i < 0xFF; i++)
    {
        sd_command(55, 0x00000000, 0xFF);
        uint8_t ret = sd_command(41, 0x40000000, 0xFF);

        if(ret == 0)
            break;
        
        _delay_ms(25);
    }

    set_high(PIN_SD_CS);

    sd_data.sector_bytes_left = 0;
}

#endif
