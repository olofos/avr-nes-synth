        #include <avr/io.h>
        #include "config.h"
        #include "registers.h"

        #define temp1 r24
        #define temp2 r25

        .section .text

#ifdef ASMINTERRUPT
        .global TIMER1_COMPA_vect
TIMER1_COMPA_vect:

//        sbi     _SFR_IO_ADDR(PIN_LED_PORT), PIN_LED
        push    temp1
        in      temp1, _SFR_IO_ADDR(SREG)
        push    temp1
        push    temp2
        
        ;; Check MSB of config
        sbis    _SFR_IO_ADDR(GPIOR0), CONF1_BIT
        rjmp    square

        ;; Check LSB of config
not_square:
        sbis    _SFR_IO_ADDR(GPIOR0), CONF0_BIT
        rjmp    triangle
        
noise:  
        mov     temp1, shift_register_lo             ; 1
        bst     shift_register_lo, 6                 ; 1
        sbis    _SFR_IO_ADDR(GPIOR0), SHIFT_MODE_BIT ; 2/1
        bst     shift_register_lo, 1                 ; 1
        ;; the first feedbackbit is now in T

        ;; We don't need to zero temp2 first since we only care about bit 0
        bld     temp2, 0                             ; 1 
        eor     temp1, temp2                         ; 1

        bst     temp1, 0                             ; 1
        bld     shift_register_hi, 7                 ; 1

        lsr     shift_register_hi                    ; 1
        ror     shift_register_lo                    ; 1

        in      temp1, _SFR_IO_ADDR(PINS_DAC_PORT)   ; 1
        andi    temp1, 0xF0                          ; 1
        sbrs    shift_register_lo, 0                 ; 2/1
        or      temp1, channel_volume                ; 1
                                                     ; 14 cycles
        
output: 
        out      _SFR_IO_ADDR(PINS_DAC_PORT), temp1
done:
        pop     temp2
        pop     temp1
        out     _SFR_IO_ADDR(SREG), temp1
        pop     temp1

//        cbi     _SFR_IO_ADDR(PIN_LED_PORT), PIN_LED
        reti

triangle:        
        and     channel_linear_counter, channel_linear_counter
        breq    done
        and     channel_length_counter, channel_length_counter
        breq    done

        inc     channel_step
        mov     temp1, channel_step

        sbrc    temp1, 4
        com     temp1

        in      temp2, _SFR_IO_ADDR(PINS_DAC_PORT)
        andi    temp1, 0x0F
        andi    temp2, 0xF0
        or      temp1, temp2
        rjmp    output

square: 
        ;; channel_step = (channel_step + 1) & 0x0F
        mov     temp1, channel_step
        subi    temp1, 0xFF
        andi    temp1, 0x0F
        mov     channel_step, temp1

        ;; if (channel_step < channel_duty_cycle) output = vol; else output = 0;
        cp      temp1, channel_duty_cycle
        in      temp1, _SFR_IO_ADDR(PINS_DAC_PORT)
        andi    temp1, 0xF0
        brcc    output
        or      temp1, channel_volume
        rjmp    output
        


        .global PCINT1_vect
PCINT1_vect:
        sbi _SFR_IO_ADDR(GPIOR0), FRAME_FLAG_BIT
//        sbi     _SFR_IO_ADDR(PIN_LED_PORT), PIN_LED
        reti

#endif
        
