#ifndef IO_H_
#define IO_H_

#include <avr/io.h>

#define set_output(pin)     {pin ## _DDR |= _BV(pin);}
#define set_input(pin)      {pin ## _DDR &= ~_BV(pin);}
#define set_pin(pin,val)     {if(val) {pin ## _PORT |= _BV(pin);} else {pin ## _PORT &= ~_BV(pin);}}

#define enable_pullup(pin)  {pin ## _PORT |= _BV(pin);}
#define disable_pullup(pin) {pin ## _PORT &= ~_BV(pin);}

#define set_low(pin)        {pin ## _PORT &= ~_BV(pin);}
#define set_high(pin)       {pin ## _PORT |= _BV(pin);}
#define toggle(pin)         {pin ## _PORT ^= _BV(pin);}

#define is_high(pin)        (pin ## _PIN & _BV(pin))
#define is_low(pin)         (!(pin ## _PIN & _BV(pin)))

#define set_outputs(pins) {pins ## _DDR |= (pins ## _MASK);}
#define set_inputs(pins)  {pins ## _DDR &= ~(pins ## _MASK);}
#define enable_pullups(pins)  {pins ## _PORT |= (pins ## _MASK);}
#define disable_pullups(pins) {pins ## _PORT &= ~(pins ## _MASK);}


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
