        #include <avr/io.h>
        #include "config.h"
        #include "registers.h"

        #define temp1 r24

        .section .text

#ifdef ASMINTERRUPT
        .global TIMER1_COMPA_vect
TIMER1_COMPA_vect:

        push    temp1
        in      temp1, _SFR_IO_ADDR(SREG)
        push    temp1

        ;; Check MSB of config
        sbis    _SFR_IO_ADDR(channel_conf), CONF1_BIT
        rjmp    square

        ;; Check LSB of config
not_square:
        sbis    _SFR_IO_ADDR(channel_conf), CONF0_BIT
        rjmp    triangle

noise:
        mov     temp1, shift_register_lo             ; 1
        bst     shift_register_lo, 6                 ; 1
        sbis    _SFR_IO_ADDR(channel_shift_mode), SHIFT_MODE_BIT ; 2/1
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
        pop     temp1
        out     _SFR_IO_ADDR(SREG), temp1
        pop     temp1
        reti

triangle:
        and     channel_linear_counter, channel_linear_counter
        breq    done
        and     channel_length_counter, channel_length_counter
        breq    done

        mov     temp1, channel_step                  ; 1
        inc     temp1                                ; 1
        andi    temp1, 0x1F                          ; 1
        mov     channel_step, temp1                  ; 1
        push    r30                                  ; 2
        push    r31                                  ; 2
        ;; We can shorten this if we align wave_buf to a 256 (or 32) byte boundary
        ;;  ldi     r31, hi8(wave_buf)
        ;;  mov     r30, channel_step
        ;; or
        ;;  ldi     r31, hi8(wave_buf)
        ;;  ldi     r30, lo8(wave_buf)
        ;;  add     r30, channel_step

        ldi     r30, lo8(wave_buf)                   ; 1
        ldi     r31, hi8(wave_buf)                   ; 1
        add     r30, channel_step                    ; 1
        adc     r31, r1                              ; 1
        ld      temp1, Z                             ; 2
        pop     r31                                  ; 2
        pop     r30                                  ; 2

        andi    temp1, 0x0F                          ; 1

        ;; Add a two bit volume to the triangle channel
        ;;  sbic    _SFR_IO_ADDR(GPIOR0), VOL_BIT1       ; 1/2
        ;;  lsr     temp1                                ; 1
        ;;  sbic    _SFR_IO_ADDR(GPIOR0), VOL_BIT1       ; 1/2
        ;;  lsr     temp1                                ; 1
        ;;  sbic    _SFR_IO_ADDR(GPIOR0), VOL_BIT0       ; 1/2
        ;;  lsr     temp1                                ; 1

        mov     temp2, temp1                         ; 1
        in      temp1, _SFR_IO_ADDR(PINS_DAC_PORT)   ; 1
        andi    temp1, 0xF0                          ; 1
        or      temp1, temp2                         ; 1
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
        sbi _SFR_IO_ADDR(frame_flag), FRAME_FLAG_BIT
        reti

#endif
