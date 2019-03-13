#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/parport.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#include <mach/hardware.h>
#include <mach/rda_clk_name.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <plat/reg_i2c.h>

#define HAL_I2C_OPERATE_TIME         (200)

#define HAL_ERR_RESOURCE_TIMEOUT     (1)
#define HAL_ERR_RESOURCE_BUSY        (2)
#define HAL_ERR_COMMUNICATION_FAILED (3)
#define HAL_ERR_INVALID              (4)
#define HAL_ERR_NO                   (0)

struct rda_i2c_dev {
	struct device		*dev;
	void __iomem		*base;		/* virtual */
	int			irq;
	struct clk		*master_clk;
	u32			speed_khz;	/* Speed of bus in Khz */
	u32			idle;
	int			master_id;
	struct i2c_adapter	adapter;
	struct regulator * i2c_regulator;
};

static void hal_i2c_set_clock(struct rda_i2c_dev *dev)
{
	unsigned long mclk = clk_get_rate(dev->master_clk);
	unsigned long clock, clk_div;
	unsigned long ctrlReg;
	HWP_I2C_MASTER_T* i2cMasterReg = (HWP_I2C_MASTER_T*)dev->base;

	clock = dev->speed_khz * 1000;
	clk_div = mclk / (5 * clock);
	if (mclk % (5 * clock))
		clk_div ++;
	if (clk_div >= 1) {
		clk_div -= 1;
	}
	if (clk_div > 0xffff) {
		clk_div = 0xffff;
	}

	ctrlReg = i2cMasterReg->CTRL & ~(I2C_MASTER_CLOCK_PRESCALE_MASK);
	ctrlReg |= I2C_MASTER_CLOCK_PRESCALE(clk_div);

	i2cMasterReg->CTRL = 0;
	i2cMasterReg->CTRL = ctrlReg;

	dev_info(dev->dev,
		"set clk = %d KHz, bus_clk = %d, divider = %d\n",
		(int)dev->speed_khz, (int)mclk, (int)clk_div);
}

static int hal_i2c_stop(struct rda_i2c_dev *dev)
{
	unsigned long timeout;
	HWP_I2C_MASTER_T* i2cMasterReg = (HWP_I2C_MASTER_T*)dev->base;

	i2cMasterReg->CMD	= I2C_MASTER_STO;

	timeout = jiffies + msecs_to_jiffies(HAL_I2C_OPERATE_TIME);
	while(!(i2cMasterReg->STATUS & I2C_MASTER_IRQ_STATUS)) {
		if (time_after(jiffies, timeout)) {
			dev_err(dev->dev, "hal_i2c_stop, timeout\n");
			return HAL_ERR_RESOURCE_TIMEOUT;
		}
	}
	i2cMasterReg->IRQ_CLR = I2C_MASTER_IRQ_CLR;
	//msleep(1); // Delay 1ms
	return HAL_ERR_NO;
}

static int hal_i2c_raw_send_byte(struct rda_i2c_dev *dev,u8 data, int start, int stop)
{
	u32 cmd;
	HWP_I2C_MASTER_T* i2cMasterReg = (HWP_I2C_MASTER_T*)dev->base;
	unsigned long timeout;
	//unsigned long timerout;
	//u32 msessc;
	//timerout = jiffies;
	cmd = I2C_MASTER_WR;
	if (start)
		cmd |= I2C_MASTER_STA;
	if (stop)
		cmd |= I2C_MASTER_STO;
	i2cMasterReg->TXRX_BUFFER = data;
	i2cMasterReg->CMD = cmd;

	timeout = jiffies + msecs_to_jiffies(HAL_I2C_OPERATE_TIME);
	while(!(i2cMasterReg->STATUS & I2C_MASTER_IRQ_STATUS))	{
		if (time_after(jiffies, timeout)) {
			i2cMasterReg->CMD = I2C_MASTER_STO;
			dev_err(dev->dev, "hal_i2c_raw_send_byte, timeout\n");
			return HAL_ERR_RESOURCE_TIMEOUT;
		}
	}

	i2cMasterReg->IRQ_CLR = I2C_MASTER_IRQ_CLR;
#if 0
	// Check RxACK
	if (i2cMasterReg->STATUS & I2C_MASTER_RXACK) {
		hal_i2c_stop(dev);
		dev_err(dev->dev, "hal_i2c_raw_send_byte, no ACK\n");
		return HAL_ERR_COMMUNICATION_FAILED;
	}
#endif
	timeout = jiffies + msecs_to_jiffies(HAL_I2C_OPERATE_TIME);
	while((i2cMasterReg->STATUS & I2C_MASTER_RXACK))	{
		if (time_after(jiffies, timeout)) {
			hal_i2c_stop(dev);
			//dev_err(dev->dev, "hal_i2c_raw_send_byte, timeout2\n");
			return HAL_ERR_COMMUNICATION_FAILED;
		}
	}
	//msessc = jiffies_to_msecs(jiffies-timerout);
	//dev_err(dev->dev, "hal_i2c_raw_send_byte = %d \n",msessc);
	return HAL_ERR_NO;
}

