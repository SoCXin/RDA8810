/*
 *  ipc_rmnet
 *  Virture Ethernet Driver for RDA8850e
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <plat/ipc.h>
#include <plat/dma.h>
#include <plat/devices.h>
#include <plat/rda_md.h>

#define MAX_RMNET_IF    4
#define MAX_RX_COUNT	64
#define SIMCARD1	0
#define SIMCARD2	1
#define CID1		1
#define CID2		2
#define IPV4 		4

#define RMNET_UL_MIN_FREE	(2)
#define RMNET_DMA_SIZE		(64)
#define RMNET_DMA_TIMEOUT_MS    (100)

#define TAG	"RMNET: "

struct rmnet_private {
	struct net_device *dev;
	struct device *pdev;
	struct net_device_stats stats;
	struct timer_list tx_check_timer;

	u8 simid;
	u8 cid;
	int channel;
};

/**********************GLOBAL VARIABLES**********************/

static struct rmnet_private *g_rmnet_private[MAX_RMNET_IF] = {NULL,};
static struct md_port *ps_port;

static struct task_struct *rmnet_tx_task;
static wait_queue_head_t rmnet_tx_wait;
static struct sk_buff_head rmnet_tx_frames;
static struct work_struct rmnet_work;
static atomic_t rx_status;

static u8 rx_dma_ch;
static u8 tx_dma_ch;
static struct completion rx_dma_comp;
static struct completion tx_dma_comp;
static struct mutex rx_dma_mutex;
static struct mutex tx_dma_mutex;

extern u8 *rda_smd_base;
extern u8 *rda_smd_phys;

/*******************FUNCTION DEFINITIONS********************/

int smd_is_ready(void);
static void rmnet_rx_notify(void *arg, unsigned int event);

static void tx_check_timer_func(unsigned long arg)
{
	struct net_device *dev = (struct net_device *)arg;
	struct rmnet_private *p = netdev_priv(dev);

	if (smd_get_ul_free_buf_count() > (RMNET_UL_MIN_FREE + 448 + \
			skb_queue_len(&rmnet_tx_frames))) {
		netif_start_queue(dev);
		pr_info(TAG "%s writeable..\n", dev->name);
	} else {
		mod_timer(&p->tx_check_timer, jiffies + HZ/10);
		pr_info(TAG "wait %s writeable..\n", dev->name);
	}
}

static int rmnet_open(struct net_device *dev)
{
	pr_info(TAG "open %s\n", dev->name);
	netif_start_queue(dev);

	return 0;
}

static int rmnet_stop(struct net_device *dev)
{
	pr_info(TAG "close %s\n", dev->name);
	netif_stop_queue(dev);

	return 0;
}

static int rmnet_tx_dma(struct rmnet_private *p,
			struct sk_buff *skb, u32 offset)
{
	struct rda_dma_chan_params dma_param;
	dma_addr_t skb_phys_addr;
	int ret;

	mutex_lock(&tx_dma_mutex);

	skb_phys_addr = dma_map_single(p->pdev, skb->data,
				skb->len, DMA_TO_DEVICE);
	dma_param.src_addr = skb_phys_addr;
	dma_param.dst_addr = (dma_addr_t)(rda_smd_phys + \
				offset + RESERVED_LEN);
	dma_param.xfer_size = skb->len;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;
	dma_param.enable_int = 1;

	ret = rda_set_dma_params(tx_dma_ch, &dma_param); // SET
	if (ret < 0) {
		pr_err(TAG "failed to set parameter\n");
		dma_unmap_single(p->pdev, skb_phys_addr, skb->len,
					DMA_TO_DEVICE);

		mutex_unlock(&tx_dma_mutex);
		return ret;
	}

	rda_start_dma(tx_dma_ch); // START

	ret = wait_for_completion_timeout(&tx_dma_comp,
				msecs_to_jiffies(RMNET_DMA_TIMEOUT_MS));
	if (ret <= 0) {
		dma_unmap_single(p->pdev, skb_phys_addr, skb->len,
					DMA_TO_DEVICE);

		mutex_unlock(&tx_dma_mutex);
		return ret;
	}

	dma_unmap_single(p->pdev, skb_phys_addr, skb->len,
				DMA_TO_DEVICE);

	mutex_unlock(&tx_dma_mutex);

	return 0;
}

