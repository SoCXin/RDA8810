#include <common.h>
#include <android/android_boot.h>
#include <asm/mach-types.h>

void creat_atags(unsigned taddr, const char *cmdline,unsigned raddr, unsigned rsize)
{
	unsigned n = 0;
	unsigned *tags = (unsigned *)taddr;

	//ATAG_CORE
	tags[n++] = 2;
	tags[n++] = 0x54410001;

	if(rsize) {
		//ATAG_INITRD2
		tags[n++] = 4;
		tags[n++] = 0x54420005;
		tags[n++] = raddr;
		tags[n++] = rsize;
	}
	if(cmdline && cmdline[0]) {
		const char *src;
		char *dst;
		unsigned len = 0;

		dst = (char*) (tags + n + 2);
		src = cmdline;
		while((*dst++ = *src++)) len++;

		len++;
		len = (len + 3) & (~3);

		// ATAG_CMDLINE
		 tags[n++] = 2 + (len / 4);
		 tags[n++] = 0x54410009;

		 n += (len / 4);
	 }

	// ATAG_NONE
	tags[n++] = 0;
	tags[n++] = 0;
}

void boot_linux(unsigned kaddr, unsigned taddr)
{
	void (*entry)(unsigned,unsigned,unsigned) = (void*) kaddr;

	entry(0, machine_arch_type, taddr);
}
