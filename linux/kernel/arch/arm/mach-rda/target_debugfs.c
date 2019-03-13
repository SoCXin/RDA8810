#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/err.h>

#include <mach/gpio_id.h>

const static char *gpio_str[] = {
	[GPIO_A0]	= "GPIO_A0",
	[GPIO_A1]	= "GPIO_A1",
	[GPIO_A2]	= "GPIO_A2",
	[GPIO_A3]	= "GPIO_A3",
	[GPIO_A4]	= "GPIO_A4",
	[GPIO_A5]	= "GPIO_A5",
	[GPIO_A6]	= "GPIO_A6",
	[GPIO_A7]	= "GPIO_A7",
	[GPIO_A8] 	= "GPIO_A8",
	[GPIO_A9]	= "GPIO_A0",
	[GPIO_A10]	= "GPIO_A10",
	[GPIO_A11]	= "GPIO_A11",
	[GPIO_A12]	= "GPIO_A12",
	[GPIO_A13]	= "GPIO_A13",
	[GPIO_A14]	= "GPIO_A14",
	[GPIO_A15]	= "GPIO_A15",
	[GPIO_A16]	= "GPIO_A16",
	[GPIO_A17] 	= "GPIO_A17",
	[GPIO_A18]	= "GPIO_A18",
	[GPIO_A19]	= "GPIO_A19",
	[GPIO_A20]	= "GPIO_A20",
	[GPIO_A21]	= "GPIO_A21",
	[GPIO_A22]	= "GPIO_A22",
	[GPIO_A23]	= "GPIO_A23",
	[GPIO_A24]	= "GPIO_A24",
	[GPIO_A25]	= "GPIO_A25",
	[GPIO_A26] 	= "GPIO_A26",
	[GPIO_A27]	= "GPIO_A27",
	[GPIO_A28]	= "GPIO_A28",
	[GPIO_A29]	= "GPIO_A29",
	[GPIO_A30]	= "GPIO_A30",
	[GPIO_A31]	= "GPIO_A31",
	[GPIO_B0]	= "GPIO_B0",
	[GPIO_B1]	= "GPIO_B1",
	[GPIO_B2]	= "GPIO_B2",
	[GPIO_B3]	= "GPIO_B3",
	[GPIO_B4]	= "GPIO_B4",
	[GPIO_B5]	= "GPIO_B5",
	[GPIO_B6]	= "GPIO_B6",
	[GPIO_B7]	= "GPIO_B7",
	[GPIO_B8] 	= "GPIO_B8",
	[GPIO_B9]	= "GPIO_B0",
	[GPIO_B10]	= "GPIO_B10",
	[GPIO_B11]	= "GPIO_B11",
	[GPIO_B12]	= "GPIO_B12",
	[GPIO_B13]	= "GPIO_B13",
	[GPIO_B14]	= "GPIO_B14",
	[GPIO_B15]	= "GPIO_B15",
	[GPIO_B16]	= "GPIO_B16",
	[GPIO_B17] 	= "GPIO_B17",
	[GPIO_B18]	= "GPIO_B18",
	[GPIO_B19]	= "GPIO_B19",
	[GPIO_B20]	= "GPIO_B20",
	[GPIO_B21]	= "GPIO_B21",
	[GPIO_B22]	= "GPIO_B22",
	[GPIO_B23]	= "GPIO_B23",
	[GPIO_B24]	= "GPIO_B24",
	[GPIO_B25]	= "GPIO_B25",
	[GPIO_B26] 	= "GPIO_B26",
	[GPIO_B27]	= "GPIO_B27",
	[GPIO_B28]	= "GPIO_B28",
	[GPIO_B29]	= "GPIO_B29",
	[GPIO_B30]	= "GPIO_B30",
	[GPIO_B31]	= "GPIO_B31",
	[GPIO_D0]	= "GPIO_D0",
	[GPIO_D1]	= "GPIO_D1",
	[GPIO_D2]	= "GPIO_D2",
	[GPIO_D3]	= "GPIO_D3",
	[GPIO_D4]	= "GPIO_D4",
	[GPIO_D5]	= "GPIO_D5",
	[GPIO_D6]	= "GPIO_D6",
	[GPIO_D7]	= "GPIO_D7",
	[GPIO_D8] 	= "GPIO_D8",
	[GPIO_D9]	= "GPIO_D0",
	[GPIO_D10]	= "GPIO_D10",
	[GPIO_D11]	= "GPIO_D11",
	[GPIO_D12]	= "GPIO_D12",
	[GPIO_D13]	= "GPIO_D13",
	[GPIO_D14]	= "GPIO_D14",
	[GPIO_D15]	= "GPIO_D15",
	[GPIO_D16]	= "GPIO_D16",
	[GPIO_D17] 	= "GPIO_D17",
	[GPIO_D18]	= "GPIO_D18",
	[GPIO_D19]	= "GPIO_D19",
	[GPIO_D20]	= "GPIO_D20",
	[GPIO_D21]	= "GPIO_D21",
	[GPIO_D22]	= "GPIO_D22",
	[GPIO_D23]	= "GPIO_D23",
	[GPIO_D24]	= "GPIO_D24",
	[GPIO_D25]	= "GPIO_D25",
	[GPIO_D26] 	= "GPIO_D26",
	[GPIO_D27]	= "GPIO_D27",
	[GPIO_D28]	= "GPIO_D28",
	[GPIO_D29]	= "GPIO_D29",
	[GPIO_D30]	= "GPIO_D30",
	[GPIO_D31]	= "GPIO_D31",
	[GPO_0]		= "GPO_0",
	[GPO_1]		= "GPO_1",
	[GPO_2]		= "GPO_2",
	[GPO_3]		= "GPO_3",
	[GPO_4]		= "GPO_4",
};

static int tgt_usb_show(struct seq_file *s, void *unused)
{
	int detect_gpio = 1;

	seq_printf(s, "usb detect gpio -%s\n",
			gpio_str[detect_gpio]);

	return 0;
}

static int tgt_usb_open(struct inode *inode, struct file *file)
{
	return single_open(file, tgt_usb_show, NULL);
}

static const struct file_operations tgt_usb_operations = {
	.open		= tgt_usb_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};



static int tgt_gpio_show(struct seq_file *s, void *unused)
{
	/*
	 *TODO: the real value should be get from target code
	 */
	uint32_t gpioa_available = 0xaa;
	uint32_t gpiob_available = 0xbb;
	uint32_t gpiod_available = 0xdd;
	uint32_t gpo_available = 0xee;

	seq_printf(s, "GPIO bitmaps:\n");
	seq_printf(s, "GPIOA -0x%x\n",
			gpioa_available);
	seq_printf(s, "GPIOB -0x%x\n",
			gpiob_available);
	seq_printf(s, "GPIOD -0x%x\n",
			gpiod_available);
	seq_printf(s, "GPO -0x%x\n",
			gpo_available);

	return 0;
}

static int tgt_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, tgt_gpio_show, NULL);
}

static const struct file_operations tgt_gpio_operations = {
	.open		= tgt_gpio_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int __init target_debugfs_init(void)
{
	struct dentry * tgt_root;

	/* /sys/kernel/debug/target */
	tgt_root = debugfs_create_dir("target", NULL);
	if (!tgt_root) {
		pr_warning("cannot create debugfs for target\n");
		return -ENXIO;
	}
	
	debugfs_create_file("gpio", S_IFREG | S_IRUGO,
				tgt_root, NULL, &tgt_gpio_operations);
	
	debugfs_create_file("usb", S_IFREG | S_IRUGO,
				tgt_root, NULL, &tgt_usb_operations);
	return 0;
}

