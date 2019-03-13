#include <linux/kernel.h>
#include <plat/rda_debug.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/module.h>

unsigned int rda_debug = RDA_DBG_ENTER
			 | RDA_DBG_LEAVE
			 | RDA_DBG_KEYPAD
			 //| RDA_DBG_MACH
			 //| RDA_DBG_ION
			 //| RDA_DBG_MMC
			 //| RDA_DBG_GOUDA
			 ;
EXPORT_SYMBOL(rda_debug);

static int __init rda_debug_setup(char *str)
{
	rda_debug = simple_strtoul(str, NULL, 0);
	return 1;
}
__setup("rdadebug=", rda_debug_setup);

void rda_dump_buf(char *data, size_t len)
{
	char temp_buf[64];
	size_t i, off = 0;

	memset(temp_buf, 0, 64);
	for (i=0; i<len; i++) {
		if(i%8 == 0) {
			sprintf(&temp_buf[off], "  ");
			off += 2;
		}
		sprintf(&temp_buf[off], "%02x ", data[i]);
		off += 3;
		if((i+1)%16 == 0 || (i+1) == len) {
			printk("%s\n", temp_buf);
			memset(temp_buf, 0, 64);
			off = 0;
		}
	}
	printk("\n");
}

void rda_debug_putc(char c)
{
	void __iomem *uart_base = (void __iomem *)RDA_TTY_BASE;
	while (!(readl(uart_base + RDA_TTY_STATUS) & 0x1F00))
		;
	writel((unsigned int)c, uart_base + RDA_TTY_TXRX);
}

static ssize_t rda_debug_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%x\n", rda_debug);
}

static ssize_t rda_debug_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t n)
{
	unsigned int value;

	if (sscanf(buf, "%x", &value) != 1)
		return -EINVAL;

	rda_debug = value;

	return n;
}

static struct kobj_attribute rda_debug_attr =
	__ATTR(rda_debug, 0664, rda_debug_show, rda_debug_store);


int rda_bus_gating = 0;
EXPORT_SYMBOL(rda_bus_gating);

static ssize_t rda_bus_gating_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rda_bus_gating ? "on" : "off");
}

static ssize_t rda_bus_gating_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;

	if (value)
		rda_bus_gating = 1;
	else
		rda_bus_gating = 0;

	return n;
}

static struct kobj_attribute rda_bus_gating_attr =
	__ATTR(rda_bus_gating, 0664, rda_bus_gating_show, rda_bus_gating_store);

int __init rda_debug_init_sysfs(void)
{
	int error = 0;
	error = sysfs_create_file(kernel_kobj, &rda_debug_attr.attr);
	if (error)
		printk(KERN_ERR "sysfs_create_file rda_debug failed: %d\n", error);
	error = sysfs_create_file(kernel_kobj, &rda_bus_gating_attr.attr);
	if (error)
		printk(KERN_ERR "sysfs_create_file rda_bus_gating failed: %d\n", error);
	return error;
}
