#ifndef REGISTERS_H_
#define REGISTERS_H_

#ifndef __ASSEMBLER__

// Square + Noise
register uint8_t channel_volume asm("r2");
// Triangle
#define channel_linear_counter channel_volume

// Square + Noise + Triangle
register uint8_t channel_length_counter asm("r3");

// Noise
register uint8_t shift_register_lo asm("r4");
register uint8_t shift_register_hi asm("r5");

// Square
#define channel_duty_cycle shift_register_lo
// Square + Triangle
#define channel_step shift_register_hi

#else

#define channel_volume         r2
#define channel_linear_counter r2
#define channel_length_counter r3
#define channel_duty_cycle     r4
#define channel_step           r5
#define shift_register_lo      r4
#define shift_register_hi      r5

#define temp2                  r6

#endif

#define channel_shift_mode GPIOR0
#define channel_conf       GPIOR0
#define frame_flag         GPIOR0

#endif
