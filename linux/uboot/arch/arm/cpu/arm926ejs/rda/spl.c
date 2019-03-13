#include <common.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <nand.h>
#include <malloc.h>

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT

DECLARE_GLOBAL_DATA_PTR;
/* Define global data structure pointer to it*/
static gd_t gdata __attribute__ ((section(".data")));
static bd_t bdata __attribute__ ((section(".data")));

#else

void puts(const char *str)
{
	while (*str)
		putc(*str++);
}

void putc(char c)
{
}

#endif /* CONFIG_SPL_LIBCOMMON_SUPPORT */

inline void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;)
		;
}

void board_init_f(ulong dummy)
{
	relocate_code(CONFIG_SPL_STACK, NULL, CONFIG_SPL_TEXT_BASE);
}

void board_init_r(gd_t *id, ulong dummy)
{
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	gd = &gdata;
	gd->bd = &bdata;
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	serial_init();          /* serial communications setup */
	gd->have_console = 1;
#endif
	puts("board_init_r\n");
	while(1)
		;
#if 0
#ifdef CONFIG_SPL_NAND_LOAD
	nand_init();
	puts("Nand boot...\n");
	nand_boot();
#endif
#ifdef CONFIG_SPL_SPI_LOAD
	mem_malloc_init(CONFIG_SYS_TEXT_BASE - CONFIG_SYS_MALLOC_LEN,
			CONFIG_SYS_MALLOC_LEN);

	gd = &gdata;
	gd->bd = &bdata;
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	serial_init();          /* serial communications setup */
	gd->have_console = 1;

	puts("SPI boot...\n");
	spi_boot();
#endif
#endif
}
