#include <common.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/reg_spi.h>
#include <asm/arch/reg_gpio.h>
#include <asm/arch/reg_cfg_regs.h>
#include <asm/arch/reg_timer.h>
#include <asm/arch/reg_i2c.h>
#include <asm/arch/ispi.h>

#ifdef CONFIG_IRQ
#include "irq.h"
#endif

/* I2C debug enable or disable */
#define rprintf printf
#define TEST_I2C_DEBUG

#ifdef TEST_I2C_DEBUG
#define d_printf rprintf
#else
#define d_printf(...) do{}while(0)
#endif

/* TX FIFO size and timeout count */
#define I2C_TX_FIFO_SIZE (0x1F)
#define I2C_WAIT_SLOT	 (20)
#define I2C_WAIT_JITTER	 (10)
#define I2C_WAIT_COUNT	 (I2C_TX_FIFO_SIZE + I2C_WAIT_JITTER)
#define I2C_WAIT_TIME	 (I2C_WAIT_SLOT * (I2C_TX_FIFO_SIZE + I2C_WAIT_JITTER))

/* APB2 bus frequency value 200MHz */
#define APB2_BUS_FREQ_VALUE (200000000)

/* Device address */
#define I2C_SLAVE_ADDR_TS_MSG2133	(0x60)

//#define I2C_TX_BUF_SIZE	 (0x100)
//#define I2C_RX_BUF_SIZE	 (0x100)

/* I2C3 and Touch Sensor GPIO PINs */
#define GPIO_A1	1
#define GPIO_B2	2
#define GPIO_B6	6
#define GPIO_B7	7

#define GPIO_TOUCH_IRQ		GPIO_A1
#define GPIO_TOUCH_RESET	GPIO_B2
#define GPIO_TOUCH_SCL		GPIO_B6
#define GPIO_TOUCH_SDA		GPIO_B7

#define BANK_TOUCH_IRQ		hwp_apGpioA
#define BANK_TOUCH_RESET	hwp_apGpioB
#define BANK_TOUCH_SCL		hwp_apGpioB
#define BANK_TOUCH_SDA		hwp_apGpioB
#define BANK_PIN_CFG		hwp_configRegs

static int g_i2c_speed = 200;
#ifdef CONFIG_IRQ
static int g_i2c_flag_tx_msg_ok = 0;
static int g_i2c_flag_rx_msg_ok = 0;
static int g_i2c_flag_tx_msg_err = 0;
#endif /* CONFIG_IRQ */
static HWP_I2C_MASTER_T *g_hwp_i2c = hwp_i2cMaster;
static HWP_I2C_MASTER_T *g_i2c_tab[] = {hwp_i2cMaster,hwp_i2cMaster2,hwp_i2cMaster3};

static void touch_sensor_init(void);
static void i2c_test_init(void);
static void i2c_buffer_init(void);
static void dump_i2c_reg(void);
static unsigned int get_ticks_x(unsigned int *ptr_high,unsigned int *ptr_low);
static unsigned int get_ticks_x_x(void);

#ifdef CONFIG_IRQ
static void i2c_tx_callback(void);
static void i2c_rx_callback(void);
#endif /* CONFIG_IRQ */

#if 0
static HWP_SPI_T *hwp_ispi = hwp_spi3;
static void ispi_open(int modemSpi);
static UINT32 ispi_reg_read(UINT32 regIdx);
static void ispi_reg_write(UINT32 regIdx, UINT32 value);
#endif

static void i2c_config_clock(int speed_khz)
{
	unsigned int clock,clk_div,mclk = APB2_BUS_FREQ_VALUE;
	unsigned int ctrl_reg;

	clock = speed_khz * 1000;
	clk_div = mclk / (5 * clock);
	if(mclk % (5 * clock))
		clk_div += 1;
	if(clk_div >= 1)
		clk_div -= 1;
	if(clk_div > 0xFFFF)
		clk_div = 0xFFFF;

	ctrl_reg = g_hwp_i2c->CTRL;
	ctrl_reg &= (~I2C_MASTER_CLOCK_PRESCALE_MASK);
	ctrl_reg |= I2C_MASTER_CLOCK_PRESCALE(clk_div);

	g_hwp_i2c->CTRL = 0;
	g_hwp_i2c->CTRL = ctrl_reg;

	d_printf("ctrl = %x,clk_div = %d,speed = %d Kbps\n"
			,g_hwp_i2c->CTRL,clk_div,speed_khz);
}

static void i2c_enable(int enabled)
{
	if(enabled){
		g_hwp_i2c->CTRL |= (I2C_MASTER_EN | (0x3 << 14));
		g_hwp_i2c->CTRL1 = 0;
	} else {
		g_hwp_i2c->CTRL &= (~I2C_MASTER_EN);
	}
}

static void i2c_test_init(void)
{
	i2c_config_clock(g_i2c_speed);
	i2c_enable(1);
}

static void i2c_buffer_init(void)
{
#if 0
	int i;
	for(i = 0;i < I2C_TX_BUF_SIZE;i++)
		g_i2c_tx_buf[i] = (unsigned char)i;
	for(i = 0;i < I2C_RX_BUF_SIZE;i++)
		g_i2c_rx_buf[i] = (unsigned char)0x5a;
#endif
}

static int i2c_tx_byte(unsigned char dat,int start,int stop)
{
	unsigned int cmd = I2C_MASTER_WR;

	if(start)
		cmd |= I2C_MASTER_STA;
	if(stop)
		cmd |= I2C_MASTER_STO;

	g_hwp_i2c->TXRX_BUFFER = dat;
	g_hwp_i2c->CMD = cmd;
	d_printf("i2c tx data:%2x,cmd:%x\n",dat,cmd);
	return 0;
}

static int i2c_rx_byte(unsigned char *buf,int start,int stop)
{
	unsigned int cmd = I2C_MASTER_RD;

	if(start)
		cmd |= I2C_MASTER_STA;
	if(stop)
		cmd |= (I2C_MASTER_ACK | I2C_MASTER_STO);
	g_hwp_i2c->CMD = cmd;
//	d_printf("i2c_rx_byte, cmd = %x\n",cmd);
	return 0;
}