static int rmnet_tx_one_skb(struct sk_buff *skb)
{
	struct rmnet_private *p;
	ps_header_t *header;
	u32 offset;
	int ret = 0;

	p = netdev_priv(skb->dev);
	if (!p) {
		pr_err(TAG "%s (private == NULL)\n", skb->dev->name);
		return -1;
	}

	if (skb->len <= RMNET_DMA_SIZE) {
		offset = smd_alloc_ul_ipdata_buf();

		memcpy(rda_smd_base + offset + RESERVED_LEN,
					skb->data, skb->len);

		header = (ps_header_t *)(rda_smd_base + offset);
		smd_fill_ul_ipdata_hdr(header, p->simid, p->cid, skb->len);
		ps_ul_write();
	} else {
		offset = smd_alloc_ul_ipdata_buf();

		ret = rmnet_tx_dma(p, skb, offset);
		if (ret == 0) {
			header = (ps_header_t *)(rda_smd_base + offset);
			smd_fill_ul_ipdata_hdr(header, p->simid, p->cid, skb->len);
			ps_ul_write();
		} else {
			memcpy(rda_smd_base + offset + RESERVED_LEN,
						skb->data, skb->len);
			header = (ps_header_t *)(rda_smd_base + offset);
			smd_fill_ul_ipdata_hdr(header, p->simid, p->cid, skb->len);
			ps_ul_write();
		}
	}
	p->stats.tx_packets++;
	p->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);

	return ret;
}

static int rmnet_tx_thread(void *arg)
{
	struct sched_param param = { .sched_priority = 1 };
	struct sk_buff *skb = NULL;

	sched_setscheduler(current, SCHED_FIFO, &param);

	while (!kthread_should_stop()) {
		if (skb_queue_empty(&rmnet_tx_frames)) {
				wait_event_interruptible(rmnet_tx_wait,
					!skb_queue_empty(&rmnet_tx_frames));
		}

		while (!skb_queue_empty(&rmnet_tx_frames)) {
			if (ps_ul_idle()) {
				skb = skb_dequeue(&rmnet_tx_frames);
				rmnet_tx_one_skb(skb);
			} else {
				/* wait modem UL idle */
				pr_info(TAG "Modem UL Busy.\n");
				msleep(2);
			}
		}
	}

	return 0;
}

static int rmnet_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct rmnet_private *p = NULL;

	p = netdev_priv(dev);
	if (!p) {
		pr_err(TAG "%s (private == NULL)\n", dev->name);
		return NETDEV_TX_BUSY;
	}

	if (smd_get_ul_free_buf_count() <= (RMNET_UL_MIN_FREE + 448 + \
			skb_queue_len(&rmnet_tx_frames))) {
		netif_stop_queue(dev);
		mod_timer(&p->tx_check_timer, jiffies + HZ/100);
		pr_info(TAG "wait %s writeable..\n", dev->name);
		return NETDEV_TX_BUSY;
	}

	skb_queue_tail(&rmnet_tx_frames, skb);
	wake_up_interruptible(&rmnet_tx_wait);

	return NETDEV_TX_OK;
}

static struct net_device_stats *rmnet_get_stats(struct net_device *dev)
{
	struct rmnet_private *p;

	p = netdev_priv(dev);
	if (p) {
		return &p->stats;
	} else {
		return NULL;
	}
}

static void rmnet_tx_timeout(struct net_device *dev)
{
	pr_info(TAG "%s\n", __func__);
}

static struct net_device_ops rmnet_ops = {
	.ndo_open       = rmnet_open,
	.ndo_stop       = rmnet_stop,
	.ndo_start_xmit = rmnet_xmit,
	.ndo_get_stats  = rmnet_get_stats,
	.ndo_tx_timeout = rmnet_tx_timeout,
};

static int rmnet_rx_dma(struct rmnet_private *p,
		struct sk_buff *skb, u32 offset, u32 ipdata_len)
{
	struct rda_dma_chan_params dma_param;
	dma_addr_t skb_phys_addr;
	int ret;

	mutex_lock(&rx_dma_mutex);

	skb_phys_addr = dma_map_single(p->pdev, skb->data,
				ipdata_len, DMA_FROM_DEVICE);
	dma_param.src_addr = (dma_addr_t)(rda_smd_phys + \
				offset + RESERVED_LEN);
	dma_param.dst_addr = skb_phys_addr;
	dma_param.xfer_size = ipdata_len;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;
	dma_param.enable_int = 1;

	ret = rda_set_dma_params(rx_dma_ch, &dma_param); // SET
	if (ret < 0) {
		pr_err(TAG "failed to set parameter\n");
		dma_unmap_single(p->pdev, skb_phys_addr, ipdata_len,
					DMA_FROM_DEVICE);
		p->stats.rx_dropped++;

		mutex_unlock(&rx_dma_mutex);
		return ret;
	}

	rda_start_dma(rx_dma_ch); // START

