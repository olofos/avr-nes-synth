#ifndef CONFIG_H_
#define CONFIG_H_

// GPIOR0 bits

#define CONF0_BIT          0
#define CONF1_BIT          1
#define FRAME_FLAG_BIT     2
#define SHIFT_MODE_BIT     7

#ifdef AVR

// Audio out

#define PINS_DAC_PORT      PORTC
#define PINS_DAC_DDR       DDRC

#define PIN_DAC0           PORTC0
#define PIN_DAC1           PORTC1
#define PIN_DAC2           PORTC2
#define PIN_DAC3           PORTC3

#define PINS_DAC_MASK      (_BV(PIN_DAC0) | _BV(PIN_DAC1) | _BV(PIN_DAC2) | _BV(PIN_DAC3))


// Bus in

#define PINS_BUS_PORT      PORTD
#define PINS_BUS_PIN       PIND
#define PINS_BUS_DDR       DDRB

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

#define PIN_FCLK           PORTC5
#define PIN_FCLK_PORT      PORTC
#define PIN_FCLK_PIN       PINC
#define PIN_FCLK_DDR       DDRC

// LED

#define PIN_LED           PORTC4
#define PIN_LED_PORT      PORTC
#define PIN_LED_PIN       PINC
#define PIN_LED_DDR       DDRC

// Config bits

#define PIN_CONF0           PORTB1
#define PIN_CONF0_PORT      PORTB
#define PIN_CONF0_PIN       PINB
#define PIN_CONF0_DDR       DDRB

#define PIN_CONF1           PORTB2
#define PIN_CONF1_PORT      PORTB
#define PIN_CONF1_PIN       PINB
#define PIN_CONF1_DDR       DDRB

#endif
#endif