#ifdef CONFIG_IRQ
static void i2c_enable_irq(int enabled)
{
	if(enabled) {
		g_hwp_i2c->CTRL |= I2C_MASTER_IRQ_MASK;
	} else {
		g_hwp_i2c->CTRL &= (~I2C_MASTER_IRQ_MASK);
	}
}

static void i2c_enable_irq_tx_fifo_empty(int enabled)
{
	if(enabled) {
		g_hwp_i2c->CTRL1 |= I2C_MASTER_TXFIFO_UNDER_IRQ_MASK;
		g_hwp_i2c->IRQ_CLR = I2C_MASTER_IRQ_CLR;
	} else {
		g_hwp_i2c->CTRL1 &= (~I2C_MASTER_TXFIFO_UNDER_IRQ_MASK);
	}
}
#endif /* CONFIG_IRQ */

static inline void i2c_set_rx_number(unsigned int n)
{
	unsigned int reg = g_hwp_i2c->CTRL;

	reg &= (~I2C_MASTER_RX_READ_NUM_MASK);
	reg |= I2C_MASTER_RX_READ_NUM(n);
	g_hwp_i2c->CTRL = reg;
}

static inline int i2c_get_rx_number(void)
{
	return (((g_hwp_i2c->STATUS & I2C_MASTER_RX_FIFO_DATA_NUM_MASK) >> 21) & 0x1F);
}

static inline void i2c_clear_fifo(void)
{
	g_hwp_i2c->CTRL |= I2C_MASTER_CLEAR_FIFO;
	g_hwp_i2c->CTRL &= ~I2C_MASTER_CLEAR_FIFO;
}

static inline int i2c_get_free_space(void)
{
	return (((g_hwp_i2c->STATUS & I2C_MASTER_TX_FIFO_FREE_NUM_MASK) >> 26) & 0x1F);
}

#if 0
//#ifdef TEST_I2C_DEBUG
static int i2c_bus_is_busy(void)
{
	unsigned int status;
	status = (g_hwp_i2c->STATUS &
			(I2C_MASTER_TIP | I2C_MASTER_BUSY));
	d_printf("i2c_bus_is_busy, status: %x\n",g_hwp_i2c->STATUS);
	return status;
}
#else
static inline int i2c_bus_is_busy(void)
{
	return (g_hwp_i2c->STATUS & (I2C_MASTER_TIP | I2C_MASTER_BUSY));
}
#endif

#if 0
static inline int i2c_is_tx_fifo_empty(void)
{
	return (g_hwp_i2c->STATUS & I2C_MASTER_IRQ_TX_UDF);
}

static void i2c_stop(void)
{
	d_printf("ctrl  : %x\n",g_hwp_i2c->CTRL);
	d_printf("status: %x\n",g_hwp_i2c->STATUS);

	g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;

	g_hwp_i2c->CTRL |= I2C_MASTER_CLEAR_FIFO;
	g_hwp_i2c->CTRL &= ~I2C_MASTER_CLEAR_FIFO;

	d_printf("ctrl  : %x\n",g_hwp_i2c->CTRL);
	d_printf("status: %x\n",g_hwp_i2c->STATUS);
	d_printf("i2c stoped\n");
}
#endif
static void dump_i2c_reg(void)
{
	rprintf("\n");
	rprintf("ctrl   :  %x\n",g_hwp_i2c->CTRL);
	rprintf("ctrl1  :  %x\n",g_hwp_i2c->CTRL1);
	rprintf("status :  %x\n",g_hwp_i2c->STATUS);
	rprintf("cmd    :  %x\n",g_hwp_i2c->CMD);
	rprintf("\n");
}

void i2c_case_check_register_default_value(void)
{
	rprintf("\ncheck register default value ...\n");
	//TODO: reset i2c
	rprintf("base addr:   %x\n",(unsigned int)g_hwp_i2c);
	dump_i2c_reg();
}