static int hal_i2c_raw_get_byte(struct rda_i2c_dev *dev,u8 *data, int start, int stop)
{
	u32 cmd;
	HWP_I2C_MASTER_T* i2cMasterReg = (HWP_I2C_MASTER_T*)dev->base;
	unsigned long timeout;

	cmd = I2C_MASTER_RD;
	if (start)
		cmd |= I2C_MASTER_STA;
	if (stop)
		cmd |= I2C_MASTER_ACK | I2C_MASTER_STO;
	i2cMasterReg->CMD = cmd;

	timeout = jiffies + msecs_to_jiffies(HAL_I2C_OPERATE_TIME);
	while(!(i2cMasterReg->STATUS & I2C_MASTER_IRQ_STATUS)) {
		if (time_after(jiffies, timeout)) {
			i2cMasterReg->CMD = I2C_MASTER_STO;
			//dev_err(dev->dev, "hal_i2c_raw_get_byte, timeout\n");
			return HAL_ERR_RESOURCE_TIMEOUT;
		}
	}

	i2cMasterReg->IRQ_CLR = I2C_MASTER_IRQ_CLR;

	// Store read value
	*data = (u8)(i2cMasterReg->TXRX_BUFFER & 0xff);

	return HAL_ERR_NO;
}

static void hal_i2c_enable(struct rda_i2c_dev *dev)
{
	HWP_I2C_MASTER_T* i2cMasterReg = (HWP_I2C_MASTER_T*)dev->base;
	i2cMasterReg->CTRL |= I2C_MASTER_EN;
}

int rda_i2c_init(struct rda_i2c_dev *dev)
{
	// Set initial clock divider
	hal_i2c_set_clock(dev);

	// Enable the I2c
	hal_i2c_enable(dev);
	return 0;
}

void rda_i2c_idle( struct rda_i2c_dev *dev)
{
	hal_i2c_stop(dev);
}

#if 0
int rda_i2c_send_reg(struct rda_i2c_dev *dev,u8 addr, u8 reg, u8 data)
{
	int ret;

	ret = hal_i2c_raw_send_byte(dev,(addr << 1) & 0xFE, 1, 0);
	if (ret) {
		dev_err(dev->dev, "i2c_send_reg, send addr fail\n");
		return ret;
	}

	ret = hal_i2c_raw_send_byte(dev,reg, 0, 0);
	if (ret) {
		dev_err(dev->dev, "i2c_send_reg, send reg fail\n");
		return ret;
	}

	ret = hal_i2c_raw_send_byte(dev,data, 0, 1);
	if (ret) {
		dev_err(dev->dev, "i2c_send_reg, send data fail\n");
		return ret;
	}

	return 0;
}
#endif

