/*
 *  smd.c
 *  share mem device driver for RDA8850e
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <plat/devices.h>
#include <plat/ipc.h>

#define TAG "SMD: "

#define RDA_SMD_UL_BUF_MOFFSET	0
#define RDA_SMD_UL_BUF_DOFFSET	4096
#define RDA_SMD_UL_BUF_EOFFSET	(1024*1024)

#define RDA_SMD_DL_BUF_MOFFSET	(1024*1024)
#define RDA_SMD_DL_BUF_DOFFSET	(1024*1024 + 4096)
#define RDA_SMD_DL_BUF_EOFFSET	(1024*1024*2)

typedef struct ipdata_buf_info {
	volatile u32 head;		/* read pointer, moved by alloc */
	volatile u32 tail;		/* write pointer, moved by free */
} buf_info_t;

/**********************GLOBAL VARIABLES**********************/

unsigned char *rda_smd_base;
unsigned char *rda_smd_phys;

static buf_info_t *ul;
static buf_info_t *dl;

u32 *ul_free_buf;/* ul free buf array */
u32 *dl_free_buf;/* dl free buf array */

/*******************FUNCTION DEFINITIONS********************/

u32 smd_get_ul_free_buf_count(void)
{
	return RDA_UL_IPDATA_BUF_CNT - ((ul->head - ul->tail) & RDA_UL_MASK);
}

u32 smd_get_dl_free_buf_count(void)
{
	return RDA_DL_IPDATA_BUF_CNT - ((dl->head - dl->tail) & RDA_DL_MASK);
}

u32 smd_alloc_ul_ipdata_buf(void)
{
	ps_header_t *header;
	u32 offset;

	offset = ul_free_buf[ul->head];
	ul->head++;
	ul->head &= RDA_UL_MASK;

	header = (ps_header_t *)(rda_smd_base + offset);
	header->next_offset = 0;


	return offset;
}

void smd_free_dl_ipdata_buf(u32 offset)
{
	dl_free_buf[dl->tail] = offset;
	dl->tail++;
	dl->tail &= RDA_DL_MASK;
}

void smd_fill_ul_ipdata_hdr(ps_header_t *header, u8 simid, u8 cid, u16 len)
{
	header->simid = simid;
	header->cid = cid;
	header->len = len;
}

static void smd_init_ipdata_buf(u8 *sm_base)
{
	int i = 0;

	/* UL IP data buf init */
	ul = (buf_info_t *)(sm_base + RDA_SMD_UL_BUF_MOFFSET);
	ul->head = 0;
	ul->tail = 0;
	ul_free_buf = (u32 *)(sm_base + RDA_SMD_UL_BUF_MOFFSET + 8);
	for (i = 0; i < RDA_UL_IPDATA_BUF_CNT; i++) {
		ul_free_buf[i] = (RDA_IPDATA_BUF_SIZE * i) + RDA_SMD_UL_BUF_DOFFSET;
		//pr_info(TAG "%d, 0x%p, 0x%x\n", i, &ul_free_buf[i], ul_free_buf[i]);
	}

	/* DL IP data buf init */
	dl = (buf_info_t *)(sm_base + RDA_SMD_DL_BUF_MOFFSET);
	dl->head = 0;
	dl->tail = 0;
	dl_free_buf = (u32 *)(sm_base + RDA_SMD_DL_BUF_MOFFSET + 8);
	for (i = 0; i < RDA_DL_IPDATA_BUF_CNT; i++) {
		dl_free_buf[i] = (RDA_IPDATA_BUF_SIZE * i) + RDA_SMD_DL_BUF_DOFFSET;
		//pr_info(TAG "%d, 0x%p, 0x%x\n", i, &dl_free_buf[i], dl_free_buf[i]);
	}
}

static ssize_t ul_free_buf_cnt_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	u32 cnt = smd_get_ul_free_buf_count();

	return sprintf(buf, "ul free buf cnt %u\n", cnt);
}
static DEVICE_ATTR(ul_info, S_IRUGO, ul_free_buf_cnt_show, NULL);

static ssize_t dl_free_buf_cnt_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	u32 cnt = smd_get_dl_free_buf_count();

	return sprintf(buf, "dl free buf cnt %u\n", cnt);
}
static DEVICE_ATTR(dl_info, S_IRUGO, dl_free_buf_cnt_show, NULL);

static int rda_smd_probe(struct platform_device *pdev)
{
	struct resource *sm;
	resource_size_t size;

	pr_info(TAG "%s\n", __func__);

	sm = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!sm) {
		pr_err(TAG "get mem resource failed\n");
		return -EINVAL;
	}
	size = resource_size(sm);
	rda_smd_phys = (u8*)(sm->start);

	rda_smd_base = ioremap_nocache(sm->start, size);
	if (!rda_smd_base) {
		pr_err(TAG "ioremap share mem failed\n");
		return -ENOMEM;
	}

	smd_init_ipdata_buf(rda_smd_base);

	device_create_file(&pdev->dev, &dev_attr_ul_info);
	device_create_file(&pdev->dev, &dev_attr_dl_info);

	pr_info(TAG "%s finished\n", __func__);

	return 0;
}

static int rda_smd_suspend(struct platform_device *pdev,
								pm_message_t state)
{
	pr_info(TAG "%s\n", __func__);
	return 0;
}

static int rda_smd_resume(struct platform_device *pdev)
{
	pr_info(TAG "%s\n", __func__);
	return 0;
}

static struct platform_driver rda_smd_driver = {
	.driver = {
		.name = RDA_SMD_DRV_NAME,
		.owner = THIS_MODULE,
	},
	.suspend = rda_smd_suspend,
	.resume = rda_smd_resume,
	.probe = rda_smd_probe,
};

static int __init rda_smd_init(void)
{
	return platform_driver_register(&rda_smd_driver);
}
module_init(rda_smd_init);

static void __exit rda_smd_exit(void)
{
	platform_driver_unregister(&rda_smd_driver);
}
module_exit(rda_smd_exit);
