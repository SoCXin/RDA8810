#include <common.h>
#include <asm/io.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <xyzModem.h>

void _serial_enable_rtscts(void);
#ifdef CONFIG_SPL_CHECK_IMAGE
int check_uimage(unsigned int *buf);
#endif

static int getcxmodem(void) {
	if (tstc())
		return (getc());
	return -1;
}

static ulong load_serial_xmodem (ulong offset)
{
	int size;
	int err;
	int res;
	connection_info_t info;
	char ymodemBuf[1024];
	ulong store_addr = ~0;
	ulong addr = 0;

	size = 0;
	info.mode = xyzModem_xmodem;
	_serial_enable_rtscts();
	mdelay(10);
	res = xyzModem_stream_open (&info, &err);
	if (!res) {
		while ((res =
			xyzModem_stream_read (ymodemBuf, 1024, &err)) > 0) {
			store_addr = addr + offset;
			size += res;
			addr += res;
			memcpy ((char *) (store_addr), ymodemBuf,res);
		}
	} else {
		printf ("%s\n", xyzModem_error (err));
	}

	xyzModem_stream_close (&err);
	xyzModem_stream_terminate (false, &getcxmodem);


	flush_cache (offset, size);
	printf("\nXmodem Download Success.\n");
	printf("Total Size = 0x%08x = %d Bytes\n", size, size);

	return offset;
}

void xmodem_boot(void)
{
	__attribute__((noreturn)) void (*uboot)(void);

	load_serial_xmodem(CONFIG_SYS_XMODEM_U_BOOT_DST);

#ifdef CONFIG_SPL_CHECK_IMAGE
	if (check_uimage((unsigned int*)CONFIG_SYS_XMODEM_U_BOOT_DST)) {
		printf("Xmodem boot failed.\n");
		return;
	}
#endif

	/*
	 * Jump to U-Boot image
	 */
	printf("Running U-Boot ...\n");
	uboot = (void *)CONFIG_SYS_XMODEM_U_BOOT_START;
	(*uboot)();
}
