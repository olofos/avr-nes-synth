#ifndef IO_H_
#define IO_H_

#include <avr/io.h>

#define set_output(pin)     do {pin ## _DDR |= _BV(pin);} while(0)
#define set_input(pin)      do {pin ## _DDR &= ~_BV(pin);} while(0)
#define set_pin(pin,val)     do {if(val) {pin ## _PORT |= _BV(pin);} else {pin ## _PORT &= ~_BV(pin);}} while(0)

#define enable_pullup(pin)  do {pin ## _PORT |= _BV(pin);} while(0)
#define disable_pullup(pin) do {pin ## _PORT &= ~_BV(pin);} while(0)

#define set_low(pin)        do {pin ## _PORT &= ~_BV(pin);} while(0)
#define set_high(pin)       do {pin ## _PORT |= _BV(pin);} while(0)
#define toggle(pin)         do {pin ## _PORT ^= _BV(pin);} while(0)

#define is_high(pin)        (pin ## _PIN & _BV(pin))
#define is_low(pin)         (!(pin ## _PIN & _BV(pin)))

#define set_outputs(pins) do {if(pins ## _MASK == 0xFF) pins ## _DDR = 0xFF; else pins ## _DDR |= (pins ## _MASK);} while(0)
#define set_inputs(pins)  do {if(pins ## _MASK == 0xFF) pins ## _DDR = 0x00; else pins ## _DDR &= ~(pins ## _MASK);} while(0)
#define enable_pullups(pins)  do {if(pins ## _MASK == 0xFF) pins ## _PORT = 0xFF; else pins ## _PORT |= (pins ## _MASK);} while(0)
#define disable_pullups(pins) do {if(pins ## _MASK == 0xFF) pins ## _PORT = 0x00; else pins ## _PORT &= ~(pins ## _MASK);} while(0)

#define are_high(pins) (pins ## _PIN & pins ## _MASK)
#define are_low(pins) ((~(pins ## _PIN)) & pins ## _MASK)

inline uint8_t SPI_transfer(const uint8_t data)
{
    // Begin transfer

    SPDR = data;

    // Wait for transfer to finish

    while(!(SPSR & _BV(SPIF)))
    {
    }

    return SPDR;
}



#endif
