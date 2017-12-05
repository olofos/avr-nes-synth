#include <util/delay.h>
#include "config.h"
#include "io.h"
#include "io-bridge.h"
#include "i2c-master.h"
#include "channel_updater.h"
#include "stk500.h"
#include "log.h"
#include "ssd1306.h"

#define OPTIBOOT_MAJVER 6
#define OPTIBOOT_MINVER 2

// ATmega88P
#define TARGET_SIGNATURE_0 0x1E
#define TARGET_SIGNATURE_1 0x93
#define TARGET_SIGNATURE_2 0x0F

static void set_bus(uint8_t val)
{
    PINS_BUS_DDR = ~val;
    PINS_BUS_PORT = val;
}

static void release_bus(void)
{
    PINS_BUS_DDR = 0x00;
    PINS_BUS_PORT = 0xFF;
}

static void update_init(void)
{
    io_set_uart_mode();
    _delay_ms(1);
    io_uart_flush();

    set_inputs(PINS_BUS);
    set_output(PIN_DCLK);
    set_input(PIN_FCLK);

    set_low(PIN_DCLK);
}

static void update_deinit(void)
{
    io_set_button_mode();

    set_outputs(PINS_BUS);
    set_output(PIN_DCLK);
    set_output(PIN_FCLK);
}

static uint8_t verify_space(void)
{
    if(io_uart_read_byte() == STK_CRC_EOP)
    {
        io_uart_write_byte(STK_INSYNC);
        return 1;
    } else {
        io_uart_write_byte(STK_NOSYNC);
        return 0;
    }
}

#define VERIFY_SPACE if(!verify_space()) { done = STK_FAILED; break; }

static void update_command_loop(void)
{
    uint8_t done = 0;

    while(!done)
    {
//        set_low(PIN_LED);
        uint8_t ch = io_uart_read_byte();

        ssd1306_puts("Received: ",0,0);
        ssd1306_putc(ch, 12*6, 0);

        switch(ch)
        {
        case STK_GET_PARAMETER:
        {
            uint8_t param = io_uart_read_byte();

            VERIFY_SPACE;

            if(param == STK_SW_MINOR)
            {
                io_uart_write_byte(OPTIBOOT_MINVER);
            } else if(param == STK_SW_MAJOR) {
                io_uart_write_byte(OPTIBOOT_MAJVER);
            } else {
                /*
                 * GET PARAMETER returns a generic 0x03 reply for
                 * other parameters - enough to keep Avrdude happy
                 */
                io_uart_write_byte(0x03);
            }

            break;
        }
        case STK_SET_DEVICE:
            // SET DEVICE is ignored
            io_uart_skip_bytes(20);
            VERIFY_SPACE;
            break;

        case STK_SET_DEVICE_EXT:
            // SET DEVICE EXT is ignored
            io_uart_skip_bytes(5);
            VERIFY_SPACE;
            break;

        case STK_LOAD_ADDRESS:
        {
            uint8_t address_lo = io_uart_read_byte();
            uint8_t address_hi = io_uart_read_byte();
            VERIFY_SPACE;

            // TODO: send STK_LOAD_ADDRESS command

            break;
        }
        case STK_UNIVERSAL:
        {
            // UNIVERSAL command is ignored
            io_uart_skip_bytes(4);
            _delay_us(100);
            VERIFY_SPACE;
            _delay_us(100);
            io_uart_write_byte(0x00);
            break;
        }

        case STK_PROG_PAGE:
        {
            io_uart_read_byte(); // Length hi == 0
            uint8_t len = io_uart_read_byte();
            uint8_t dest_type = io_uart_read_byte();

            // TODO: Write to device
            do {
                uint8_t c = io_uart_read_byte();
            } while(--len);

            VERIFY_SPACE;

            break;
        }

        case STK_READ_PAGE:
        {
            io_uart_read_byte(); // Length hi == 0
            uint8_t len = io_uart_read_byte();
            uint8_t dest_type = io_uart_read_byte();

            VERIFY_SPACE;

            //

            do {
                uint8_t c = 0xFF;
                io_uart_write_byte(c);
            } while(--len);

            break;
        }

        case STK_READ_SIGN:
            VERIFY_SPACE;

            io_uart_write_byte(TARGET_SIGNATURE_0);
            io_uart_write_byte(TARGET_SIGNATURE_1);
            io_uart_write_byte(TARGET_SIGNATURE_2);
            break;

        case STK_LEAVE_PROGMODE:
            VERIFY_SPACE;

            done = STK_OK;
            break;

        default:
//            set_high(PIN_LED);
            VERIFY_SPACE;
            break;
        }
        _delay_us(100);
        if(!done || done == STK_OK)
        {
            io_uart_write_byte(STK_OK);
        }
        _delay_us(100);
    }
}

void update_channels(void)
{
    log_puts("Starting channel updater\n");
    ssd1306_clear();
    _delay_ms(1);
    update_init();
    update_command_loop();
    update_deinit();
}
