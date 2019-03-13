OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x00100100;
	. = ALIGN(4);
	.text :
	{
		__start = .;
		arch/arm/cpu/armv7/start.o (.text)
			*(.text)
	}
	. = ALIGN(4);
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }
	. = ALIGN(4);
	.data : {
		*(.data)
	}
	. = ALIGN(4);
	. = .;
	. = ALIGN(4);
	__image_copy_end = .;
	_end = .;
	.bss :
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.bss*)
		. = ALIGN(4);
		__bss_end__ = .;
		ASSERT((ABSOLUTE(__bss_end__) < 0x00110000), \
			"too big data in spl, don't exceed intrenal ram");
	}
}
