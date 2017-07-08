#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/pgmspace.h>

#include "../common-config.h"

#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define COPYRIGHT_YEAR "2016-2017"
#define AUTHOR "OOS"

#define BAUD 19200UL
#define SCL_CLOCK  100000UL

#define LOG_USE_I2C
#define LOG_USE_SSD1306


//// Pins /////////////////////////////////////////////////////////////////////

// Bus out

#define PINS_BUS_PORT      PORTD
#define PINS_BUS_PIN       PIND
#define PINS_BUS_DDR       DDRD

#define PIN_BUS0           PORTD0
#define PIN_BUS1           PORTD1
#define PIN_BUS2           PORTD2
#define PIN_BUS3           PORTD3
#define PIN_BUS4           PORTD4
#define PIN_BUS5           PORTD5
#define PIN_BUS6           PORTD6
#define PIN_BUS7           PORTD7

#define PINS_BUS_MASK      (_BV(PIN_BUS0) | _BV(PIN_BUS1) | _BV(PIN_BUS2) | _BV(PIN_BUS3) | _BV(PIN_BUS4) | _BV(PIN_BUS5) | _BV(PIN_BUS6) | _BV(PIN_BUS7))

// Bus clock

#define PIN_DCLK           PORTB0
#define PIN_DCLK_PORT      PORTB
#define PIN_DCLK_PIN       PINB
#define PIN_DCLK_DDR       DDRB

// Frame clock

#define PIN_FCLK           PORTC3
#define PIN_FCLK_PORT      PORTC
#define PIN_FCLK_PIN       PINC
#define PIN_FCLK_DDR       DDRC

// SPI + SD card

#define PIN_SD_CD          PORTB1
#define PIN_SD_CD_PORT     PORTB
#define PIN_SD_CD_PIN      PINB
#define PIN_SD_CD_DDR      DDRB

#define PIN_SD_CS          PORTB2
#define PIN_SD_CS_PORT     PORTB
#define PIN_SD_CS_PIN      PINB
#define PIN_SD_CS_DDR      DDRB

#define PIN_MOSI          PORTB3
#define PIN_MOSI_PORT     PORTB
#define PIN_MOSI_PIN      PINB
#define PIN_MOSI_DDR      DDRB

#define PIN_MISO          PORTB4
#define PIN_MISO_PORT     PORTB
#define PIN_MISO_PIN      PINB
#define PIN_MISO_DDR      DDRB

#define PIN_SCK           PORTB5
#define PIN_SCK_PORT      PORTB
#define PIN_SCK_PIN       PINB
#define PIN_SCK_DDR       DDRB

// I2C

#define PIN_SDA           PORTC4
#define PIN_SDA_PORT      PORTC
#define PIN_SDA_PIN       PINC
#define PIN_SDA_DDR       DDRC

#define PIN_SCL           PORTC5
#define PIN_SCL_PORT      PORTC
#define PIN_SCL_PIN       PINC
#define PIN_SCL_DDR       DDRC

// IO interrupt

#define PIN_IO_INT        PORTC0
#define PIN_IO_INT_PORT   PORTC
#define PIN_IO_INT_PIN    PINC
#define PIN_IO_INT_DDR    DDRC

// LED

#define PIN_LED           PORTC1
#define PIN_LED_PORT      PORTC
#define PIN_LED_PIN       PINC
#define PIN_LED_DDR       DDRC

// Channel reset

#define PIN_CH_RESET      PORTC2
#define PIN_CH_RESET_PORT PORTC
#define PIN_CH_RESET_PIN  PINC
#define PIN_CH_RESET_DDR  DDRC



#endif