int rda_i2c_send_bytes(struct rda_i2c_dev *dev,u8 addr, u8 *data, int len,bool stop)
{
	int ret, i;
	//unsigned long timerout;
	//u32 msessc;
	//timerout = jiffies;
	if (len < 1)
		return -1;
	rda_dbg_i2c("%s, addr =0x%x , len =%d ,stop =%d\n",
			__func__, addr, len, stop);
	ret = hal_i2c_raw_send_byte(dev,(addr << 1) & 0xFE, 1, 0);
	if (ret) {
		//dev_err(dev->dev, "%s, send addr fail, addr = 0x%02x\n",
		//	__func__, addr);
		return ret;
	}

	for (i=0;i<len-1;i++) {
		ret = hal_i2c_raw_send_byte(dev, data[i], 0, 0);
		if (ret) {
			//dev_err(dev->dev, "%s, send data[%d] = 0x%02x fail\n",
			//	__func__, i, data[i]);
			return ret;
		}
	}
	ret = hal_i2c_raw_send_byte(dev, data[len-1], 0,stop);
	if (ret) {
		//dev_err(dev->dev, "%s, send last data 0x%02x fail\n",
		//	__func__, data[len-1]);
		return ret;
	}
	//msessc = jiffies_to_msecs(jiffies-timerout);
	//dev_err(dev->dev, "rda_i2c_send_bytes = %d \n",msessc);
	return 0;
}

int rda_i2c_get_bytes(struct rda_i2c_dev *dev,u8 addr, u8 *data, int len,bool stop)
{
	int ret, i;

	if (len < 1)
		return -1;
	rda_dbg_i2c("%s, addr =0x%x , len =%d ,stop =%d\n",
			__func__, addr, len, stop);
	ret = hal_i2c_raw_send_byte(dev,(addr << 1) | 0x01, 1, 0);
	if (ret) {
		//dev_err(dev->dev, "%s, send addr fail, addr = 0x%02x\n",
		//	__func__, addr);
		return ret;
	}

	for (i=0;i<len-1;i++) {
		ret = hal_i2c_raw_get_byte(dev, &data[i], 0, 0);
		if (ret) {
		//	dev_err(dev->dev, "%s, get data %d fail\n",
			//	__func__, i);
			return ret;
		}
	}

	ret = hal_i2c_raw_get_byte(dev,&data[len-1], 0, stop);
	if (ret) {
		//dev_err(dev->dev, "%s, get last data fail\n", __func__);
		return ret;
	}

	return 0;
}


static int rda_i2c_xfer_msg(struct i2c_adapter *adap,
				 struct i2c_msg *msg, bool stop)
{
	int ret = 0, retry;
	struct rda_i2c_dev *dev;

	dev = i2c_get_adapdata(adap);
	for(retry = 0; retry < adap->retries; retry++){
		if (!(msg->flags & I2C_M_RD)) {
			ret = rda_i2c_send_bytes(dev,msg->addr, msg->buf, msg->len,stop);
		} else {
			ret = rda_i2c_get_bytes(dev,msg->addr, msg->buf, msg->len,stop);
		}

		if (!ret)
                        break;
	}

	return ret;
}

static int
rda_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct rda_i2c_dev *dev = i2c_get_adapdata(adap);
	int i;
	int r = 0;
	//unsigned long timerout;
	//u32 msessc;
	//timerout = jiffies;
	for (i = 0; i < num; i++) {
		r = rda_i2c_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if (r != 0)
		{
			r =-1;
			break;
		}
	}

	if (r == 0){
		r = num;
	}
	else {
		rda_i2c_idle(dev);
	}
	//msessc = jiffies_to_msecs(jiffies-timerout);
	//dev_err(dev->dev, "rda_i2c_xfer = %d ,nem %d \n",msessc,num);
	//timerout = jiffies;
	
	//msessc = jiffies_to_msecs(jiffies-timerout);
	//dev_err(dev->dev, "rda_i2c_idle = %d ,nem %d \n",msessc,num);

	return r;
}

