/*
 * modemcore.c - 2G and 3G modem core ELF dumper
 *
 * Copyright (C) 2015 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Parts based on mem.c and kcore.c, copyright:
 *      Jeremy Fitzhardinge <jeremy@sw.oz.au>
 *      David Howells <David.Howells@nexor.co.uk>
 *      Tigran Aivazian <tigran@veritas.com>
 *      Linus Torvalds
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/user.h>
#include <linux/capability.h>
#include <linux/elf.h>
#include <linux/elfcore.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/memory.h>
#include <asm/sections.h>


#ifndef ELF_CORE_EFLAGS
#define ELF_CORE_EFLAGS	0
#endif

#define ELF_XCPU_EFLAGS 0x48f1001  // oabi / xcpu

struct modem_pa_map {
	char name[8];
	off_t addr;    /* Address/offset as seen by modem cpu */
	off_t pa_addr; /* Address/offset as seen by AP */
	size_t size;   /* Size of area */
};

struct cpu_config {
	struct proc_dir_entry *proc;
	const struct modem_pa_map *map;
	int nphdr;         /* Number of program headers in the ELF file */
	size_t elf_buflen; /* ELF core header size */
	size_t size;       /* ELF core file size */
	int arch;          /* EM_MIPS or EM_ARM */
	int eflags;        /* ELF flags */
};

#define XCPU_SEGMENTLIST_PA 0x0
#define WCPU_SEGMENTLIST_PA 0x0

// A '*' in the first position of the section name mark the section as 16 bit
#define MEMORY_IS_16BIT(name) (name[0] == '*')

#define WCPU_DDR_SEGMENTS \
	{ "startup",  0x82400000, 0x8f400000,   0x2000}, /* startup code  */ \
	{ "data",     0x82430000, 0x8f430000,   0x3000}, /* data          */ \
	{ "bss",      0x82900000, 0x8f900000, 0x500000}, /* bss area      */ \
	{ "data_uc",  0xA2438000, 0x8f438000,   0x1000}, /* data uncached */ \

#define WCPU_DDR_ALL \
	{ "wpu_ddr",  0x82400000, 0x8f400000, 0xa00000}, /* DDR 10MB */      \
	{ "data_uc",  0xA2438000, 0x8f438000,   0x1000}, /* data uncached */ \

#define MAP_END   { "" }

static const struct modem_pa_map wcpu_map[] = {
	// Name        WCPU addr  AP physical      Size
	{ "wdmbox",   0xA18b0000, 0x118b0000,   0x8000},
	{ "sys",      0x81c18000, 0x11c18000,  0x28000}, /* sys text and bss */
	WCPU_DDR_ALL
	{ "ring_buf", 0x82e00000, 0x8fe00000, 0x200000}, /* trace ring-buffer 2MB */
	{ "wstk_L1T", 0x81c42000, 0x11c33000,   0x1000}, /* WCPU stack */
	{ "wstk_L1R", 0x81c45000, 0x11c34000,   0x1000}, /* WCPU stack */
	{ "wstk_L1S", 0x81c48000, 0x11c35000,   0x3000}, /* WCPU stack */
	{ "wstk_L2T", 0x81c4d000, 0x11c38000,   0x2000}, /* WCPU stack */
	{ "wstk_L2R", 0x81c51000, 0x11c3a000,   0x2000}, /* WCPU stack */
	{ "wstk_L2C", 0x81c55000, 0x11c3c000,   0x1000}, /* WCPU stack */
	{ "wstk_L2D", 0x81c55000, 0x11c3d000,   0x1000}, /* WCPU stack */
	{ "wstk_DS",  0x81c55000, 0x11c3e000,   0x1000}, /* WCPU stack */
	{ "*wcpuapb", 0x01940000, 0x11940000,  0x20400}, /* WCPU peripherals */
	MAP_END
};

static const struct modem_pa_map xcpu_map[] = {
	// Name        WCPU addr  AP physical      Size
	{ "lpddr",    0x82000000, 0x8f000000, 0x400000}, /* DDR 4MB */
	{ "apsram",   0x00100000, 0x00100000,   0x2000},
	{ "APmbox",   0x818A0000, 0x00200000,   0x2000},
	{ "sram",     0x81c00000, 0x11c00000,  0x18000},
	{ "bbmbox",   0x818b8000, 0x118b8000,    0x800},
	{ "bbsram",   0x81980000, 0x11980000,  0x10000},
	{ "xcpu_rom", 0x81e00000, 0x11e00000,   0x5000},
	{ "bcpu_rom", 0x81e80000, 0x11e80000,  0x28000},
	MAP_END
};

static struct cpu_config wcpu_config = {
	.map = wcpu_map, // TODO: comes from modem image
	.arch = ELF_ARCH,
	.eflags = ELF_CORE_EFLAGS,
};