int i2c_test_tx_data_pol(unsigned char addr,const unsigned char *dat,int len)
{
	int i;
	unsigned char temp;
	static volatile unsigned int timeout = 0;

	rprintf("\ni2c tx data polling test,addr = %x, len = %d\n",
			(unsigned int)dat,len);

	/* wait until bus is idle */
	rprintf("clear fifo ...\n");
	i2c_clear_fifo();
	rprintf("wait i2c busy until it is idle ...\n");
	timeout = I2C_WAIT_COUNT;
	while(g_hwp_i2c->STATUS & (I2C_MASTER_TIP | I2C_MASTER_BUSY)) {
		if(timeout == 0) {
			dump_i2c_reg();
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("i2c bus is busy,exit\n");
			return -1;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* send first byte: device address,RW = 0 */
	temp = (addr << 1) & 0xFE;
	i2c_tx_byte(temp,1,0);
	/* send data byte array */
	for(i = 0;i < len - 1;i++) {
		while(i2c_get_free_space() == 0);
		i2c_tx_byte(dat[i],0,0);
	}
	/* send last byte */
	i2c_tx_byte(dat[len-1],0,1);
	/* wait sending finish */
	rprintf("wait for tx finish ...\n");
	if(1) {
		timeout = I2C_WAIT_COUNT;
		while(!(g_hwp_i2c->STATUS & (I2C_MASTER_IRQ_TX_UDF))) {
			if(timeout == 0) {
				rprintf("dump i2c register ...\n");
				dump_i2c_reg();
				g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
				rprintf("i2c send data timeout %d\n",
						I2C_WAIT_TIME);
				return -2;
			}
			udelay(I2C_WAIT_SLOT);
			timeout--;
			dump_i2c_reg();
		}
		g_hwp_i2c->CMD = 0;
		g_hwp_i2c->CTRL1 = 0;
		g_hwp_i2c->CTRL |= I2C_MASTER_CLEAR_FIFO;
		g_hwp_i2c->CTRL	&= ~I2C_MASTER_CLEAR_FIFO;
	}
	rprintf("test done !!!\n");
	return 0;
}
#ifdef CONFIG_IRQ
static void i2c_tx_callback(void)
{
	if(g_hwp_i2c->STATUS & I2C_MASTER_IRQ_TX_UDF) {
		g_i2c_flag_tx_msg_ok = 1;
	} else {
		g_i2c_flag_tx_msg_err++;
	}
	rprintf("i2c_tx_callback\n");
	/* clear irq status */
	g_hwp_i2c->IRQ_CLR = 0x01;
}

static inline void wr_i2c_bus(unsigned char dat,unsigned int cmd)
{
	g_hwp_i2c->TXRX_BUFFER = dat;
	g_hwp_i2c->CMD	= cmd;
}

static inline void rd_i2c_bus(unsigned char dat,unsigned int cmd)
{
	g_hwp_i2c->CMD = cmd;
}

int i2c_test_tx_data_int(unsigned char addr,const unsigned char *dat,int len)
{
	int i,err = 0;
	unsigned int sta_cmd,sto_cmd,dat_cmd,busy;
	volatile unsigned int timeout;

	rprintf("\ni2c tx data interrupt test,addr = %x, len = %d\n",
			(unsigned int)dat,len);

	dat_cmd = I2C_MASTER_WR;
	sta_cmd = I2C_MASTER_STA | I2C_MASTER_WR;
	sto_cmd = I2C_MASTER_STO | I2C_MASTER_WR;
	busy	= I2C_MASTER_TIP | I2C_MASTER_BUSY;
	timeout = I2C_WAIT_COUNT;
	addr	= (addr << 1) & 0xFE;

	/* waitting for bus idle */
	while(g_hwp_i2c->STATUS & busy) {
		if(timeout == 0) {
			rprintf("dump i2c register ...\n");
			dump_i2c_reg();
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("i2c bus is busy,exit\n");
			return -1;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* ready data */
	timeout = I2C_WAIT_COUNT;
	g_i2c_flag_tx_msg_ok = 0;
	g_i2c_flag_tx_msg_err = 0;
	/*
	 * register a interrupt handler
	 * When the first byte is moved to shift register from tx fifo and the
	 * tx fifo is empty,the TX_UDF will be set 1. A interrupt should be
	 * triggered by TX_UDF and MCU begin to run a interrupt service funtion
	 * named callback().
	 */
	i2c_enable_irq(0);
	/* clear tx fifo and flag */
	i2c_clear_fifo();
	/* register a interrupt handler */
	irq_request(RDA_IRQ_I2C,i2c_tx_callback);
	/* enable gic interrupt */
	irq_unmask(RDA_IRQ_I2C);
	/* enable TX_UDF inerrupt */
	i2c_enable_irq(1);
	/* enable tx fifo under flow interrupt */
	g_hwp_i2c->CTRL1 |= I2C_MASTER_TXFIFO_UNDER_IRQ_MASK;
	/* begin to send data */
	wr_i2c_bus(addr,sta_cmd);
	for(i = 0;i < len - 1;i++) {
		while(i2c_get_free_space() == 0);
		wr_i2c_bus(dat[i],dat_cmd);
	}
	wr_i2c_bus(dat[i],sto_cmd);
	/* wait sending finish */
	while(1) {
		if(g_i2c_flag_tx_msg_ok) {
			err = 0;
			rprintf("transmit data ok !\n");
			break;
		}
		if(!timeout) {
			err = -2;
			rprintf("dump i2c register ...\n");
			dump_i2c_reg();
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("transmit data timeout %d !\n",
					I2C_WAIT_TIME);
			break;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* disable GIC ieterrupt */
	irq_mask(RDA_IRQ_I2C);
	irq_free(RDA_IRQ_I2C);
	i2c_enable_irq(0);
	g_hwp_i2c->CMD = 0;
	g_hwp_i2c->CTRL1 = 0;
	i2c_clear_fifo();
#if 0
	rprintf("dump i2c register ...\n");
	dump_i2c_reg();
#endif
	if(g_i2c_flag_tx_msg_err) {
		/*
		 * if g_i2c_flag_msg_err is not 0, this means
		 * a interrupt is triggered while there is no
		 * anyone IRQ source is enabled of I2C.
		 */
		rprintf("IRQ error occur ###,error is %d\n",
				g_i2c_flag_tx_msg_err);
		while(1);//halt
	}
	if(err)
		rprintf("test error ###\n",err);
	else
		rprintf("test done !!!\n");
	return err;
}
#else
int i2c_test_tx_data_int(unsigned char addr,const unsigned char *dat,int len){return 0;}
#endif /* CONFIG_IRQ */

int i2c_case_test_tx_busy_state(unsigned char addr)
{
	unsigned char i = 0;
	unsigned int sta_cmd,sto_cmd,dat_cmd;
	unsigned int status;

	rprintf("\ni2c tx busy state test,addr is %x\n",addr);
	sta_cmd = I2C_MASTER_STA | I2C_MASTER_WR;
	sto_cmd = I2C_MASTER_STO | I2C_MASTER_WR;
	dat_cmd  = I2C_MASTER_WR;
	addr = (addr << 1) & 0xFE;

	/* wait until bus is idle */
	rprintf("dump i2c register\n");
	dump_i2c_reg();
	rprintf("check busy state\n");
	if(!(g_hwp_i2c->STATUS & I2C_MASTER_BUSY)) {
		rprintf("i2c bus is idle,send some bytes\n");
		/* send address byte */
		g_hwp_i2c->TXRX_BUFFER = addr;
		g_hwp_i2c->CMD = sta_cmd;
		/* load data into tx fifo */
		for(i = 0;i < I2C_TX_FIFO_SIZE - 2;i++) {
			g_hwp_i2c->TXRX_BUFFER = i;
			g_hwp_i2c->CMD = dat_cmd;
		}
		g_hwp_i2c->TXRX_BUFFER = 0xED;
		g_hwp_i2c->CMD = sto_cmd;

		status = g_hwp_i2c->STATUS;
		rprintf("status is %x\n",status);
		if(!(status & I2C_MASTER_BUSY)) {
			rprintf("i2c bus is still idle after fill tx fifo\n"
					"test error ###\n");
			return -1;
		}
		dump_i2c_reg();
		rprintf("delay ...\n");
		mdelay(10);
		dump_i2c_reg();
	}
	if(g_hwp_i2c->STATUS & (I2C_MASTER_BUSY)) {
		rprintf("i2c bus is busy,delay ...\n");
		mdelay(100);
	} else {
		rprintf("i2c bus is idle after sending some bytes\n"
				"test done !!!\n");
		return 0;
	}
	rprintf("check busy flag again\n");
	dump_i2c_reg();
	if(g_hwp_i2c->STATUS & (I2C_MASTER_BUSY)) {
		rprintf("i2c bus is still busy, send stop cmd ...\n");
		i2c_clear_fifo();
		g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
		rprintf("delay ...\n");
		mdelay(100);
	}
	dump_i2c_reg();
	if(g_hwp_i2c->STATUS & I2C_MASTER_BUSY) {
		rprintf("i2c bus is busy after sending stop cmd\n"
				"test error ###\n");
		return -1;
	} else {
		rprintf("i2c bus is idle after sending stop cmd\n"
				"test done !!!\n");
		return 0;
	}
}

int i2c_case_test_tx_speed(unsigned char addr,unsigned int speed)
{
	unsigned char i = 0x55;
	unsigned int sta_cmd,sto_cmd,dat_cmd;
	unsigned int emp_num,busy;
	unsigned int tick_a,tick_b;
	unsigned int dur_time,real_speed,sum_bit;

	rprintf("\ni2c tx speed test,addr is %x\n",addr);

	busy = I2C_MASTER_BUSY | I2C_MASTER_TIP;
	sta_cmd = I2C_MASTER_STA | I2C_MASTER_WR;
	sto_cmd = I2C_MASTER_STO | I2C_MASTER_WR;
	dat_cmd  = I2C_MASTER_WR;
	addr = (addr << 1) & 0xFE;

	/* wait until bus is idle */
	rprintf("clear fifo\n");
	i2c_clear_fifo();

	rprintf("check busy\n");
	if(g_hwp_i2c->STATUS & busy) {
		rprintf("i2c bus is busy,delay ...\n");
		mdelay(100);
	}
	if(g_hwp_i2c->STATUS & busy) {
		rprintf("dump i2c register\n");
		dump_i2c_reg();
		rprintf("i2c bus is busy,exit\n");
		return -1;
	}

	rprintf("i2c bus is idle,send some bytes\n");
	/* mark current ticks value */
	tick_a = get_ticks_x(NULL,NULL);
	/* send address byte */
	g_hwp_i2c->TXRX_BUFFER = addr;
	g_hwp_i2c->CMD = sta_cmd;
	/* load data into tx fifo */
	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = sto_cmd;

	do {
		emp_num = (g_hwp_i2c->STATUS >> 26) & I2C_TX_FIFO_SIZE;
	}while(emp_num != I2C_TX_FIFO_SIZE);

	/* mark current ticks value */
	tick_b = get_ticks_x(NULL,NULL);
	rprintf("tick_a: %d, tick_b: %d,b-a: %d\n"
			,tick_a,tick_b,tick_b-tick_a);

	// assume HWTIMER clock frequency is 2MHz,1 tick is 0.5 us
	dur_time = ((tick_b - tick_a) * 10) / 2;
	sum_bit = 20 * (8 + 1) + 2 - 10;
	real_speed = ((sum_bit * 1000) * 10) / dur_time;

	//the real speed is 184.846 kbps
	rprintf("real speed is %d kbps,the expected speed is %d kbps\n"
			,real_speed,speed);
	rprintf("test done !!!\n");
	return 0;
}

int i2c_case_test_tx_speed_with_busy(unsigned char addr,unsigned int speed)
{
	unsigned char i = 0x55;
	unsigned int sta_cmd,sto_cmd,dat_cmd;
	unsigned int emp_num,busy,status;
	unsigned int tick_a,tick_b,tick_c;
	unsigned int dur_time,real_speed,sum_bit;

	rprintf("\ni2c tx speed test,addr is %x\n",addr);

	busy = I2C_MASTER_BUSY | I2C_MASTER_TIP;
	sta_cmd = I2C_MASTER_STA | I2C_MASTER_WR;
	sto_cmd = I2C_MASTER_STO | I2C_MASTER_WR;
	dat_cmd  = I2C_MASTER_WR;
	addr = (addr << 1) & 0xFE;

	/* wait until bus is idle */
	rprintf("clear fifo\n");
	i2c_clear_fifo();

	rprintf("check busy\n");
	if(g_hwp_i2c->STATUS & busy) {
		rprintf("i2c bus is busy,delay ...\n");
		mdelay(100);
	}
	if(g_hwp_i2c->STATUS & busy) {
		rprintf("dump i2c register\n");
		dump_i2c_reg();
		rprintf("i2c bus is busy,exit\n");
		return -1;
	}

	rprintf("i2c bus is idle,send some bytes\n");
	/* mark time before start to write data to buffer */
	tick_a = get_ticks_x_x();
	/* send address byte */
	g_hwp_i2c->TXRX_BUFFER = addr;
	g_hwp_i2c->CMD = sta_cmd;
	/* load data into tx fifo */
	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = dat_cmd;

	g_hwp_i2c->TXRX_BUFFER = i;
	g_hwp_i2c->CMD = sto_cmd;

	/* wait until tx fifo is empty */
	do {
		emp_num = (g_hwp_i2c->STATUS >> 26) & I2C_TX_FIFO_SIZE;
	}while(emp_num != I2C_TX_FIFO_SIZE);

	/* mack time when tx fifo is empty */
	tick_b = get_ticks_x_x();

	/* wait until busy flag is cleared */
	do {
		status = (g_hwp_i2c->STATUS & busy);
	}while(status);

	/* mark time when busy state is cleared */
	tick_c = get_ticks_x_x();

	rprintf("tick_a: %d, tick_b: %d,tick_c: %d,"
		"b - a: %d, c - a: %d, c - b: %d\n"
		,tick_a,tick_b,tick_c,
		tick_b - tick_a,tick_c - tick_a,tick_c - tick_b);

	// assume HWTIMER clock frequency is 2MHz,1 tick is 0.5 us
	dur_time = ((tick_b - tick_a) * 10) / 2;
	sum_bit = 20 * (8 + 1) + 2 - 10;
	real_speed = ((sum_bit * 1000) * 10) / dur_time;

	//the real speed is 184.846 kbps when I2C is  configured 200 kbps
	rprintf("real speed is %d Kbps,the expected speed is %d Kbps\n"
			,real_speed,speed);
	rprintf("test done !!!\n");
	return 0;
}

int i2c_test_rx_data_pol(unsigned char addr,unsigned char *buf,int len)
{
	int i = 0;
	unsigned char temp;
	int rx_len;
	static volatile unsigned int timeout = 0;

	rprintf("\ni2c rx data polling test,addr = %x,len = %d\n",
			(unsigned int)buf,len);
	len = (len > I2C_TX_FIFO_SIZE) ? I2C_TX_FIFO_SIZE : len;

//	rprintf("clear fifo ...\n");
//	i2c_clear_fifo();

	rprintf("waiting for bus idle ...\n");
	timeout = I2C_WAIT_COUNT;
	while(g_hwp_i2c->STATUS & (I2C_MASTER_TIP | I2C_MASTER_BUSY)) {
		if(timeout == 0) {
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("dump register ...\n");
			dump_i2c_reg();
			rprintf("i2c bus is busy,exit\n");
			return -1;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* send address,RW = 1 */
	temp = (addr << 1) | 0x01;
	i2c_tx_byte(temp,1,0);
#if 1
	/* wait tx fifo empty,maybe the data walking on the bus */
	timeout = I2C_WAIT_COUNT;
	while(!(g_hwp_i2c->STATUS & I2C_MASTER_IRQ_TX_UDF)) {
		if(timeout == 0) {
			g_hwp_i2c->CMD = I2C_MASTER_STO;
			rprintf("i2c send address timeout %d\n",
					I2C_WAIT_TIME);
			rprintf("dump register ...\n");
			dump_i2c_reg();
			return -2;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
#endif
	/* set rx data number */
	i2c_set_rx_number(len);
	/* set rx data command */
	i2c_rx_byte(NULL,0,1);
	/* wait until rx data finished */
	timeout = I2C_WAIT_COUNT;
	while(1) {
		rx_len = i2c_get_rx_number();
		if(rx_len >= len) {
			rprintf("received %d bytes: ",rx_len);
			break;
		}
		if(timeout == 0) {
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("i2c receive data timeout %d\n",
					I2C_WAIT_TIME);
			rprintf("dump register ...\n");
			dump_i2c_reg();
			return -3;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* store data */
	rprintf("received data:\n");
	for(i = 0;i < rx_len;i++) {
		buf[i] = g_hwp_i2c->TXRX_BUFFER;
		rprintf("%2x ",buf[i]);
	}
	rprintf("\n");

	if(rx_len != len) {
		rprintf("test error ###\n");
		i2c_set_rx_number(0);
		i2c_clear_fifo();
		return -3;
	}
	/* clear buffer */
	i2c_set_rx_number(0);
	i2c_clear_fifo();
	rprintf("test done !!!\n");
	return 0;
}

#ifdef CONFIG_IRQ
static void i2c_rx_callback(void)
{
	g_i2c_flag_rx_msg_ok = 1;
	rprintf("i2c_rx_callback\n");
	/* clear irq status */
	g_hwp_i2c->IRQ_CLR = 0x01;
}

int i2c_test_rx_data_int(unsigned char addr,unsigned char *buf,int len)
{
	int i,err = 0;
	int rx_len = 0;
	volatile unsigned int timeout = I2C_WAIT_COUNT;
	unsigned int busy = I2C_MASTER_TIP | I2C_MASTER_BUSY;
	unsigned int sta_cmd = I2C_MASTER_WR|I2C_MASTER_STA;
	unsigned int dat_cmd = I2C_MASTER_RD|I2C_MASTER_STO|I2C_MASTER_ACK;
	unsigned char temp = 0;
	unsigned int tick_a = 0,tick_b = 0;

	rprintf("\ni2c rx data interrupts test,addr = %x,len = %d\n",
			(unsigned int)buf,len);
	while(g_hwp_i2c->STATUS & busy) {
		if(timeout == 0) {
			dump_i2c_reg();
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("i2c bus is busy,exit\n");
			return -1;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* disable I2C IRQs */
	i2c_enable_irq(0);
	g_hwp_i2c->CTRL1 = 0;
	g_hwp_i2c->IRQ_CLR = 0x01;
	/* clear TX FIFO */
	i2c_clear_fifo();
	/* ready data */
	timeout = I2C_WAIT_COUNT;
	addr = (addr << 1) | 0x01;
	len = (len > I2C_TX_FIFO_SIZE) ? I2C_TX_FIFO_SIZE : len;
	/* enable GIC IRQs */
	irq_request(RDA_IRQ_I2C,i2c_rx_callback);
	irq_unmask(RDA_IRQ_I2C);
	/* enable I2C IRQs */
	i2c_enable_irq(1);
	/* send address,RW = 1 */
	wr_i2c_bus(addr,sta_cmd);
#if 1
	/* wait tx fifo empty,maybe the data walking on the bus */
	while(!(g_hwp_i2c->STATUS & I2C_MASTER_IRQ_TX_UDF)) {
		if(timeout == 0) {
			g_hwp_i2c->CMD = I2C_MASTER_STO;
			rprintf("i2c send address timeout %d\n",
					I2C_WAIT_TIME);
			dump_i2c_reg();
			return -1;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
#endif
	/* set rx data number */
	i2c_set_rx_number(len);
	/* set rx data command */
	rd_i2c_bus(0,dat_cmd);
	/* clear ok flag */
	timeout = I2C_WAIT_COUNT;
	g_i2c_flag_rx_msg_ok = 0;
	/* enable rx fifo interrupts */
	g_hwp_i2c->CTRL1 |= I2C_MASTER_RXFIFO_NUM_IRQ_MASK;
	/* wait until rx data finished */
	tick_a = get_ticks_x_x();
	while(1) {
		if(!i2c_bus_is_busy()){
			if(g_i2c_flag_rx_msg_ok) {
				err = 0;
				tick_b = get_ticks_x_x();
				rx_len = i2c_get_rx_number();
			//	rprintf("%x\n",g_hwp_i2c->STATUS); // for TIP
				rprintf("received %d bytes: ",rx_len);
				break;
			}
		}
		if(timeout == 0) {
			err = -2;
			g_hwp_i2c->CMD = I2C_MASTER_STO | I2C_MASTER_FORCE_STO;
			rprintf("i2c receive data timeout %d us\n",
					I2C_WAIT_TIME);
			dump_i2c_reg();
			break;
		}
		udelay(I2C_WAIT_SLOT);
		timeout--;
	}
	/* disable I2C rx fifo interrupts */
	g_hwp_i2c->CTRL1 &= (~I2C_MASTER_RXFIFO_NUM_IRQ_MASK);
	/* store data */
	if(buf != NULL) {
		for(i = 0;i < rx_len;i++)
			buf[i] = g_hwp_i2c->TXRX_BUFFER;
	} else {
		temp = temp;
		for(i = 0;i < rx_len;i++)
			temp = g_hwp_i2c->TXRX_BUFFER;
	}
	/*
	 * there is a very strange issue:
	 * if we do disable irq before clear fifo
	 * the TIP status will not be reset to 0
	 * so we should set rx number to 0 and
	 * clear the RX fifo before disable I2C
	 * IRQs
	 */
//	i2c_enable_irq(0); // for TIP
	/* clear buffer */
	i2c_set_rx_number(0);
	i2c_clear_fifo();
	/* disable I2C IRQs */
	i2c_enable_irq(0);// for TIP
	/* disable GIC IRQs */
	irq_mask(RDA_IRQ_I2C);
	irq_free(RDA_IRQ_I2C);
	if(buf != NULL) {
		rprintf("i2c receive data: \n");
		for(i = 0;i < rx_len;i++)
			rprintf(" %2x",buf[i]);
		rprintf("\n");
	}
	rprintf("tick_b: %d, tick_a: %d,b-a: %d\n"
			,tick_b,tick_a,tick_b-tick_a);
	dump_i2c_reg();
	/* check received data length */
	if((!err) && (rx_len >= len)) {
		rprintf("test done !!!\n");
	} else {
		rprintf("test error ###\n");
	}
	return err;
}
#else
int i2c_test_rx_data_int(unsigned char addr,unsigned char *buf,int len){return 0;}
#endif /* CONFIG_IRQ */

int i2c_case_test_receiving_msg_pol(unsigned char addr,
					unsigned char *buf,
					int len,
					int times)
{
	int ret = 0;

	rprintf("\ni2c receiving messsage polling test.\n"
		"please touch you phone's screen.\n");

	mdelay(1000);
	while(times--) {
		ret += i2c_test_rx_data_pol(addr,buf,len);
		mdelay(1000);
	}
	if(ret !=0)
		rprintf("test error,ret is %d ###\n",ret);
	else
		rprintf("test done !!!\n");
	return ret;
}

int i2c_case_test_transmitting_msg_pol(unsigned char addr,
					const unsigned char *dat,
					int len,
					int times)
{
	int ret = 0;
	rprintf("\ni2c transmitting message polling test ...\n");
	while(times--) {
		ret += i2c_test_tx_data_pol(addr,dat,len);
	}
	if(ret != 0)
		rprintf("test error,ret is %d ###\n",ret);
	else
		rprintf("test done !!!\n");
	return ret;
}

int i2c_case_test_transmitting_speed(unsigned char addr)
{
	unsigned int spd_tab[7] = {100,150,200,250,300,350,400};
	unsigned int old_speed = g_i2c_speed;
	int i,ret = 0;

	rprintf("\ni2c transmitting speed test ...\n");
	for(i = 0;i < 7;i++) {
		g_i2c_speed = spd_tab[i];
		i2c_test_init();
		mdelay(100);
		ret += i2c_case_test_tx_speed(addr,g_i2c_speed);
		mdelay(100);
	}
	/* recovery old speed configuration */
	g_i2c_speed = old_speed;
	i2c_test_init();
	mdelay(100);

	if(ret != 0)
		rprintf("test error,ret is %d ###\n",ret);
	else
		rprintf("test done !!!\n");
	return ret;
}

int i2c_case_test_transmitting_msg_irq(unsigned char addr,
					const unsigned char *dat,
					int len,
					int times)
{
	int ret = 0;

	rprintf("\ni2c transmit interrupts test\n");
	while(times--) {
		ret += i2c_test_tx_data_int(addr,dat,len);
	}
	if(ret != 0)
		rprintf("test error,ret is %d\n",ret);
	else
		rprintf("test done !!!\n");
	return ret;
}

int i2c_case_test_receiving_msg_irq(unsigned char addr,
					unsigned char *rx_buf,
					int len,
					int times)
{
	int ret = 0;

	rprintf("\n i2c receiving interrputs test\n");
	while(times--) {
		ret += i2c_test_rx_data_int(addr,rx_buf,len);
	}
	if(ret != 0)
		rprintf("test error,ret is %d\n",ret);
	else
		rprintf("test done !!!\n");
	return  ret;
}

int i2c_check_result(int val,const char *name_str)
{
	if(val != 0) {
		rprintf("\n%s failed ###,error is %d\n"
			,name_str,val);
		//rprintf("test stoped\n");
		//while(1);
		return -1;
	}
	return 0;
}

#if 0
int test_i2c(int id,int times)
{
	unsigned char tx_buf[10] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA};
	unsigned char rx_buf[10] = {0x5A};
	int rx_len;
	int tx_len;
	int ret;
	unsigned char addr;
	volatile unsigned char cmd;

	rprintf("\n");
	if((id > 2) || (id < 0)) {
		rprintf("i2c id %d is invalid,must be 0 ~ 2.exit\n",id);
		return -1;
	}
	if(times <= 0) {
		rprintf("i2c times %d is invalid,exit\n"
				,times);
		return -2;
	}
	rprintf("\nstart to test i2c %d,test %d times\n",id,times);

	g_hwp_i2c 	= g_i2c_tab[id];
	g_i2c_speed 	= 200;
	g_hwp_i2c 	= hwp_i2cMaster3;
	rx_len 	= 4;
	tx_len 	= 10;
	ret 	= 0;
	addr 	=I2C_SLAVE_ADDR_TS_MSG2133;

	touch_sensor_init();
	i2c_test_init();
	i2c_buffer_init();

	while(1) {
		rprintf("> ");
		cmd = serial_getc();
		rprintf("%c\n",cmd);

		switch(cmd) {
		case '0':
			i2c_case_check_register_default_value();
			break;
		case '1':
			ret = i2c_test_tx_data_pol(addr,tx_buf,tx_len);
			i2c_check_result(ret,"tx data pol test");
			break;
		case '2':
			ret = i2c_test_tx_data_int(addr,tx_buf,tx_len);
			i2c_check_result(ret,"tx data int test");
			break;
		case '3':
			ret = i2c_test_rx_data_pol(addr,rx_buf,rx_len);
			i2c_check_result(ret,"rx data pol test");
			break;
		case '4':
			ret = i2c_test_rx_data_int(addr,rx_buf,rx_len);
			i2c_check_result(ret,"rx data int test");
			break;
		case '5':
			ret = i2c_case_test_tx_speed(addr,g_i2c_speed);
			i2c_check_result(ret,"tx speed test");
			break;
		case '6':
			ret = i2c_case_test_tx_speed_with_busy(addr,g_i2c_speed);
			i2c_check_result(ret,"tx speed with busy test");
			break;

		case 'a':
			ret = i2c_case_test_tx_busy_state(addr);
			i2c_check_result(ret,"tx busy state test");
			break;
		case 'b':
			ret = i2c_case_test_transmitting_speed(addr);
			i2c_check_result(ret,"transmitting speed test");
			break;
		case 'c':
			ret = i2c_case_test_receiving_msg_pol(addr,
								rx_buf,
								rx_len,
								10);
			i2c_check_result(ret,"receiving msg pol test");
			break;
		case 'd':
			ret = i2c_case_test_transmitting_msg_pol(addr,
								tx_buf,
								tx_len,
								times);
			i2c_check_result(ret,"transmitting msg pol test");
			break;
		case 'e':
			ret = i2c_case_test_transmitting_msg_irq(addr,
								tx_buf,
								tx_len,
								times);
			i2c_check_result(ret,"transmitting msg irq test");
			break;
		case 'f':
			ret = i2c_case_test_receiving_msg_irq(addr,
								tx_buf,
								rx_len,
								times);
			i2c_check_result(ret,"receiving msg irq test");
			break;
		case 'q':
			rprintf("i2c test quit\n");
			return 0;
		case 'r':
			rprintf("reset i2c");
			i2c_test_init();
			i2c_buffer_init();
			break;
		default:
			break;
		}
		rprintf("\n");
	}
}

#else
int test_i2c(int id,int times)
{
	unsigned char tx_buf[10] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA};
	unsigned char rx_buf[10] = {0x5A};
	int rx_len;
	int tx_len;
	int ret;
	unsigned char addr;

	rprintf("\n");
	if((id > 2) || (id < 0)) {
		rprintf("i2c id %d is invalid,must be 0 ~ 2.exit\n",id);
		return -1;
	}
	if(times <= 0) {
		rprintf("i2c times %d is invalid,exit\n"
				,times);
		return -2;
	}
	rprintf("\nstart to test i2c %d,test %d times\n",id,times);

	g_hwp_i2c 	= g_i2c_tab[id];
	g_i2c_speed 	= 200;
	rx_len 	= 4;
	tx_len 	= 10;
	ret 	= 0;
	addr 	=I2C_SLAVE_ADDR_TS_MSG2133;

	touch_sensor_init();
	i2c_test_init();
	i2c_buffer_init();

	ret = i2c_case_test_tx_speed(addr,g_i2c_speed);
	if(i2c_check_result(ret,"tx speed test"))
		return ret;

	ret = i2c_case_test_tx_speed_with_busy(addr,g_i2c_speed);
	if(i2c_check_result(ret,"tx speed with busy test"))
		return ret;

	ret = i2c_case_test_tx_busy_state(addr);
	if(i2c_check_result(ret,"tx busy state test"))
		return ret;

	ret = i2c_case_test_transmitting_speed(addr);
	if(i2c_check_result(ret,"transmit speed test"))
		return ret;

	ret = i2c_case_test_receiving_msg_pol(addr,rx_buf,rx_len,10);
	if(i2c_check_result(ret,"receiving msg pol test"))
		return ret;

	ret = i2c_case_test_transmitting_msg_pol(addr,tx_buf,tx_len,times);
	if(i2c_check_result(ret,"transmitting msg pol test"))
		return ret;

	ret = i2c_case_test_transmitting_msg_irq(addr,tx_buf,tx_len,times);
	if(i2c_check_result(ret,"transmitting msg irq test"))
		return ret;

	ret = i2c_case_test_receiving_msg_irq(addr,tx_buf,rx_len,times);
	if(i2c_check_result(ret,"receiving msg irq test"))
		return ret;

	rprintf("\n\nall i2c test cases are OK !!!\n");
	rprintf("\n\ni2c test done !!!\n");
	return 0;
}
#endif

#if 0
static void ispi_open(int modemSpi)
{
	UINT32 cfgReg = 0;
	UINT32 ctrlReg = 0;

	/* hard code for now */
	cfgReg = 0x100003;
	ctrlReg = 0x2019d821;

	if (modemSpi)
		hwp_ispi = hwp_mspi2;
	else
		hwp_ispi = hwp_spi3;

	/* Activate the ISPI. */
	hwp_ispi->cfg = cfgReg;
	hwp_ispi->ctrl = ctrlReg;

	/* No IRQ. */
	hwp_ispi->irq = 0;
}

static UINT8 ispi_tx_fifo_avail(void)
{
	UINT8 freeRoom;

	/* Get avail level. */
	freeRoom = GET_BITFIELD(hwp_ispi->status, SPI_TX_SPACE);

	return freeRoom;
}

static BOOL ispi_tx_finished(void)
{
	UINT32 spiStatus;
	spiStatus = hwp_ispi->status;

	/* If ISPI FSM is active and the TX Fifo is empty */
	/* (ie available space == Fifo size), the tf is not done */
	if ((!(hwp_ispi->status & SPI_ACTIVE_STATUS))
		&& (SPI_TX_FIFO_SIZE == GET_BITFIELD(spiStatus, SPI_TX_SPACE))) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static UINT32 ispi_send_data(UINT32 csId, UINT32 data, BOOL read)
{
	UINT32 freeRoom;

	/* Clear data upper bit to only keep the data frame. */
	UINT32 reg = data & ~(SPI_CS_MASK | SPI_READ_ENA_MASK);

	/* Add CS and read mode bit */
	reg |= SPI_CS(csId) | (read ? SPI_READ_ENA : 0);

	/* Enter critical section. */
	//UINT32 status = hwp_sysIrq->SC;

	/* Check FIFO availability. */
	freeRoom = GET_BITFIELD(hwp_ispi->status, SPI_TX_SPACE);

	if (freeRoom > 0) {
		hwp_ispi->rxtx_buffer = reg;

		return 1;
	} else {
		return 0;
	}
}

static UINT32 ispi_get_data(UINT32 * recData)
{
	UINT32 nbAvailable;

	nbAvailable = GET_BITFIELD(hwp_ispi->status, SPI_RX_LEVEL);

	if (nbAvailable > 0) {
		*recData = hwp_ispi->rxtx_buffer;
		return 1;
	} else {
		return 0;
	}
}

static void ispi_reg_write(UINT32 regIdx, UINT32 value)
{
	UINT32 wrData;

	wrData = (0 << 25) | ((regIdx & 0x1ff) << 16) | (value & 0xffff);

	while (ispi_tx_fifo_avail() < 1 ||
		   ispi_send_data(0, wrData, FALSE) == 0) ;

	/* wait until any previous transfers have ended */
	while (!ispi_tx_finished()) ;
}

static UINT32 ispi_reg_read(UINT32 regIdx)
{
	UINT32 wrData, rdData = 0;
	UINT32 count;

	wrData = (1 << 25) | ((regIdx & 0x1ff) << 16) | 0;

	while (ispi_tx_fifo_avail() < 1 ||
		   ispi_send_data(0, wrData, TRUE) == 0) ;

	/* wait until any previous transfers have ended */
	while (!ispi_tx_finished()) ;

	count = ispi_get_data(&rdData);
	if (1 != count)
		rprintf("ABB ISPI count err!");

	rdData &= 0xffff;

	return rdData;
}
#endif

static void touch_sensor_power_init(void)
{
	unsigned int addr,val;

	ispi_open(1);
	/* select vol > 2V */
	addr = 0x07;
	val = ispi_reg_read(addr);
	val &= (~(1<<13));
//	val |= (1<<13);// select vol < 2V
	ispi_reg_write(addr,val);
	/*
	 * enable power in normal mode
	 * set max value
	 */
	addr = 0x28;
	val = ispi_reg_read(addr);
	val |= (1<<13);
	val |= (7 << 3);
	ispi_reg_write(addr,val);
	/* enable power in LP mode */
	addr = 0x29;
	val = ispi_reg_read(addr);
	val |= (1<<13);
	ispi_reg_write(addr,val);

	d_printf("pmu reg %x = %x\n",0x07,(unsigned int)ispi_reg_read(0x07));
	d_printf("pmu reg %x = %x\n",0x28,(unsigned int)ispi_reg_read(0x28));
	d_printf("pmu reg %x = %x\n",0x29,(unsigned int)ispi_reg_read(0x29));

	ispi_open(0);
}

static void touch_sensor_gpio_init(void)
{
	/* touch sensor reset pin direction: output,0 */
	BANK_TOUCH_RESET->gpio_oen_set_out = (1 << GPIO_TOUCH_RESET);
	BANK_TOUCH_RESET->gpio_clr = (1 << GPIO_TOUCH_RESET);
	/* touch sensor irq pin direction: input */
	BANK_TOUCH_IRQ->gpio_oen_set_in = (1 << GPIO_TOUCH_IRQ);
	/* touch sensor i2c 3 pin scl,sda: alt */
	d_printf("BANK_PIN_CFG : %x, BANK_PIN_CFG->AP_GPIO_B_Mode: %x\n"
			,(unsigned int)(BANK_PIN_CFG),BANK_PIN_CFG->AP_GPIO_B_Mode);
	BANK_PIN_CFG->AP_GPIO_B_Mode &= (~(1 << GPIO_TOUCH_SCL));
	BANK_PIN_CFG->AP_GPIO_B_Mode &= (~(1 << GPIO_TOUCH_SDA));
	d_printf("BANK_PIN_CFG : %x, BANK_PIN_CFG->AP_GPIO_B_Mode: %x\n"
			,(unsigned int)(BANK_PIN_CFG),BANK_PIN_CFG->AP_GPIO_B_Mode);
}

static void gpio_set_val(HWP_GPIO_T *ptr_gpio,int pin,int val)
{
	unsigned int mask = 1 << (pin % 32);
	if(val)
		ptr_gpio->gpio_set = mask;
	else
		ptr_gpio->gpio_clr = mask;
	d_printf("ptr_gpio: %x, pin: %x, mask: %x, val: %x\n",
			(unsigned int)ptr_gpio,pin,mask,val);
}

static void touch_sensor_reset(void)
{
	gpio_set_val(BANK_TOUCH_RESET,GPIO_TOUCH_RESET,1);
	mdelay(5);
	gpio_set_val(BANK_TOUCH_RESET,GPIO_TOUCH_RESET,0);
	mdelay(80);
	gpio_set_val(BANK_TOUCH_RESET,GPIO_TOUCH_RESET,1);
	mdelay(80);
}
#if 0
static void touch_sensor_suspend(void)
{
	gpio_set_val(BANK_TOUCH_RESET,GPIO_TOUCH_RESET,0);
	mdelay(10);
}

static void touch_sensor_resume(void)
{
	gpio_set_val(BANK_TOUCH_RESET,GPIO_TOUCH_RESET,1);
	mdelay(1);
	touch_sensor_reset();
}
#endif
static void touch_sensor_init(void)
{
	touch_sensor_power_init();
	touch_sensor_gpio_init();
	touch_sensor_reset();
}

static unsigned int get_ticks_x(unsigned int *ptr_high,unsigned int *ptr_low)
{
	unsigned int val_h,val_l;
	val_h = (unsigned int)hwp_apTimer->HWTimer_LockVal_H;
	val_l = (unsigned int)hwp_apTimer->HWTimer_LockVal_L;
	if(val_h != hwp_apTimer->HWTimer_LockVal_H) {
		val_h = (unsigned int)hwp_apTimer->HWTimer_LockVal_H;
		val_l = (unsigned int)hwp_apTimer->HWTimer_LockVal_L;
	}
	if(ptr_high != NULL)
		*ptr_high = val_h;
	if(ptr_low != NULL)
		*ptr_low = val_l;
	return val_l;
}

static unsigned int get_ticks_x_x(void)
{
	return ((unsigned int)hwp_apTimer->HWTimer_LockVal_L);
}
