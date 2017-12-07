#ifndef OPEN_DRAIN_H
#define OPEN_DRAIN_H


static inline void set_bus(uint8_t val)
{
    PINS_BUS_PORT = val;
    PINS_BUS_DDR = ~val;
}

static inline void release_bus(void)
{
    PINS_BUS_DDR = 0x00;
    PINS_BUS_PORT = 0xFF;
}

static inline void hold_fclk(void)
{
    disable_pullup(PIN_FCLK);
    set_output(PIN_FCLK);
}

static inline void release_fclk(void)
{
    set_input(PIN_FCLK);
    enable_pullup(PIN_FCLK);
}

static inline void set_open_drain(void)
{
    release_bus();
    release_fclk();
}

static inline void set_push_pull(void)
{
    set_outputs(PINS_BUS);
    set_output(PIN_FCLK);
}

#endif