	ret = wait_for_completion_timeout(&rx_dma_comp,
				msecs_to_jiffies(RMNET_DMA_TIMEOUT_MS));
	if (ret <= 0) {
		pr_err(TAG "dma timeout, ret = 0x%08x\n", ret);
		dma_unmap_single(p->pdev, skb_phys_addr, ipdata_len,
					DMA_FROM_DEVICE);
		p->stats.rx_dropped++;

		mutex_unlock(&rx_dma_mutex);
		return ret;
	}

	dma_unmap_single(p->pdev, skb_phys_addr, ipdata_len,
				DMA_FROM_DEVICE);

	mutex_unlock(&rx_dma_mutex);

	return 0;
}

static void rmnet_rx_one_skb(struct net_device *dev, u32 offset)
{
	struct rmnet_private *p = NULL;
	struct iphdr *iph;
	struct sk_buff *skb;
	u32 ipdata_len;
	int err = 0;

	p = netdev_priv(dev);
	if (!p) {
		pr_err(TAG "Error rmnet_private is NULL\n");
		BUG();
	}

	iph = (struct iphdr *)(rda_smd_base + offset + RESERVED_LEN);
	if (iph->version == IPV4) {
		ipdata_len = ntohs(iph->tot_len);
	} else {
		p->stats.rx_dropped++;
		return;
	}

	skb = netdev_alloc_skb(dev, ipdata_len);
	if (!skb) {
		pr_err(TAG "%s: alloc skbuf failed.\n", dev->name);
		p->stats.rx_dropped++;
		return;
	}
	skb_put(skb, ipdata_len);

	if (ipdata_len <= RMNET_DMA_SIZE) {
		memcpy(skb->data, (void *)iph, ipdata_len);
	} else {
		err = rmnet_rx_dma(p, skb, offset, ipdata_len);
		if (err < 0) {
			memcpy(skb->data, (void *)iph, ipdata_len);
		}
	}

	skb->pkt_type = 0;
	skb->protocol = htons(ETH_P_IP);
	p->stats.rx_packets++;
	p->stats.rx_bytes += skb->len;
	netif_rx(skb);
}

static void rmnet_rx_work(struct work_struct *work)
{
	struct net_device *dev = NULL;
	ps_header_t *header;
	u32 avail, offset;
	u32 count = 0;

	if (0 == atomic_read(&rx_status)) {
		atomic_set(&rx_status, 1); // rx is running now
	} else {
		return;
	}

	do {
		avail = ps_dl_read_avail();
		if (avail > 0) {
			offset = ps_dl_read();

			header = (ps_header_t *)(rda_smd_base + offset);
			if (header->simid == SIMCARD1) {
				if (header->cid == CID1) {
					dev = g_rmnet_private[0]->dev; //rmnet0
				} else if (header->cid == CID2) {
					dev = g_rmnet_private[1]->dev; //rmnet1
				}
			} else if (header->simid == SIMCARD2) {
				if (header->cid == CID1) {
					dev = g_rmnet_private[2]->dev; //rmnet2
				} else if (header->cid == CID2) {
					dev = g_rmnet_private[3]->dev; //rmnet3
				}
			}

			rmnet_rx_one_skb(dev, offset);
			smd_free_dl_ipdata_buf(offset);
			count++;
		} else {
			break;
		}
	} while (count < MAX_RX_COUNT);

	atomic_set(&rx_status, 0); // rx is exit

	if (ps_dl_read_avail() > 0) {
		schedule_work(&rmnet_work);
	} else {
		enable_rmnet_irq();
	}

	return;
}

static void rmnet_rx_notify(void *arg, unsigned int event)
{
	if (event != MD_EVENT_DATA)
		return;

	disable_rmnet_irq();
	schedule_work(&rmnet_work);
}

static void rmnet_setup(struct net_device *dev)
{
	pr_info(TAG "%s\n", __func__);

	dev->netdev_ops     = &rmnet_ops;
	dev->watchdog_timeo = 20;
	dev->mtu            = ETH_DATA_LEN;
	dev->flags          = IFF_NOARP | IFF_POINTOPOINT | IFF_MULTICAST;
}

static void rmnet_rx_dma_cb(u8 ch, void *data)
{
	complete((struct completion *)data);
}

static void rmnet_tx_dma_cb(u8 ch, void *data)
{
	complete((struct completion *)data);
}

