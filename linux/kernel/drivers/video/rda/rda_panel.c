#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <mach/hardware.h>
#include <plat/devices.h>

#include "rda_panel.h"
#include "rda_gouda.h"

static int found_lcd_panel = 0;
static int test_lcd_num = 0;
/*
struct rda_panel_id_info rda_id_table[] = {
	{
		.panel_name = ILI9806C_PANEL_NAME,
		.id = {0x0,0x98,0x06}
	},
	{
		.panel_name = ILI9806H_MCU_PANEL_NAME,
		.id = {0x0,0x98,0x26}
	},
};
*/

unsigned int read_rxtxbuffer(u16 * receive_buffer,u8 framesize,u8 total_bits,u8 byte_num)
{
	u16 tmp;
	u32 readdata = 0;
	int hi,lo;

	tmp = receive_buffer[0];
	readdata = (tmp << (total_bits -16));
	tmp = receive_buffer[1];
	tmp &= (0xffff >> (32 - total_bits));
	readdata |= tmp;

	/*get send cmd*/
	hi = total_bits - 1;
	lo = hi - framesize + 1;
	tmp = PANEL_GET_BITFIELD(readdata,lo,hi);

	switch(byte_num){
		case 0:
			/*get first byte*/
			hi = lo - 1;
			lo = hi - 7;
			lo = lo < 0 ? 0 : lo;
			if(hi >= 0){
				tmp = PANEL_GET_BITFIELD(readdata,lo,hi);
				printk("first byte 0x%02x\n",tmp);
			}else{
				printk("bits beyond receive length\n");
				return 0;
			}
			break;
		case 1:
			/*get second byte*/
			hi = lo - 9;
			lo = hi - 7;
			lo = lo < 0 ? 0 : lo;
			if(hi >= 0){
				tmp = PANEL_GET_BITFIELD(readdata,lo,hi);
				printk("sec byte 0x%02x\n",tmp);
			}else{
				printk("bits beyond receive length\n");
				return 0;
			}
			break;
		case 2:
			/*get third byte*/
			hi = lo - 17;
			lo = hi - 7;
			lo = lo < 0 ? 0 : lo;
			if(hi >= 0){
				tmp = PANEL_GET_BITFIELD(readdata,lo,hi);
				printk("third byte 0x%02x\n",tmp);
			}else{
				printk("bits beyond receive length\n");
				return 0;
			}
			break;
	}
	return tmp;
}

void panel_spi_transfer(struct rda_spi_panel_device *panel_dev,
			       const u8 * wbuf, u8 * rbuf, int len)
{
	struct spi_message m;
	struct spi_transfer *x;
	int r;

	BUG_ON(panel_dev->spi == NULL);

	spi_message_init(&m);
	x = &panel_dev->spi_xfer[panel_dev->spi_xfer_num];
	x->bits_per_word = panel_dev->spi->bits_per_word;
	x->len = len;
	panel_dev->spi_xfer_num++;
	if (panel_dev->spi_xfer_num >= 4)
		panel_dev->spi_xfer_num = 0;

	if (wbuf) {
		x->tx_buf = wbuf;
	}

	if (rbuf) {
		x->rx_buf = rbuf;
	}

	spi_message_add_tail(x, &m);

	r = spi_sync(panel_dev->spi, &m);
	if (r < 0)
		dev_err(&panel_dev->spi->dev, "spi_sync err %d\n", r);

	return;
}

void panel_spi_write(struct rda_spi_panel_device *panel_dev,
				   const u8 * buf, int len)
{
	panel_spi_transfer(panel_dev, buf, NULL, len);
}

void panel_spi_read(struct rda_spi_panel_device *panel_dev,
				  u8 * buf, int len)
{
	panel_spi_transfer(panel_dev, NULL, buf, len);
}

void panel_spi_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer)
{
	struct spi_message m;
	struct spi_transfer *x;
	u8 total_bits,read_total_bits,framesize,i,r;
	u16 receive_buffer[2] = {0};
	u16 AddressAndData,wlen;

	/*for 3 wire 9bits spi*/
	if(id_param->lcd_spicfg->frameSize == 9){
		AddressAndData = cmd << 8;
		AddressAndData = AddressAndData >> 1;
	}

	read_total_bits = id_param->per_read_bytes << 3;
	if(read_total_bits > MAX_SPI_READ_BITS)
		read_total_bits = MAX_SPI_READ_BITS;

	id_param->lcd_spicfg->spi_read_bits = read_total_bits;

	framesize = id_param->lcd_spicfg->frameSize;
	total_bits = framesize + read_total_bits;

	wlen = (total_bits + framesize -1)/framesize;

	spi_message_init(&m);
	x = &id_param->panel_dev->spi_xfer[0];
	x->bits_per_word = id_param->panel_dev->spi->bits_per_word;
	x->len = wlen;
	x->tx_buf = (u8 *) (&AddressAndData);
	x->rx_buf = (u8 *) (&receive_buffer);

	spi_message_add_tail(x, &m);

	r = spi_sync(id_param->panel_dev->spi, &m);
	if (r < 0)
		dev_err(&id_param->panel_dev->spi->dev, "spi_sync err %d\n", r);

	for(i = 0;i < id_param->per_read_bytes;i++){
		id_buffer[i] = read_rxtxbuffer(receive_buffer,framesize + id_param->dumy_bits,total_bits,i);
	}

	id_param->lcd_spicfg->spi_read_bits = 0;
	return ;
}

