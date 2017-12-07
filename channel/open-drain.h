#ifndef OPEN_DRAIN_H
#define OPEN_DRAIN_H

static inline void set_bus(uint8_t val)
{
    PINS_BUS_DDR = ~val;
}

static inline void release_bus(void)
{
    set_inputs(PINS_BUS);
}

static inline void hold_fclk(void)
{
    set_output(PIN_FCLK);
}

static inline void release_fclk(void)
{
    set_input(PIN_FCLK);
}

#endif
