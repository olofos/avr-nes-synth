#ifndef NES_APU_CHANNEL_H_
#define NES_APU_CHANNEL_H_

#define CHAN_SQ1   0
#define CHAN_SQ2   1
#define CHAN_TRI   2
#define CHAN_NOISE 3

typedef struct
{
    union
    {
        uint16_t period;                     // Square + Noise + Triangle
        struct
        {
            uint8_t period_lo;
            uint8_t period_hi;
        };
    };

    volatile uint8_t enabled;                // Square + Noise + Triangle
    uint8_t length_counter_halt_flag;        // Square + Noise + Triangle

#ifndef AVR // moved to register
    volatile uint8_t length_counter;         // Square + Noise + Triangle

    union {
        volatile uint8_t volume;             // Square + Noise
        volatile uint8_t linear_counter;     // Triangle
    };
#endif

    union {
        uint8_t env_reload_flag;             // Square + Noise
        uint8_t linear_counter_reload_flag;  // Triangle
    };

    union {
        uint8_t env_reload_value;            // Square + Noise
        uint8_t linear_counter_reload_value; // Triangle
    };

    uint8_t env_const_flag;                  // Square + Noise
    uint8_t env_divider;                     // Square + Noise
    uint8_t env_volume;                      // Square + Noise

#ifndef AVR // moved to register & GPIOR0
    union {
        volatile uint8_t duty_cycle;         // Square
        volatile uint8_t shift_mode;         // Noise
    };
#endif

#ifndef AVR // moved to register
    uint16_t shift_register;             // Noise
#endif

    uint8_t sweep_div_period;                // Square
    uint8_t sweep_div_counter;               // Square
    uint8_t sweep_shift;
    uint8_t sweep_neg_flag;                  // Square
    uint8_t sweep_reload_flag;               // Square
    uint8_t sweep_enabled;                   // Square

    uint8_t frame_counter;

#ifndef AVR
    uint8_t conf;
    uint8_t step;
    uint8_t muted;
    uint16_t reload_period;
    int16_t sweep_target_period;
    double phase;
#endif
} channel_t;

extern volatile uint8_t wave_buf[32] __attribute__ ((aligned (0x100)));

#ifndef AVR
extern channel_t *current_channel;
#define channel (*current_channel)
#endif

void write_reg_sq1(uint8_t address, uint8_t val);
void write_reg_sq2(uint8_t address, uint8_t val);
void write_reg_tri(uint8_t address, uint8_t val);
void write_reg_noise(uint8_t address, uint8_t val);

void frame_update_sq(void);
void frame_update_tri(void);
void frame_update_noise(void);

#ifndef AVR
void channel_timer_start(void);
void channel_timer_stop(void);
void channel_timer_set_period(uint16_t period);

#define _BV(n) (1u << (n))
#endif


#endif