void panel_mcu_read_id(u32 cmd,struct rda_panel_id_param *id_param,u16 *id_buffer)
{
	u8 i;

	LCD_MCU_CMD(cmd);
	for(i = 0;i < id_param->per_read_bytes;i++){
		LCD_MCU_READ((u8 *)&id_buffer[i]);
	}
	return ;
}

static inline struct spi_device *to_spi_dev(struct device *dev)
{
	return container_of(dev, struct spi_device, dev);
}

struct spi_device *panel_find_spidev_by_name(const char * panel_spi_name)
{
	struct device *found;
	struct spi_device * spi_panel_device;
	char  spi_name[8];

	strcpy(spi_name,panel_spi_name);

	found = bus_find_device_by_name(&spi_bus_type, NULL, spi_name);
	if (!found){
		printk("##lcd## not found spi dev\n");
		return NULL;
	}
	spi_panel_device = to_spi_dev(found);

	return spi_panel_device;
}

static int parse_lcd_str(const char * wr_str,unsigned int *value)
{
	const char * wrstr = wr_str;
	const char *presrc = wr_str;
	char str_to[64] = {0};
	int found = 0;
	int val = 0,count = 0,ret = 0;

	if(isdigit(*wrstr)){
		found = 1;
	}

	while(*wrstr != '\0'){
		wrstr++;
		if(isxdigit(*wrstr) && !found){
			presrc = wrstr;
			found = 1;
		}else{
			if(!isxdigit(*wrstr) && found && _tolower(*wrstr) != 'x'){
				memcpy(str_to,presrc,wrstr - presrc);

				ret = kstrtoint(str_to, 0, &val);
				if (ret < 0) {
					return ret;
				}
				value[count++] = val;
				found = 0;
			}
		}
	}

	return count;
}

static int rda_lcd_debug_parse_str(const char *buf, size_t count)
{
	struct platform_device *rda_fb_device;
	struct rda_lcd_info *lcd_info;
	int nbuf[32] = {0};
	int wr_num;

	rda_fb_device = (struct platform_device*)rda_fb_get_device();
	lcd_info = (struct rda_lcd_info *)rda_fb_device->dev.platform_data;

	if(!isdigit(*buf)){
		if(!strncmp("reset",buf,5)){
			lcd_info->ops.s_init_gpio();
		}else if(!strncmp("init",buf,4)){
			lcd_info->ops.s_open();
		}/*else if(!strncmp("open",buf,4)){
		}*/
		return 0;
	}

	wr_num = parse_lcd_str(buf,nbuf);
	rda_gouda_stretch_pre_wait_and_enable_clk();
	lcd_info->ops.s_rda_lcd_dbg_w(nbuf,wr_num);
	rda_gouda_stretch_post_disable_clk();

	return 0;
}

static ssize_t rda_panel_wrlcd_write_file(struct file *file,
				      const char __user *user_buf, size_t count,
				      loff_t *ppos)
{
	char buf[256];
	ssize_t buf_size;

	buf_size = min(count, (sizeof(buf)));
	if (copy_from_user(buf, user_buf, buf_size)) {
		printk("Failed to copy from user\n");
		return -EFAULT;
	}

	rda_lcd_debug_parse_str(buf,buf_size);

	return buf_size;
}

static const struct file_operations rda_panel_debugw_fops = {
	.open = simple_open,
	.write = rda_panel_wrlcd_write_file,
};

static int rda_lcd_dbg_init(void)
{
	struct dentry *droot;
	struct dentry *debugw;

	droot = debugfs_create_dir("rda_panel_debug", NULL);
	if (!droot) {
		printk("Failed to create rda_panel_debug droot dir\n");
		return -ENOMEM;
	}

	debugw = debugfs_create_file("wrlcd", 0755, droot, NULL,
		&rda_panel_debugw_fops);

	if (!debugw) {
		printk("Failed to create rda_panel_debug debug file\n");
		return -ENOMEM;
	}

	return 0;

}

