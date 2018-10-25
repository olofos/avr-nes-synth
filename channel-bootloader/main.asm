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
        andi    temp1, ~(_BV(WDRF))
        out     IO(MCUSR), temp1

        lds     temp1, WDTCSR
        ori     temp1, _BV(WDCE) | _BV(WDE)
        clr     zero

        sts     WDTCSR, temp1
        sts     WDTCSR, zero

;;; Check if we should run the bootloader
        out     IO(PINS_BUS_DDR), zero
        in      temp1, IO(PINS_BUS_PIN)
        ldi     count, BOOTLOADER_FLAG
;;; Jump to application if the bus is 0x00
        cpse    temp1, count
        rjmp    try_app_start

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

;;; TODO: add a handshake?

get_command:
        wait_for_clock_lo
        out     IO(PINS_BUS_DDR), zero
        wait_for_clock_hi
        in      temp1, IO(PINS_BUS_PIN)
        cpi     temp1, STK_PROG_PAGE
        breq    prog_page
        cpi     temp1, STK_READ_PAGE
        breq    read_page
        cpi     temp1, STK_LOAD_ADDRESS
        breq    load_address
        rjmp    error_loop

load_address:
        wait_for_clock_lo
        wait_for_clock_hi
        in      ZL, IO(PINS_BUS_PIN)
        wait_for_clock_lo
        wait_for_clock_hi
        in      ZH, IO(PINS_BUS_PIN)
        ;; Convert from word address to byte address
        add     ZL, ZL
        adc     ZH, ZH
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
        com     temp1
        wait_for_clock_lo
        out     IO(PINS_BUS_DDR), temp1
        wait_for_clock_hi
        dec     count
        brne    read_page_loop
        rjmp    get_command

try_app_start:
        // Start app if temp1 = 0x00
        cp      r1, r1
        brne    error_loop
        clr     r30
        clr     r31
        ijmp

error_loop:
        sbi     IO(PIN_LED_DDR), PIN_LED
1:      sbi     IO(PIN_LED_PIN), PIN_LED
        rcall   delay270ms
        cbi     IO(PIN_LED_PIN), PIN_LED
        rcall   delay270ms
        rjmp    1b


delay270ms:
        ldi     temp1, 5
1:      rcall   delay65535
        dec     temp1
        brne    1b
        ret

;;; About 54 ms at 14.318180 MHz
delay65535:
        ldi     ZL, 0xFF
        ldi     ZH, 0xFF
1:      sbiw    ZL, 1
        brne    1b
        ret
