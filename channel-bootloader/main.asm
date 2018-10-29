;;; avr-gcc -Wall -mmcu=atmega88p -x assembler-with-cpp  -Wl,--section-start=.text=0x1800 -nostartfiles  -nodefaultlibs  -nostdlib main.asm -o main.elf
        #include <avr/io.h>
        #include "../channel/config.h"
        #include "../lib/stk500.h"
        #define IO(X) _SFR_IO_ADDR (X)

        #if SPM_PAGESIZE >= 256
        #error Pagesize too large
        #endif

        temp1   =       16
        count   =       17
        zero    =       15

        ;; Assume page_buffer contained within one 256 byte page
        .lcomm 	page_buffer, SPM_PAGESIZE

        .section .text

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

get_command:
        rcall   wait_for_clock_lo
        out     IO(PINS_BUS_DDR), zero
        rcall   wait_for_clock_hi
        in      temp1, IO(PINS_BUS_PIN)
        cpi     temp1, STK_PROG_PAGE
        breq    prog_page
        cpi     temp1, STK_READ_PAGE
        breq    read_page
        cpi     temp1, STK_LOAD_ADDRESS
        breq    load_address
        rjmp    error_loop

load_address:
        rcall   wait_for_clock_lo
        rcall   wait_for_clock_hi
        in      ZL, IO(PINS_BUS_PIN)
        rcall   wait_for_clock_lo
        rcall   wait_for_clock_hi
        in      ZH, IO(PINS_BUS_PIN)
        ;; Convert from word address to byte address
        add     ZL, ZL
        adc     ZH, ZH
        rjmp    get_command

read_page:
	ldi     count, SPM_PAGESIZE
read_page_loop:
	lpm     temp1, Z+
	com     temp1
	rcall   wait_for_clock_lo
	out     IO(PINS_BUS_DDR), temp1
	rcall   wait_for_clock_hi
	dec     count
	brne    read_page_loop
	rjmp    get_command

prog_page:
        ldi     count, SPM_PAGESIZE
        ldi     YL, lo8(page_buffer)
        ldi     YH, hi8(page_buffer)

fill_page_buffer_loop:
	rcall   wait_for_clock_lo
	rcall   wait_for_clock_hi
	in      temp1, IO(PINS_BUS_PIN)
        st      Y+, temp1
        dec     count
        brne    fill_page_buffer_loop

;;; Assert flag
	sbi     IO(PIN_FCLK_DDR), PIN_FCLK

;;; Erase page
	rcall   wait_spm_busy
        ldi     temp1, _BV(PGERS) | _BV(SELFPRGEN)
	out     IO(SPMCSR), temp1
	spm

        ;; Assume that page_buffer is contained in one 256 page in SRAM
	ldi     YL, lo8(page_buffer)
	ldi     count, (SPM_PAGESIZE >> 1)

fill_temp_buffer_loop:
        ld      r0, Y+
        ld      r1, Y+

        rcall   wait_spm_busy
        ldi     temp1, _BV(SELFPRGEN)
        out     IO(SPMCSR), temp1
        spm
        subi    ZL, -2
        dec     count
        brne    fill_temp_buffer_loop

;;; Go back to start of page
	subi    ZL, SPM_PAGESIZE

;;; Execute page write
        rcall   wait_spm_busy
        ldi     temp1, _BV(PGWRT) | _BV(SELFPRGEN)
        out     IO(SPMCSR), temp1
        spm

;;; Re-enable RWW section
        rcall   wait_spm_busy
        ldi     temp1,	_BV(RWWSRE) | _BV(SELFPRGEN)
        out     IO(SPMCSR), temp1
        spm

;;; Release flag
        rcall   wait_spm_busy
        cbi     IO(PIN_FCLK_DDR), PIN_FCLK
        rjmp    get_command

wait_for_clock_hi:
1:      sbis    IO(PIN_DCLK_PIN), PIN_DCLK
	rjmp    1b
	ret

wait_for_clock_lo:
1:      sbic    IO(PIN_DCLK_PIN), PIN_DCLK
	rjmp    1b
	ret

wait_spm_busy:
1:      in      temp1, IO(SPMCSR)
	sbrc    temp1, SELFPRGEN
	rjmp    1b
	ret

try_app_start:
        // Start app if temp1 = 0x00
        cp      temp1, temp1
        brne    error_loop
        clr     ZL
        clr     ZH
        ijmp

error_loop:
        sbi     IO(PIN_LED_DDR), PIN_LED
        ;; Toggle LED
1:      sbi     IO(PIN_LED_PIN), PIN_LED
        ;; Delay around 270ms
        ldi     temp1, 5
        ;; Delay around 54 ms at 14.318180 MHz
2:      ldi     ZL, 0xFF
	ldi     ZH, 0xFF
3:      sbiw    ZL, 1
	brne    3b
	dec     temp1
	brne    2b
        rjmp    1b
