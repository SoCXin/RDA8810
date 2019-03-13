#ifndef _ANDROID_BOOT_H_
#define _ANDROID_BOOT_H_

void creat_atags(unsigned taddr, const char *cmdline, unsigned raddr, unsigned rsize);
void boot_linux(unsigned kaddr,unsigned taddr);
#endif