static int rmnet_init(struct platform_device *pdev)
{
	struct net_device *dev  = NULL;
	struct rmnet_private *p = NULL;
	int ret, i, j;

	skb_queue_head_init(&rmnet_tx_frames);
	init_waitqueue_head(&rmnet_tx_wait);
	init_completion(&tx_dma_comp);
	init_completion(&rx_dma_comp);
	INIT_WORK(&rmnet_work, rmnet_rx_work);
	atomic_set(&rx_status, 0);
	mutex_init(&rx_dma_mutex);
	mutex_init(&tx_dma_mutex);

	ret = rda_request_dma(0, "rmnet-rx-dma", rmnet_rx_dma_cb, &rx_dma_comp,
			&rx_dma_ch);
	if (ret < 0) {
		pr_info(TAG "request rmnet-rx-dma failed.\n");
		goto rx_dma_request_failed;
	}

	ret = rda_request_dma(0, "rmnet-tx-dma", rmnet_tx_dma_cb, &tx_dma_comp,
			&tx_dma_ch);
	if (ret < 0) {
		pr_info(TAG "request rmnet-tx-dma failed.\n");
		goto tx_dma_request_failed;
	}

	rmnet_tx_task = kthread_run(rmnet_tx_thread, NULL, "rmnet-tx");
	if (IS_ERR(rmnet_tx_task)) {
		goto rmnet_tx_task_failed;
	}

	for (i = 0; i < MAX_RMNET_IF; i++) {
		dev = alloc_netdev(sizeof(struct rmnet_private),
					"rmnet%d", rmnet_setup);
		if (!dev) {
			pr_err(TAG "alloc_netdev failed.\n");
			goto alloc_failed;
		}

		p = netdev_priv(dev);
		p->dev = dev;
		p->pdev = &pdev->dev;
		p->channel = i;
		g_rmnet_private[i] = p;

		switch (p->channel) {
		case 0:
			p->simid = SIMCARD1;
			p->cid = CID1;
			break;
		case 1:
			p->simid = SIMCARD1;
			p->cid = CID2;
			break;
		case 2:
			p->simid = SIMCARD2;
			p->cid = CID1;
			break;
		case 3:
			p->simid = SIMCARD2;
			p->cid = CID2;
			break;
		}
		setup_timer(&p->tx_check_timer,
			     tx_check_timer_func, (unsigned long)dev);

		ret = register_netdev(dev);
		if (ret < 0) {
			pr_err(TAG "register_netdev failed\n");
			free_netdev(dev);
			p   = NULL;
			dev = NULL;
			goto register_failed;
		}
		p   = NULL;
		dev = NULL;
	}

	return 0;

register_failed:
alloc_failed:
	for (j = 0; j < i; j++) {
		unregister_netdev(g_rmnet_private[j]->dev);
		free_netdev(g_rmnet_private[j]->dev);
		g_rmnet_private[j] = NULL;
	}

	kthread_stop(rmnet_tx_task);

rmnet_tx_task_failed:
	rda_free_dma(tx_dma_ch);

tx_dma_request_failed:
	rda_free_dma(rx_dma_ch);

rx_dma_request_failed:
	return -1;
}

static int rda_rmnet_probe(struct platform_device *pdev)
{
	int ret = 0;

	if (!smd_is_ready()) {
		pr_err(TAG "SMD device not exist, does not supprot rmnet\n");
		return -ENOENT;
	}

	ret = rda_md_open(MD_PORT_PS, &ps_port, NULL, rmnet_rx_notify);
	if (ret < 0) {
		pr_err(TAG "Open md PS port failed, %d\n", ret);
		return ret;
	}

	ret = rmnet_init(pdev);
	if (ret < 0) {
		pr_err(TAG "rmnet init failed, %d\n", ret);
		rda_md_close(ps_port);
		return ret;
	}

	return 0;
}

static int rda_rmnet_suspend(struct platform_device *pdev,
								pm_message_t state)
{
	pr_info(TAG "%s\n", __func__);

	mutex_lock(&rx_dma_mutex);
	mutex_lock(&tx_dma_mutex);

	return 0;
}

static int rda_rmnet_resume(struct platform_device *pdev)
{
	pr_info(TAG "%s\n", __func__);

	mutex_unlock(&rx_dma_mutex);
	mutex_unlock(&tx_dma_mutex);

	return 0;
}

static struct platform_driver rda_rmnet_driver = {
	.driver = {
		.name = RDA_RMNET_DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = rda_rmnet_probe,
	.suspend = rda_rmnet_suspend,
	.resume = rda_rmnet_resume,
};

static int __init rda_rmnet_init(void)
{
	return platform_driver_register(&rda_rmnet_driver);
}
module_init(rda_rmnet_init);

static void __exit rda_rmnet_exit(void)
{
	platform_driver_unregister(&rda_rmnet_driver);
}
module_exit(rda_rmnet_exit);
