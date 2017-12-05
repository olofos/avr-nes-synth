;;; avr-gcc -Wall -mmcu=atmega88p -x assembler-with-cpp  -Wl,--section-start=.text=0x1800 -nostartfiles  -nodefaultlibs  -nostdlib main.asm -o main.elf
        #include <avr/io.h>
        #include "../channel/config.h"
        #include "../lib/stk500.h"
        #define IO(X) _SFR_IO_ADDR (X)

        temp1   =       16
        count   =       17
        zero    =       15

        .section .text

        .macro  wait_for_clock_hi
1:      sbis    IO(PIN_DCLK_PIN), PIN_DCLK
        rjmp    1b
        .endm

        .macro  wait_for_clock_lo
1:      sbic    IO(PIN_DCLK_PIN), PIN_DCLK
        rjmp    1b
        .endm

        .macro  wait_spm_busy
1:      in      temp1, IO(SPMCSR)
        sbrc    temp1, SELFPRGEN
        rjmp    1b
        .endm

        .global main
main:

;;; Turn off watchdog
        wdr
        in      temp1, IO(MCUSR)
        andi    temp1, (0xFF & _BV(WDRF))
        out     IO(MCUSR), temp1

        lds     temp1, WDTCSR
        ori     temp1, _BV(WDCE) | _BV(WDE)
        sts     WDTCSR, temp1

        clr     temp1
        sts     WDTCSR, temp1
        
;;; Check if we should run the bootloader
        clr     zero
        out     IO(PINS_BUS_DDR), zero
        in      temp1, IO(PINS_BUS_PIN)
        and     temp1, temp1
        brne    get_command
        rjmp    0x0000

;;; Set up IO ports
        out     IO(PINS_BUS_PORT), zero
        cbi     IO(PIN_DCLK_DDR), PIN_DCLK
        cbi     IO(PIN_DCLK_PORT), PIN_DCLK
        cbi     IO(PIN_FCLK_DDR), PIN_FCLK
        cbi     IO(PIN_FCLK_PORT), PIN_FCLK
        cbi     IO(PIN_CONF0_DDR), PIN_CONF0
        sbi     IO(PIN_CONF0_PORT), PIN_CONF0
        cbi     IO(PIN_CONF1_DDR), PIN_CONF1
        sbi     IO(PIN_CONF1_PORT), PIN_CONF1
        
get_command:
        wait_for_clock_lo
        wait_for_clock_hi
        in      temp1, IO(PINS_BUS_PIN)
        cpi     temp1, STK_PROG_PAGE
        breq    prog_page
        cpi     temp1, STK_READ_PAGE
        breq    read_page

load_address:
        wait_for_clock_lo
        wait_for_clock_hi
        in      ZL, IO(PINS_BUS_PIN)
        wait_for_clock_lo
        wait_for_clock_hi
        in      ZH, IO(PINS_BUS_PIN)
        rjmp    get_command

prog_page:
        wait_spm_busy
;;; Fill flash buffer
        ldi     count, (SPM_PAGESIZE >> 1)
        
write_loop:
        wait_for_clock_lo
        wait_for_clock_hi
        in      r0, IO(PINS_BUS_PIN)
        wait_for_clock_lo
        wait_for_clock_hi
        in      r1, IO(PINS_BUS_PIN)
        out     IO(SPMCSR), _BV(SELFPRGEN)
        spm
        adiw    Z, 2
        dec     count
        brne    write_loop

;;; Assert flag
        sbi     IO(PIN_FCLK_DDR), PIN_FCLK

;;; Execute page write
        subi    ZL, SPM_PAGESIZE
        out     IO(SPMCSR), _BV(PGWRT) | _BV(SELFPRGEN)
        spm

;;; Re-enable RWW section
        out     IO(SPMCSR), _BV(RWWSRE) | _BV(SELFPRGEN)
        spm        
        
;;; Release flag
        cbi     IO(PIN_FCLK_DDR), PIN_FCLK

        rjmp    get_command
        
read_page:
        ldi     count, SPM_PAGESIZE
read_page_loop: 
        lpm     temp1, Z+
        wait_for_clock_lo
        out     IO(PINS_BUS_DDR), temp1
        wait_for_clock_hi
        dec     count
        brne    read_page_loop
        rjmp    get_command
        