static struct cpu_config xcpu_config = {
	.map = xcpu_map, // TODO: comes from modem image
	.arch = EM_MIPS,
	.eflags = ELF_XCPU_EFLAGS,
};


static size_t get_mcore_size(const struct modem_pa_map *m, int *nphdr,
                             size_t *elf_buflen)
{
	size_t size;

	*nphdr = 0;
	size = 0;
	while(m->name[0]) {
		size += m->size;
		(*nphdr)++;
		m++;
	}

	*elf_buflen =	sizeof(struct elfhdr) +
			(*nphdr)*sizeof(struct elf_phdr);
	*elf_buflen = PAGE_ALIGN(*elf_buflen);

	return size + *elf_buflen;
}


static void mcore_update_ram(struct cpu_config *cc)
{
	cc->size = get_mcore_size(cc->map, &cc->nphdr, &cc->elf_buflen);
	proc_set_size(cc->proc, cc->size);
}

/*
 * store an ELF coredump header in the supplied buffer
 * nphdr is the number of elf_phdr to insert
 */
static void elf_mcore_store_hdr(struct cpu_config *cc, char *bufp, int nphdr, int dataoff)
{
	struct elf_phdr *phdr;
	struct elfhdr *elf;
	const struct modem_pa_map *m;

	/* setup ELF header */
	elf = (struct elfhdr *) bufp;
	bufp += sizeof(struct elfhdr);

	memcpy(elf->e_ident, ELFMAG, SELFMAG);
	elf->e_ident[EI_CLASS]	= ELF_CLASS;
	elf->e_ident[EI_DATA]	= ELF_DATA;
	elf->e_ident[EI_VERSION]= EV_CURRENT;
	elf->e_ident[EI_OSABI] = ELF_OSABI;
	memset(elf->e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
	elf->e_type	= ET_CORE;
	elf->e_machine	= cc->arch;
	elf->e_version	= EV_CURRENT;
	elf->e_entry	= 0;
	elf->e_phoff	= sizeof(struct elfhdr);
	elf->e_shoff	= 0;
	elf->e_flags	= cc->eflags;
	elf->e_ehsize	= sizeof(struct elfhdr);
	elf->e_phentsize= sizeof(struct elf_phdr);
	elf->e_phnum	= nphdr;
	elf->e_shentsize= 0;
	elf->e_shnum	= 0;
	elf->e_shstrndx	= 0;

	m = cc->map;
	while(m->name[0]) {
		phdr = (struct elf_phdr *) bufp;
		bufp += sizeof(struct elf_phdr);

		phdr->p_type	= PT_LOAD;
		phdr->p_flags	= PF_R|PF_W|PF_X;
		phdr->p_offset	= dataoff;
		phdr->p_vaddr	= (size_t)m->addr;
		phdr->p_paddr	= (size_t)m->addr;
		phdr->p_filesz	= phdr->p_memsz	= m->size;
		phdr->p_align	= PAGE_SIZE;
		dataoff += m->size;
//		dataoff = PAGE_ALIGN(dataoff);
		m++;
	}
}


static inline unsigned long size_inside_page(unsigned long start,
					     unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}

void __weak unxlate_dev_mem_ptr(unsigned long phys, void *addr)
{
}

/*
 * This funcion reads the *physical* memory. The p points directly to the
 * memory location.
 */
static void copy_mem16(short *dest, const short *src16, size_t sz)
{
	const short *end = (const void*)src16 + (sz & ~1);

	while(src16 < end) {
		*dest++ = *src16++;
	}
}

static ssize_t read_mem(void *mem16, char __user *buf,
			size_t count, const void *address)
{
	ssize_t read, sz;
	unsigned long remaining;

	read = 0;
	while (count > 0) {
		const void *src = address+read;
		sz = size_inside_page((unsigned long)src, count);
		if (mem16) {
			copy_mem16(mem16, src, sz);
			src = mem16;
		}
		remaining = copy_to_user(buf, src, sz);
		if (remaining) {
			return -EFAULT;
		}
		read += sz;
		buf += sz;
		count -= sz;
	}

	return read;
}

/*****************************************************************************/
/*
 * Map memory before copy
 */
static const void * map_region(phys_addr_t pa_addr, size_t count, void __iomem **region)
{
	const void *paddr;

	*region = NULL;
	if (0 && valid_phys_addr_range(pa_addr, count)) {
		paddr = xlate_dev_mem_ptr(pa_addr);
	}
	else {
		*region = ioremap(pa_addr, count);
		paddr = (const void *)*region;
	}
	if (!paddr) {
		pr_err("Could not create temp mapping for 0x%08x\n", pa_addr);
	}
	return paddr;
}

void unmap_region(void __iomem *region)
{
	if (region)
		iounmap(region);
}

/*****************************************************************************/
/*
 * read from the ELF header and then kernel memory
 */
static ssize_t
read_mcore(struct file *file, char __user *buffer, size_t count, loff_t *fpos)
{
	ssize_t acc = 0;
	size_t size, tsz;
	size_t dataoff;
	int nphdr;
	unsigned long start;
	struct cpu_config *cc;
	const struct modem_pa_map *m;
	void *temp_page = 0;

	cc = PDE_DATA(file_inode(file));
	nphdr = cc->nphdr;
	dataoff = cc->elf_buflen;
	size = cc->size;

	if (count == 0 || *fpos >= size) {
		return 0;
	}

	/* trim count to not go beyond EOF */
	if (count > size - *fpos)
		count = size - *fpos;

	/* construct an ELF core header if we'll need some of it */
	if (*fpos < dataoff) {
		char * elf_buf;

		tsz = dataoff - *fpos;
		if (count < tsz)
			tsz = count;
		elf_buf = kzalloc(dataoff, GFP_ATOMIC);
		if (!elf_buf) {
			return -ENOMEM;
		}
		elf_mcore_store_hdr(cc, elf_buf, nphdr, dataoff);
		if (copy_to_user(buffer, elf_buf + *fpos, tsz)) {
			kfree(elf_buf);
			return -EFAULT;
		}
		kfree(elf_buf);
		count -= tsz;
		*fpos += tsz;
		buffer += tsz;
		acc += tsz;

		/* leave now if buffer full */
		if (count == 0)
			return acc;
	}

	temp_page = kzalloc(PAGE_SIZE, GFP_ATOMIC);
	if (!temp_page) {
		return -ENOMEM;
	}

	/* Go through the list of segments and return then in order */
	start = 0;
	dataoff = *fpos - cc->elf_buflen;

	m = cc->map;
	while(m->name[0]) {
		off_t soff;
		void __iomem *region;
		const void *paddr;
		void *mem16_temp = 0;

		if (MEMORY_IS_16BIT(m->name)) {
			mem16_temp = temp_page;
		}

		soff = dataoff - start;
		if (soff < m->size) {
			tsz = m->size - soff;
			if (tsz > count)
				tsz = count;

			paddr = map_region(m->pa_addr+soff, tsz, &region);

			tsz = read_mem(mem16_temp, buffer, tsz, paddr);

			unmap_region(region);

			if ((ssize_t)tsz < 0) {
				acc = (ssize_t)tsz;
				break;
			}
			count -= tsz;
			*fpos += tsz;
			dataoff += tsz;
			buffer += tsz;
			acc += tsz;

			/* leave now if buffer full */
			if (count == 0)
				break;
		}
		start += m->size;
		m++;
	}

	kfree(temp_page);
	return acc;
}


static int open_mcore(struct inode *inode, struct file *filp)
{
	struct cpu_config *cc;
	cc = PDE_DATA(inode);

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	if (i_size_read(inode) != cc->size) {
		mutex_lock(&inode->i_mutex);
		i_size_write(inode, cc->size);
		mutex_unlock(&inode->i_mutex);
	}
	return 0;
}


static const struct file_operations proc_mcore_operations = {
	.read		= read_mcore,
	.open		= open_mcore,
	.llseek		= default_llseek,
};

static int __init proc_mcore_init(void)
{
	xcpu_config.proc = proc_create_data("xcpucore", S_IRUSR, NULL,
					 &proc_mcore_operations, &xcpu_config);
	if (!xcpu_config.proc) {
		pr_err("couldn't create /proc/xcpucore\n");
		return 0; /* Always returns 0. */
	}
	wcpu_config.proc = proc_create_data("wcpucore", S_IRUSR, NULL,
					 &proc_mcore_operations, &wcpu_config);
	if (!xcpu_config.proc) {
		pr_err("couldn't create /proc/wcpucore\n");
		return 0; /* Always returns 0. */
	}

#if 0
	ptr = xlate_dev_mem_ptr(XCPU_SEGMENTLIST_PA);
	if (!ptr) goto tear_down;
	memcpy(xcpu_config.map, ptr, sizeof(xcpu_map));
	unxlate_dev_mem_ptr(p, ptr);

	ptr = xlate_dev_mem_ptr(WCPU_SEGMENTLIST_PA);
	if (!ptr) goto tear_down;
	memcpy(wcpu_config.map, ptr, sizeof(wcpu_map));
	unxlate_dev_mem_ptr(p, ptr);
#endif

	/* Update ELF segment lists */
	mcore_update_ram(&xcpu_config);
	mcore_update_ram(&wcpu_config);

	return 0;
}
module_init(proc_mcore_init);

MODULE_AUTHOR("Klaus Pedersen<klauskpedersen@rdamicro.com>");
MODULE_DESCRIPTION("RDA Modem CPU core dumper");
MODULE_LICENSE("GPL");