static u32
rda_i2c_func(struct i2c_adapter *adap)
{
	rda_dbg_i2c("rda_i2c_func\n");
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm rda_i2c_algo = {
	.master_xfer	= rda_i2c_xfer,
	.functionality	= rda_i2c_func,
};

static int
rda_i2c_probe(struct platform_device *pdev)
{
	struct rda_i2c_dev	*dev;
	struct i2c_adapter	*adap;
	struct resource	*mem;
	int r = 0;
	struct rda_i2c_device_data *pdata;
	u32 speed = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	dev = kzalloc(sizeof(struct rda_i2c_dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&pdev->dev, "fail to alloc rda_i2c_dev\n");
		return -ENOMEM;
	}

	dev->i2c_regulator = regulator_get(NULL, LDO_I2C);

        if (IS_ERR(dev->i2c_regulator)) {
                dev_err(&pdev->dev,
                        "Failed to get camera regulator\n");
                goto err_free_mem;
        }
        r = regulator_enable(dev->i2c_regulator);
        if (r < 0) {
                dev_err(&pdev->dev,
                        "Failed to enable camera regulator\n");
                goto err_regulator;
        }

	pdata = pdev->dev.platform_data;
	if (pdata){
		speed = pdata->speed;
	} else
		speed = 100;	/* Defualt speed */

	dev->speed_khz = speed;
	dev->idle = 1;
	dev->dev = &pdev->dev;
	dev->master_clk = clk_get(NULL, RDA_CLK_APB1);
	if (!dev->master_clk) {
		dev_err(&pdev->dev, "no handler of clock\n");
		r = -EINVAL;
		goto err_regulator;
	}

	dev->base = ioremap(mem->start, resource_size(mem));
	if (!dev->base) {
		dev_err(&pdev->dev, "ioremap fail\n");
		r = -ENOMEM;
		goto err_put_clk;
	}

	rda_dbg_i2c("rda_i2c_probe, mem = %08x, speed = %d\n",
		mem->start, speed);

	platform_set_drvdata(pdev, dev);
	dev->master_id = pdev->id;
	
	rda_dbg_i2c("rda_i2c_probe  I2C %d\n",dev->master_id);
	/* reset ASAP, clearing any IRQs */
	rda_i2c_init(dev);

	rda_i2c_idle(dev);

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strncpy(adap->name, "RDA I2C adapter", sizeof(adap->name));
	adap->algo = &rda_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->retries = 3;

	/* i2c device drivers may be active on return from add_adapter() */
	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(dev->dev, "failure adding adapter\n");
		goto err_free_all;
	}

	dev_info(dev->dev, "rda_i2c, adapter %d, bus %d, at %d kHz\n",
		 adap->nr, pdev->id, dev->speed_khz);
	return 0;

err_free_all:
	rda_i2c_idle(dev);
	platform_set_drvdata(pdev, NULL);
	iounmap(dev->base);
err_put_clk:
	clk_put(dev->master_clk);
err_regulator:
	if (!IS_ERR(dev->i2c_regulator)) {
		regulator_disable(dev->i2c_regulator);
		regulator_put(dev->i2c_regulator);
	}
err_free_mem:
	kfree(dev);

	return r;
}

static int
rda_i2c_remove(struct platform_device *pdev)
{
	struct rda_i2c_dev	*dev = platform_get_drvdata(pdev);
	
	if (!IS_ERR(dev->i2c_regulator)) {
		regulator_disable(dev->i2c_regulator);
		regulator_put(dev->i2c_regulator);
	}
	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&dev->adapter);
	clk_put(dev->master_clk);
	kfree(dev);
	return 0;
}

static struct platform_driver rda_i2c_driver = {
	.probe		= rda_i2c_probe,
	.remove		= rda_i2c_remove,
	.driver		= {
		.name	= RDA_I2C_DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init i2c_rda_init(void)
{
	return platform_driver_register(&rda_i2c_driver);
}

static void __exit i2c_rda_exit(void)
{
	platform_driver_unregister(&rda_i2c_driver);
}

MODULE_AUTHOR("Wang Lei <leiwang@rdamicro.com>");
MODULE_DESCRIPTION("RDA I2C bus");
MODULE_LICENSE("GPL");

module_init(i2c_rda_init);
module_exit(i2c_rda_exit);
