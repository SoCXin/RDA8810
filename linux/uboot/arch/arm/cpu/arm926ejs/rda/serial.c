#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/reg_uart.h>

DECLARE_GLOBAL_DATA_PTR;

void _serial_set_baudrate(int rate)
{
    hwp_sysCtrlAp->Cfg_Clk_Uart[2] = 0x36;        // 115200 @ 26MHz
    //hwp_sysCtrlAp->Cfg_Clk_Uart[2] = 0x05;        // 921600 @ 26MHz
}

void _serial_enable_rtscts(void)
{
     hwp_uart->ctrl |= UART_AUTO_FLOW_CONTROL;
}

void _serial_disable_rtscts(void)
{
     hwp_uart->ctrl &= ~UART_AUTO_FLOW_CONTROL;
}

void _serial_init(void)
{
    _serial_set_baudrate(CONFIG_BAUDRATE);
    hwp_uart->triggers            = UART_AFC_LEVEL(1); //7 ?

    hwp_uart->ctrl                = UART_ENABLE | UART_DATA_BITS_8_BITS |
        UART_TX_STOP_BITS_1_BIT | UART_PARITY_ENABLE_NO;

    /* Allow reception */
    hwp_uart->CMD_Set             = UART_RTS;
}

void _serial_deinit(void)
{
    hwp_uart->ctrl    = 0;
    hwp_uart->CMD_Clr = UART_RTS;
}

/*
 * Test whether a character is in the RX buffer
 */
int _serial_tstc(const int port)
{
    return (GET_BITFIELD(hwp_uart->status, UART_RX_FIFO_LEVEL));
}

int _serial_getc(const int port)
{
    /* wait for character to arrive */ ;
    while (!(GET_BITFIELD(hwp_uart->status, UART_RX_FIFO_LEVEL)))
        ;

    return (hwp_uart->rxtx_buffer & 0xff);
}

void _serial_putc_hw(const char c, const int port)
{
    // Place in the TX Fifo ?
    while (!(GET_BITFIELD(hwp_uart->status, UART_TX_FIFO_SPACE)))
        ;
    hwp_uart->rxtx_buffer = (u32)c;
}

void _serial_putc(const char c, const int port)
{
    if (c == '\n') {
        _serial_putc_hw('\r', 0); 
    }
    _serial_putc_hw(c, 0);
}

void _serial_puts(const char *s, const int port)
{
    while (*s) {
        _serial_putc(*s++, 0);
    }
}

static int hwflow = 0; /* turned off by default */
int hwflow_onoff(int on)
{
	switch(on) {
	case 0:
	default:
		break; /* return current */
	case 1:
		hwflow = 1;
		_serial_enable_rtscts(); /* turn on */
		break;
	case -1:
		hwflow = 0;
		_serial_disable_rtscts(); /* turn off */
		break;
	}
	return hwflow;
}

int serial_init(void)
{
    //_serial_init(); // already init in boot_test
    return 0;
}

int serial_getc(void)
{
    return _serial_getc(0);
}

int serial_tstc(void)
{
    return _serial_tstc(0);
}

void serial_putc(const char c)
{
    _serial_putc(c, 0);
}

void serial_puts(const char *s)
{
    _serial_puts(s, 0);
}

void serial_setbrg (void)
{
}

