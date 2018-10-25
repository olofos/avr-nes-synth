#include <util/delay.h>
#include "config.h"
#include "io.h"
#include "open-drain.h"
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
#define PAGESIZE 64

static inline void channel_reset_hold(void)
{
    disable_pullup(PIN_CH_RESET);
    set_output(PIN_CH_RESET);
    set_low(PIN_CH_RESET);
}

static inline void channel_reset_release(void)
{
    set_input(PIN_CH_RESET);
    enable_pullup(PIN_CH_RESET);
}

static void update_init(void)
{
    io_set_uart_mode();
    io_uart_flush();

    set_output(PIN_DCLK);
    set_low(PIN_DCLK);
    set_open_drain();
}

static void update_deinit(void)
{
    io_set_button_mode();

    set_push_pull();
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

static void send_byte(uint8_t c)
{
    set_low(PIN_DCLK);
    set_bus(c);
    _delay_us(10);
    set_high(PIN_DCLK);
    _delay_us(10);
    set_low(PIN_DCLK);
    release_bus();
}

static uint8_t read_byte()
{
    release_bus();
    set_low(PIN_DCLK);
    _delay_us(10);
    set_high(PIN_DCLK);
    _delay_us(10);
    uint8_t c = are_high(PINS_BUS);
    set_low(PIN_DCLK);
    return c;
}

static void update_command_loop(void)
{
    uint8_t done = 0;

    while(!done)
    {
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

            send_byte(STK_LOAD_ADDRESS);
            send_byte(address_lo);
            send_byte(address_hi);
            break;
        }
        case STK_UNIVERSAL:
        {
            // UNIVERSAL command is ignored
            io_uart_skip_bytes(4);
            VERIFY_SPACE;
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

            // TODO: Read from device

            if(len != PAGESIZE)
            {
                // TODO: error
            }

            if(dest_type != 'F')
            {
                // TODO: error
            }

            send_byte(STK_READ_PAGE);

            do {
                uint8_t c = read_byte();
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
            VERIFY_SPACE;
            break;
        }
        if(!done || done == STK_OK)
        {
            io_uart_write_byte(STK_OK);
        }
    }
}

void update_channels(void)
{
    log_puts("Starting channel updater\n");
    ssd1306_clear();
    update_init();

    channel_reset_hold();
    set_bus(BOOTLOADER_FLAG);
    _delay_ms(10);
    channel_reset_release();
    _delay_ms(1);

    update_command_loop();

    channel_reset_hold();
    set_bus(0x00);
    _delay_ms(10);
    channel_reset_release();

    update_deinit();
}
