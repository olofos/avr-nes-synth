#ifndef CONFIG_H_
#define CONFIG_H_

#include "../common-config.h"

#define uart_tx_cbuf_LEN 256

#define BAUD 38400UL

#define LOG_USE_UART

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

#define PIN_IO_INT        PORTD2
#define PIN_IO_INT_PORT   PORTD
#define PIN_IO_INT_PIN    PIND
#define PIN_IO_INT_DDR    DDRD

// UART

#define PIN_RX            PORTD0
#define PIN_RX_PORT       PORTD
#define PIN_RX_PIN        PIND
#define PIN_RX_DDR        DDRD

#define PIN_TX            PORTD1
#define PIN_TX_PORT       PORTD
#define PIN_TX_PIN        PIND
#define PIN_TX_DDR        DDRD

// Switches

#define PINS_SW_PORT      PORTB
#define PINS_SW_PIN       PINB
#define PINS_SW_DDR       DDRB

#define PIN_SW1           PORTB0
#define PIN_SW1_PORT      PORTB
#define PIN_SW1_PIN       PINB
#define PIN_SW1_DDR       DDRB

#define PIN_SW2           PORTB1
#define PIN_SW2_PORT      PORTB
#define PIN_SW2_PIN       PINB
#define PIN_SW2_DDR       DDRB

#define PIN_SW3           PORTB6
#define PIN_SW3_PORT      PORTB
#define PIN_SW3_PIN       PINB
#define PIN_SW3_DDR       DDRB

#define PIN_SW4           PORTB7
#define PIN_SW4_PORT      PORTB
#define PIN_SW4_PIN       PINB
#define PIN_SW4_DDR       DDRB

#define PINS_SW_MASK      (_BV(PIN_SW1) | _BV(PIN_SW2) | _BV(PIN_SW3) | _BV(PIN_SW4))


// LED:s

#define PINS_LED_PORT      PORTC
#define PINS_LED_PIN       PINC
#define PINS_LED_DDR       DDRC

#define PIN_LED1           PORTC0
#define PIN_LED1_PORT      PORTC
#define PIN_LED1_PIN       PINC
#define PIN_LED1_DDR       DDRC

#define PIN_LED2           PORTC1
#define PIN_LED2_PORT      PORTC
#define PIN_LED2_PIN       PINC
#define PIN_LED2_DDR       DDRC

#define PIN_LED3           PORTC2
#define PIN_LED3_PORT      PORTC
#define PIN_LED3_PIN       PINC
#define PIN_LED3_DDR       DDRC

#define PIN_LED4           PORTC3
#define PIN_LED4_PORT      PORTC
#define PIN_LED4_PIN       PINC
#define PIN_LED4_DDR       DDRC

#define PINS_LED_MASK      (_BV(PIN_LED1) | _BV(PIN_LED2) | _BV(PIN_LED3) | _BV(PIN_LED4))

#endif