int rda_fb_probe_panel(struct rda_lcd_info *p_info, void *p_driver)
{
	int len,ret = -1;
	char *pstr = NULL;

	if(found_lcd_panel)
		return ret;

	if (AUTO_DETECT_SUPPORTED_PANEL_NUM > 0) {
		if (!strcmp(AUTO_DETECT_SUPPORTED_PANEL_LIST, DEFAULT_PANEL_LIST)) {
			printk(KERN_ERR"target panel setting fatal error\n");
			return ret;
		}
			
		pstr = strstr(AUTO_DETECT_SUPPORTED_PANEL_LIST, p_info->name);
		if (pstr)
			test_lcd_num++;
		else
			return ret;
	}

	len = strlen(p_info->name);
	pstr = strstr(saved_command_line, p_info->name);

	if(pstr && pstr[len] == ' ' ) {
		printk(KERN_INFO"%s normal probe panel\n",__func__);
		found_lcd_panel = 1;
		goto found;
	}

	len = strlen(AUTO_DET_LCD_PANEL_NAME);
	pstr = strstr(saved_command_line, AUTO_DET_LCD_PANEL_NAME);
	if(pstr && pstr[len] == ' ' ) {
		printk(KERN_INFO"%s auto detect lcd panel by ID\n",__func__);
		if (AUTO_DETECT_SUPPORTED_PANEL_NUM > 0)
			printk(KERN_INFO"supported list num %d test num %d\n", AUTO_DETECT_SUPPORTED_PANEL_NUM, test_lcd_num);
		if((p_info->ops.s_readid && p_info->ops.s_readid()) || (test_lcd_num == AUTO_DETECT_SUPPORTED_PANEL_NUM)){
			found_lcd_panel = 1;
			goto found;
		}
	}
	printk(KERN_INFO"%s lcd panel'name(%s) doesn't match of cmdline!\n"
		,__func__,p_info->name);
	return ret;

found:
	printk("lcd %s is selected\n", p_info->name);
	if (p_info->lcd.lcd_interface == GOUDA_LCD_IF_DBI) {
		struct platform_driver *plat_drv = (struct platform_driver *)p_driver;
		plat_drv->driver.name = RDA_DBI_PANEL_DRV_NAME;
	}
	else if (p_info->lcd.lcd_interface == GOUDA_LCD_IF_DPI) {
		struct spi_driver *spi = (struct spi_driver *)p_driver;
		spi->driver.name = RDA_DPI_PANEL_DRV_NAME;
	} else {
		printk("we do not support DSI currently\n");
	}

	ret = rda_lcd_dbg_init();

	return 0;
}

/*LCD panel driver init*/
#ifdef TARGET_AP_PANEL_ILI9806H_MCU
#include "rda_panel_ili9806h_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_ILI9806G_MCU
#include "rda_panel_ili9806g_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_RM68180_MCU
#include "rda_panel_rm68180_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_R61581B_MCU
#include "rda_panel_r61581b_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_ILI9488_MCU
#include "rda_panel_ili9488_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_RM68140_MCU
#include "rda_panel_rm68140_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_NT35310_MCU
#include "rda_panel_nt35310_mcu.c"
#endif
#ifdef TARGET_AP_PANEL_OTM8019A_RGB
#include "rda_panel_otm8019a.c"
#endif
#ifdef TARGET_AP_PANEL_JD9161BA_RGB
#include "rda_panel_jd9161ba.c"
#endif
#ifdef TARGET_AP_PANEL_HX8357_MCU
#include "rda_panel_hx8357_mcu.c"
#endif


static struct rda_panel_driver *panel_driver[] = {
#ifdef TARGET_AP_PANEL_ILI9806H_MCU
	&ili9806h_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_ILI9806G_MCU
	&ili9806g_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_RM68180_MCU
	&rm68180_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_R61581B_MCU
	&r6158b_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_RM68140_MCU
	&rm68140_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_NT35310_MCU
	&nt35310_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_ILI9488_MCU
	&ili9488_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_OTM8019A_RGB
	&otm8019a_mcu_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_JD9161BA_RGB
	&jd9161ba_rgb_panel_driver,
#endif
#ifdef TARGET_AP_PANEL_HX8357_MCU
	&hx8357_mcu_panel_driver
#endif
};

static int found_driver_index;
static int __init rda_fb_lcd_panel_init(void)
{
	int i;
	int status;
	struct rda_panel_driver *pdrvier;

	for(i = 0;i < ARRAY_SIZE(panel_driver);i++){
		pdrvier = panel_driver[i];
		if(pdrvier->panel_type != GOUDA_LCD_IF_DPI){
			status = rda_fb_probe_panel(pdrvier->lcd_driver_info,
				pdrvier->pltaform_panel_driver);
			if(!status){/*read id or found lcd*/
				found_driver_index = i;
				return platform_driver_register(pdrvier->pltaform_panel_driver);
			}
		}

		if(pdrvier->panel_type == GOUDA_LCD_IF_DPI){
			status = rda_fb_probe_panel(pdrvier->lcd_driver_info,
				pdrvier->rgb_panel_driver);
			if(!status){/*read id or found lcd*/
				found_driver_index = i;
				return spi_register_driver(pdrvier->rgb_panel_driver);
			}
		}
	}

	pr_err("ERROR no lcd panel driver init\n");
	return -EINVAL;
}

static void __exit rda_fb_lcd_panel_exit(void)
{
	struct rda_panel_driver *pdrvier;

	pdrvier = panel_driver[found_driver_index];
	if(pdrvier->panel_type != GOUDA_LCD_IF_DBI){
		platform_driver_unregister(pdrvier->pltaform_panel_driver);
	}

	if(pdrvier->panel_type == GOUDA_LCD_IF_DPI){
		spi_unregister_driver(pdrvier->rgb_panel_driver);
	}
}
module_init(rda_fb_lcd_panel_init);
module_exit(rda_fb_lcd_panel_exit);
