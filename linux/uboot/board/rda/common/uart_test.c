#include <asm/arch/cs_types.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/reg_uart.h>
#include <asm/arch/reg_sysctrl.h>

#if defined(CONFIG_MACH_RDA8850E) || defined(CONFIG_MACH_RDA8810H)

/* #define CONFIG_UART_DMA_TEST */

/* #define UART_BAUD_RATE_DEFAULT	115200 */
#define UART_BAUD_RATE_DEFAULT	921600

#define MARRAY_SIZE(x) (sizeof(x)/(x)[0])
#define CONFIG_PUTS_UART_INFO
#define CONFIG_UART_TEST_DEBUG

#ifdef CONFIG_UART_TEST_DEBUG
#define d_printf printf
#else
#define d_printf(...) do{}while(0)
#endif

/*
 *Default uart perpherial configuration
 */
#define SER_TEST_DATA_COUNT	10

#else
#error "CONFIG_MACH_RDA8850E or CONFIG_MACH_RDA8810H is undefined !"
#endif

enum UART_PORT_ID
{
	UART_ID_1 = 0,
	UART_ID_2 = 1,
	UART_ID_3 = 2,//this port is used to print debug message
};

/* UART test case id number*/
enum UART_TC_ID
{
	UART_TC_ID_POL_TRANSMIT = 0,
	UART_TC_ID_DMA_TRANSMIT,
	UART_TC_ID_RX_IRQ,
	UART_TC_ID_RX_OVF_IRQ,
	UART_TC_ID_TX_IRQ,
	UART_TC_ID_CTS_RTS,
	UART_TC_ID_RX_OVF_STATE,
	UART_TC_ID_REG_DEFAULT_VALUE,
	UART_TC_ID_TX_FIFO_DEPTH,
	UART_TC_ID_RX_FIFO_LEVEL,
};

enum UART_TEST_STATE
{
	TEST_OK = 0,
	TEST_ERROR = -1,
};

static HWP_UART_T *hwpt_uart = hwp_uart3;
static HWP_UART_T *hwp_uart_tab[] = {hwp_uart1,hwp_uart2,hwp_uart3};

static int g_uart_id = UART_ID_3;//uart3 is used
static u8 g_uart_buf[2080];
static int serial_baud_rate_tab[] = {
	2400,
	4800,
	9600,
	//14400,
	19200,
	//28800,
	//33600,
	38400,
	57600,
	115200,
	230400,
	460800,
	921600,
	//1843200,
};

/*
 * UART PRIVATE FUNCTION DECLARATION
 *********************************************************
 */

static int set_speed(int speed);
static int config_frame_format(int speed, int data_bit, int parity_bit, int stop_bit);
static void put_case_info(const char *str);
static void clear_error_state(void);
static int tx_is_finished(void);
static void enable_cts_rts(int enabled);
static int reset_uart(int id);
static void set_irq_trigger_level(int rx_triggered,int value);

/*
 * UART FUNCTION IMPLEMENTATION
 *********************************************************
 */

#ifdef CONFIG_UART_DMA_TEST

BOOL hal_UartDmaFifoEmpty(HAL_IFC_REQUEST_ID_T requestId)
{
	UINT8 i;
	BOOL fifoIsEmpty = TRUE;

	/* Check the requested id is not currently already used. */
	for (i = 0; i < SYS_IFC_STD_CHAN_NB ; i++) {
		if (GET_BITFIELD(hwp_sysIfc->std_ch[i].control,
				SYS_IFC_REQ_SRC) == requestId) {
			fifoIsEmpty = hal_IfcChannelIsFifoEmpty(requestId, i);
			return fifoIsEmpty;
		}
	}
	return TRUE;
}

void hal_UartDmaResetFifoTc(HAL_IFC_REQUEST_ID_T requestId)
{
	UINT8 i;
	/* Check the requested id is not currently already used. */
	for (i = 0; i < SYS_IFC_STD_CHAN_NB ; i++) {
		if (GET_BITFIELD(hwp_sysIfc->std_ch[i].control,
				SYS_IFC_REQ_SRC) == requestId){
			hwp_sysIfc->std_ch[i].tc = 0;
			return;
		}
	}
	return;
}

void hal_UartRxDmaPurgeFifo(HAL_IFC_REQUEST_ID_T requestId)
{
	UINT8 i;
	for (i = 0; i < SYS_IFC_STD_CHAN_NB ; i++){
		if (GET_BITFIELD(hwp_sysIfc->std_ch[i].control,
				SYS_IFC_REQ_SRC) == requestId) {
			(void)hal_IfcChannelFlush(requestId, i);
		}
	}
}

UINT32 hal_UartSendData(CONST UINT8* data, UINT32 length)
{
	UINT8  TxChannelNum = 0;
	TxChannelNum = hal_IfcTransferStart(HAL_IFC_UART3_TX,
						(UINT8 *)data,
						length,
						HAL_IFC_SIZE_8_MODE_AUTO);
	if (HAL_UNKNOWN_CHANNEL == TxChannelNum) {
		/* No channel available */
		return 0;
	} else {
		/* all data will be send */
		return length;
	}
}

UINT32 hal_UartGetData(UINT8* destAddress, UINT32 length)
{
	UINT8  RxChannelNum = 0;

	/* (Re)Start transfert, this will reset the timeout counter.
	   Do this before clearing the mask and cause, to prevent
	   a previous unwanted timeout interrupt to occur right between the
	   two clears and then the restart transfert. */

	RxChannelNum = hal_IfcTransferStart(HAL_IFC_UART3_RX,
						destAddress,
						length,
						HAL_IFC_SIZE_8_MODE_AUTO);

	/* check if we got an IFC channel */
	if (HAL_UNKNOWN_CHANNEL == RxChannelNum) {
		/* No channel available */
		/* No data received */
		return 0;
	} else {
		/* all data will be fetched */
		return length;
	}
}

void serial_dma_init(void)
{
	set_speed(UART_BAUD_RATE_DEFAULT);

	hwpt_uart->triggers  = UART_AFC_LEVEL(1); //7 ?
	hwpt_uart->ctrl = UART_ENABLE
		|UART_DATA_BITS_8_BITS
		|UART_TX_STOP_BITS_1_BIT
		|UART_PARITY_ENABLE_NO
		|UART_DMA_MODE_ENABLE;
	/* Allow reception */
	hwpt_uart->CMD_Set    = UART_RTS;
	hwpt_uart->irq_mask   = UART_RX_DMA_DONE | UART_RX_DMA_TIMEOUT;
}

void serial_dma_puts(const UINT8 *s)
{
	UINT32 length = 0;

	length = (UINT32)strlen((const char *)s);

	/* wait until fifo is empty, to prevent overlap original data */
	while(!hal_UartDmaFifoEmpty(HAL_IFC_UART3_TX));
	(void)hal_UartSendData(s, length);
}

void serial_dma_gets(UINT8 *s, UINT32 length)
{
	if (hwpt_uart->irq_cause & UART_RX_DMA_TIMEOUT) {
		hal_UartRxDmaPurgeFifo(HAL_IFC_UART3_RX);
		while(!hal_UartDmaFifoEmpty(HAL_IFC_UART3_RX));
		hwpt_uart->irq_cause |= UART_RX_DMA_TIMEOUT;
	}
	hal_UartDmaResetFifoTc(HAL_IFC_UART3_RX);
	(void)hal_UartGetData(s, length);
}

void serial_dma_getc(UINT8 *s)
{
	serial_dma_gets(s, 1);
}

//this case will test dma mode
// Phepherial  to Memory
// Memory to Phepherial
static void subtest_dma(int len)
{
	u8 rd_buf[50] = {0};
	int cnt = 0;
	cnt = cnt;
	len = (len > 50)?(50):(len);

	//check error flags
	clear_error_state();

	/* Begin to receive data by DMA (Phepherial to Memory) */
	serial_dma_gets(rd_buf,len);
	/* Waiting for DMA Rx done*/
	while(!(hwpt_uart->irq_cause & UART_RX_DMA_DONE));
	/* Clear DMA Rx done flag*/
	hwpt_uart->irq_cause |= UART_RX_DMA_DONE;
	/* Waiting for DMA Tx FIFO is empty*/
	while(!hal_UartDmaFifoEmpty(HAL_IFC_UART3_TX));
	/* Send back data by DMA (Memory to Phepherial)*/
	hal_UartSendData(rd_buf,len);

//	while(!(hwpt_uart->irq_cause & UART_TX_DMA_DONE));
//	hwpt_uart->irq_cause |= UART_TX_DMA_DONE;

	/* Waiting for all data sending finished */
	while((hwpt_uart->status & UART_TX_ACTIVE) == 0);
	while((hwpt_uart->status & UART_TX_ACTIVE) != 0);

	/* Clear DMA Tx done flag */
	if(hwpt_uart->irq_cause & UART_TX_DMA_DONE)
		hwpt_uart->irq_cause |= UART_TX_DMA_DONE;
}

int uart_case_dma_transmitting(void)
{
	int n=0;
	int len = SER_TEST_DATA_COUNT;
	int speed = 0;
	int tab_size = MARRAY_SIZE(serial_baud_rate_tab);

	put_case_info("Start UART DMA transmitting\n");
	serial_init(); //921600 8 N 1
	set_speed(UART_BAUD_RATE_DEFAULT);
	serial_dma_init();
	hwpt_uart->irq_mask |=UART_TX_DMA_DONE;
	hwpt_uart->irq_mask |=UART_RX_DMA_DONE;
	hwpt_uart->irq_mask |=UART_RX_DMA_TIMEOUT;
	while(1) {
		/* Get baudrate value */
		speed = serial_baud_rate_tab[n];

		/* Begin to DMA transmitting */
		//8 N 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//8 E 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//8 O 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//8 N 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);
		//8 E 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);
		//8 O 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);
		//7 N 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//7 E 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//7 O 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_dma(len);
		//7 N 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);
		//7 E 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);
		//7 O 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_dma(len);

		/* Change speed table index */
		n++;
		if(n == tab_size)
			break;
	}
	hwpt_uart->ctrl &= (~(UART_DMA_MODE_ENABLE));
	hwpt_uart->irq_mask &= (~UART_TX_DMA_DONE);
	hwpt_uart->irq_mask &=(~UART_RX_DMA_DONE);
	hwpt_uart->irq_mask &=(~UART_RX_DMA_TIMEOUT);
	put_case_info("Finsh UART DMA transmitting\n");
	return 0;
}
#endif /* CONFIG_UART_DMA_TEST */

static int set_speed(int speed)
{
	u32 clksrc;
	u32 divmode;
	u32 div;
	u32 reg;

	if (speed > 3250000 ) {
	    speed = 3250000;
	    hwpt_uart->ctrl &= (~UART_DIVISOR_MODE);
	    divmode = 4;
	}
	else if (speed < 6342 ) {
	    hwpt_uart->ctrl |= UART_DIVISOR_MODE;
	    divmode = 16;
	} else {
	    hwpt_uart->ctrl &= (~UART_DIVISOR_MODE);
	    divmode = 4;
	}
	div = (26000000 + divmode / 2 * speed) / (divmode * speed) - 2;
	clksrc = SYS_CTRL_AP_UART_SEL_PLL_SLOW;
	//clksrc = SYS_CTRL_AP_UART_SEL_PLL_PLL;
	reg = hwp_sysCtrlAp->Cfg_Clk_Uart[g_uart_id];
	reg &= (~(SYS_CTRL_AP_UART_DIVIDER_MASK));
	reg &= (~(SYS_CTRL_AP_UART_SEL_PLL_PLL));
	reg |= (clksrc | SYS_CTRL_AP_UART_DIVIDER(div));
	hwp_sysCtrlAp->Cfg_Clk_Uart[g_uart_id] = reg;
//        iowrite32(clksrc | SYS_CTRL_AP_UART_DIVIDER(div),
//                &hwp_sysCtrlAp->Cfg_Clk_Uart[g_uart_id]);
	return 0;
}

static int config_frame_format(int speed, int data_bit, int parity_bit,
				int stop_bit)
{
	int config_err = 0;
	u32 ctrl_reg;

	ctrl_reg = hwpt_uart->ctrl;

	/* Set data bit: 8, 7 */
	ctrl_reg &= (~UART_DATA_BITS);
	switch(data_bit) {
	case HAL_UART_8_DATA_BITS:
		ctrl_reg |= UART_DATA_BITS_8_BITS;
		break;
	case HAL_UART_7_DATA_BITS:
		ctrl_reg |= UART_DATA_BITS_7_BITS;
		break;
	default:
		config_err |= 0x01;
		break;
	}
	/* Set parity bit: N, O, E */
	ctrl_reg &= (~UART_PARITY_ENABLE);
	switch(parity_bit) {
	case HAL_UART_NO_PARITY:
		ctrl_reg |= UART_PARITY_ENABLE_NO;
		break;
	case HAL_UART_ODD_PARITY:
		ctrl_reg |= UART_PARITY_ENABLE_YES;
		ctrl_reg &= (~UART_PARITY_SELECT_MARK);
		ctrl_reg |= UART_PARITY_SELECT_ODD;
		break;
	case HAL_UART_EVEN_PARITY:
		ctrl_reg |= UART_PARITY_ENABLE_YES;
		ctrl_reg &= (~UART_PARITY_SELECT_MARK);
		ctrl_reg |= UART_PARITY_SELECT_EVEN;
		break;
	default:
		config_err |= 0x02;
		break;
	}
	/* Set stop bit: 1, 2 */
	ctrl_reg &= (~UART_TX_STOP_BITS);
	switch(stop_bit) {
	case HAL_UART_1_STOP_BIT:
		ctrl_reg |= UART_TX_STOP_BITS_1_BIT;
		break;
	case HAL_UART_2_STOP_BITS:
		ctrl_reg |= UART_TX_STOP_BITS_2_BITS;
		break;
	default:
		config_err |= 0x04;
		break;
	}
	/* Enable UART */
	ctrl_reg |= UART_ENABLE;
	/* Check error */
	if(config_err > 0) {
		d_printf("config frame format failed,error is 0x%x\n",
				config_err);
		return -1;
	}
	/* Write to register */
	hwpt_uart->ctrl = ctrl_reg;
	/* Set baud rate*/
	set_speed(speed);
	return 0;
}

static int get_rx_fifo_level(void)
{
	int rx_level;
	rx_level = (hwpt_uart->status & UART_RX_FIFO_LEVEL_MASK)
			>> UART_RX_FIFO_LEVEL_SHIFT;
	return rx_level;
}

static int get_tx_fifo_empty_space(void)
{
	int tx_space;
	tx_space = (hwpt_uart->status & UART_TX_FIFO_SPACE_MASK)
			>> UART_TX_FIFO_SPACE_SHIFT;
	return tx_space;
}

static int tx_fifo_all_empty(void)
{
	int empty_space = get_tx_fifo_empty_space();
	return ((empty_space == UART_TX_FIFO_SIZE)?(1):(0));
}

static int tx_is_active(void)
{
	int state;
	state = (hwpt_uart->status & UART_TX_ACTIVE);
	return ((state)?(1):(0));
}

static int tx_is_finished(void)
{
	if((tx_fifo_all_empty()) && (!tx_is_active()))
		return 1;//tx finished
	else
		return 0;//tx do not finished
}

static void clear_error_state(void)
{
	u32 state_reg = hwpt_uart->status;
	u32 err_mask = UART_TX_OVERFLOW_ERR
			| UART_RX_OVERFLOW_ERR
			| UART_RX_PARITY_ERR
			| UART_RX_FRAMING_ERR;

	/* Check error flags */
	if(state_reg & err_mask) {
		//reset FIFO
		hwpt_uart->CMD_Set |= UART_RX_FIFO_RESET;
		hwpt_uart->CMD_Set |= UART_TX_FIFO_RESET;
		//clear error by writing
		hwpt_uart->status = err_mask;
		while(1) {
			state_reg = hwpt_uart->status;
			if(state_reg & err_mask)
				hwpt_uart->status = err_mask;
		//	else
			break;
		}
	}
}

static void put_case_info(const char *str)
{
#ifdef CONFIG_PUTS_UART_INFO
	while(!tx_is_finished());
	serial_init(); //8 N 1
	set_speed(UART_BAUD_RATE_DEFAULT);
	serial_puts(str);
	while(!tx_is_finished());
#endif
}

int uart_case_test_tx_fifo_depth(void)
{
	int tx_space = 0;
	//delay somtime
	while(!tx_is_finished());
	mdelay(100);
	//reset tx fifo
	hwpt_uart->CMD_Set |= UART_TX_FIFO_RESET;
	clear_error_state();
	mdelay(1);
	//get fifo empty space number
	tx_space = (hwpt_uart->status & UART_TX_FIFO_SPACE_MASK)
			>> UART_TX_FIFO_SPACE_SHIFT;
	if(tx_space != UART_TX_FIFO_SIZE){
		d_printf("check tx fifo size error, should be %d\n",tx_space);
		return -1;
	} else {
		d_printf("uart tx fifo size is %d\n",tx_space);
		return 0;
	}
}

int uart_case_test_rx_fifo_level(void)
{
	int rx_space = 0;

	while(!tx_is_finished());
	mdelay(100);
	hwpt_uart->CMD_Set |= UART_RX_FIFO_RESET;
	clear_error_state();
	mdelay(1);
	rx_space = (hwpt_uart->status & UART_RX_FIFO_LEVEL_MASK)
			>> UART_RX_FIFO_LEVEL_SHIFT;
	if(rx_space != 0) {
		d_printf("check rx fifo level error,should be %d\n",0);
		return -1;
	} else {
		d_printf("uart rx fifo level is %d\n",rx_space);
		return 0;
	}
}

static void subtest_pol(int len)
{
	u8 rd_buf[50] = {0};
	int cnt = 0;

	len = (len > 50)?(50):(len);
	/* clear error flags */
	clear_error_state();
	/* receive data */
	while(1) {
		if(cnt >= len)
			break;
		rd_buf[cnt] = serial_getc();
		cnt++;
	}
	/* delay n us for host ready */
	mdelay(100);
	/* write data back */
	cnt = 0;
	while(1) {
		if(cnt >= len)
			break;
		// Place in the TX Fifo
    		while ((GET_BITFIELD(hwpt_uart->status, UART_TX_FIFO_SPACE)) == 0);
		hwpt_uart->rxtx_buffer = (UINT32)(rd_buf[cnt]);
		cnt++;
	}
	/* wait until tx is finished */
	while(!tx_is_finished());
}

int uart_case_pol_transmitting(void)
{
	int n=0;
	int len = SER_TEST_DATA_COUNT;
	int speed = 0;
	int tab_size = MARRAY_SIZE(serial_baud_rate_tab);

	put_case_info("start uart polling transmitting\n");
	while(1) {
		/* Get baudrate value */
		speed = serial_baud_rate_tab[n];

		/* Begin to data transmitting */
		//8 N 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//8 E 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//8 O 1
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//8 N 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);
		//8 E 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);
		//8 O 2
		config_frame_format(speed,
					HAL_UART_8_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);
		//7 N 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//7 E 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//7 O 1
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_1_STOP_BIT);
		subtest_pol(len);
		//7 N 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_NO_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);
		//7 E 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_EVEN_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);
		//7 O 2
		config_frame_format(speed,
					HAL_UART_7_DATA_BITS,
					HAL_UART_ODD_PARITY,
					HAL_UART_2_STOP_BITS);
		subtest_pol(len);

		/* Change baudrate table index */
		n++;
		if(n == tab_size)
			break;
	}
	put_case_info("finished polling transmitting\n");
	return 0;
}

static void set_irq_trigger_level(int rx_triggered,int value)
{
	unsigned int reg = hwpt_uart->triggers;

	if(rx_triggered) {
		reg &= ~(0x1F);//bit4~bit0
		reg |= UART_RX_TRIGGER(value);
	} else {
		reg &= ~(0xF00);//bit11~bit8
		reg |= UART_TX_TRIGGER(value);
	}
	hwpt_uart->triggers = reg;
}

int uart_case_test_rx_irq(int triggered_val)
{
	int cnt = 0,err;
	int temp;

	//waiting for tx finished
	while(!tx_is_finished());
	//reset uart
	reset_uart(g_uart_id);
	//init uart
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	// set rx triggered value
	set_irq_trigger_level(1,triggered_val);
	//set rx irq mask
	hwpt_uart->irq_mask |= UART_RX_DATA_AVAILABLE;
	d_printf("input more than %d data to trigger rx interrupt\n"
			,triggered_val);
	//waiting for irq flag
	while((hwpt_uart->irq_cause & 0x02) == 0);
	//check test result
	cnt = triggered_val + 1;
	temp = get_rx_fifo_level();
	if(temp == cnt) {
		err = 0;//the rx irq should occur when Rx_FIFO > Rx_Trigger
		d_printf("triggered rx irq at correct level %d\n",temp);
	} else {
		err = -1;
		d_printf("triggered rx irq at error level %d\n",temp);
	}
	while(!tx_is_finished());
	reset_uart(g_uart_id);
	return err;
}

int uart_case_test_tx_irq(int triggered_val)
{
	int cnt,err,a,b,c,d;
	int i;
	//waiting for tx finished
	while(!tx_is_finished());
	//reset uart
	reset_uart(g_uart_id);
	//init uart
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	//set tx triggered value
	set_irq_trigger_level(0,triggered_val);
	mdelay(5);
	a = hwpt_uart->triggers;
	b = get_tx_fifo_empty_space();

	if((hwpt_uart->irq_cause & 0x4)) {
		d_printf("tx irq occured without irq enabled,exit\n");
		return -1;
	}

	//enable tx irq
	hwpt_uart->irq_mask |= UART_TX_DATA_NEEDED;
	//write data to tx fifo
	for(i = 0;i < 15;i++)
		hwpt_uart->rxtx_buffer = 0x55;
	c = get_tx_fifo_empty_space();
	//waiting for irq flag
	while((hwpt_uart->irq_cause & 0x04) == 0);
	d = get_tx_fifo_empty_space();
	//disable tx irq
	hwpt_uart->irq_mask &= ~UART_TX_DATA_NEEDED;
	cnt = UART_TX_FIFO_SIZE - d;
	//check result
	if(cnt <= triggered_val ) {
		err = 0;//the tx irq should occur when TX_FIFO <= TX_Trigger
		d_printf("\ntriggered tx irq at correct level %d\n",cnt);
	} else {
		err = -1;
		d_printf("\ntriggered tx irq at error level %d,level should be %d\n",
				cnt,triggered_val);
	}
	d_printf("a = %x,b = %d,c = %d,d= %d\n",a,b,c,d);
	d_printf("wait tx is finished ...\n");
	while(!tx_is_finished());
	// wait irq cause flag change to zero from one
	mdelay(6);
	reset_uart(g_uart_id);
	return err;
}

int uart_case_test_rx_ovf_irq(void)
{
	int cnt,err;

	while(!tx_is_finished());
	reset_uart(g_uart_id);

	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);

	d_printf("input more than %d data to trigger rx overflow irq:\n",
			UART_RX_FIFO_SIZE);
	hwpt_uart->irq_mask |= UART_RX_LINE_ERR;
	while((hwpt_uart->irq_cause & 0x10) == 0);

	cnt = get_rx_fifo_level();
	d_printf("triggered rx ovf irq correctly and rx fifo level is %d\n",
			cnt);
	err = 0;
	while(!tx_is_finished());
	reset_uart(g_uart_id);
	return err;
}

int uart_case_test_rx_ovf_state(void)
{
	int cnt = 0,err;

	while(!tx_is_finished());
	reset_uart(g_uart_id);

	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	clear_error_state();

	d_printf("input more than %d data to rise up rx overflow state:\n",
			UART_RX_FIFO_SIZE);
	/* Waiting for Rx overflow error occurring */
	while((hwpt_uart->status & UART_RX_OVERFLOW_ERR) == 0);
	cnt = get_rx_fifo_level();
	clear_error_state();
	err = 0;
	d_printf("rise up rx ovf state correctly and rx fifo level is %d\n",
			cnt);
	while(!tx_is_finished());
	reset_uart(g_uart_id);
	return err;
}

static void enable_cts_rts(int enabled)
{
	hwpt_uart->ctrl &= (~UART_AUTO_FLOW_CONTROL);
	if(enabled)
		hwpt_uart->ctrl |= UART_AUTO_FLOW_CONTROL;
}

int uart_case_test_cts_rts(void)
{
	int cnt = 0;
	int len = 2048;

	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	//LAB_TEST_BEGIN:
	enable_cts_rts(1);
	//Check error flags
	clear_error_state();
	//Receive data
	cnt = 0;
	while(1) {
		if(cnt >= len)
			break;
		g_uart_buf[cnt] = serial_getc();
		cnt++;
	}
	//Delay n us for host ready
	mdelay(100);
	//Write data back
	cnt = 0;
	while(1) {
		if(cnt >= len)
			break;
		while(!(GET_BITFIELD(hwpt_uart->status,UART_TX_FIFO_SPACE)));
		hwpt_uart->rxtx_buffer = (UINT32)g_uart_buf[cnt];
		cnt++;
	}
	//Waitting for data sending end
	while(!tx_is_finished());
	//Disable CTS/RTS
	enable_cts_rts(0);
	return 0;
}

/*
 * when program runs in DRAM, the APB2 bus should not be reset
 * otherwise it will dead
 */
#if 0
static int reset_uart(int id)
{
	int ret = 0;
	if(id == UART_ID_1) {
		hwp_sysCtrlAp->APB2_Rst_Set = SYS_CTRL_AP_SET_APB2_RST_UART1;
		hwp_sysCtrlAp->APB2_Rst_Clr = SYS_CTRL_AP_CLR_APB2_RST_UART1;
	} else if(id == UART_ID_2) {
		hwp_sysCtrlAp->APB2_Rst_Set = SYS_CTRL_AP_SET_APB2_RST_UART2;
		hwp_sysCtrlAp->APB2_Rst_Clr = SYS_CTRL_AP_CLR_APB2_RST_UART2;
	} else if(id == UART_ID_3) {
		hwp_sysCtrlAp->APB2_Rst_Set = SYS_CTRL_AP_SET_APB2_RST_UART3;
		hwp_sysCtrlAp->APB2_Rst_Clr = SYS_CTRL_AP_CLR_APB2_RST_UART3;
	} else {
		ret = -1;
	}
	return ret;
}
#else
static int reset_uart(int id)
{
	/* reset the TX FIFO,RX FIFO */
	hwpt_uart->CMD_Set |= UART_RX_FIFO_RESET;
	hwpt_uart->CMD_Set |= UART_TX_FIFO_RESET;
	return 0;
}
#endif
int uart_case_test_default_value(void)
{
	unsigned int buf[10];
	int idx = 0;

	// reset uart
	while(!tx_is_finished());
	reset_uart(g_uart_id);

	buf[idx++] = hwpt_uart->ctrl;
	buf[idx++] = hwpt_uart->status;
	buf[idx++] = hwpt_uart->irq_mask;
	buf[idx++] = hwpt_uart->irq_cause;
	buf[idx++] = hwpt_uart->triggers;
	buf[idx++] = hwpt_uart->CMD_Set;
	buf[idx++] = hwpt_uart->CMD_Clr;
	//init uart
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	idx = 0;
	d_printf("\nread register default value ...\n");
	d_printf("ctrl:      %x\n",buf[idx++]);
	d_printf("status:    %x\n",buf[idx++]);
	d_printf("irq_mask:  %x\n",buf[idx++]);
	d_printf("irq_cause: %x\n",buf[idx++]);
	d_printf("triggers:  %x\n",buf[idx++]);
	d_printf("CMD_Set:   %x\n",buf[idx++]);
	d_printf("CMD_Clr:   %x\n",buf[idx++]);
	d_printf("\n");
	return 0;
}

int uart_simple_test_in(int port_id)
{
	char data,reply;
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	d_printf("\n\n Start UART easy test.\n");
	d_printf("\nThis is a simple loop-back test."
		"\nPlease input some string,Press q to exit\n");
	do {
		serial_puts("Check input: ");

		data = (char)serial_getc();

		if(data >= 'A' && data <='Z'){
			reply = data + 'a' - 'A';
		}else if(data >= 'a' && data <= 'z'){
			reply = data + 'A' - 'a';
		}else if(data >= '0' && data <= '9'){
			reply = '0' + ((data - '0' + 5) % 10);
		}else{
			reply = data;
		}
		serial_putc(reply);
		serial_puts("\n");
		if (data == 'q' || data== 'Q')
			break;
	}while(1);
	d_printf("\nFinish UART easy test.\n");
	return 0;
}

int uart_full_test_in(int port_id, int case_id)
{
	int ret = 0;

	/* Be ready */
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	d_printf("start to test uart, case id is %d\n",case_id);
	/* Begin tests */
	switch(case_id) {
	case UART_TC_ID_POL_TRANSMIT:
		ret = uart_case_pol_transmitting();
		break;
	case UART_TC_ID_DMA_TRANSMIT:
#ifdef XXXDMATEST //CONFIG_UART_DMA_TEST
		ret = uart_case_dma_transmitting();
#else
		ret = -1;
#endif
		break;
	case UART_TC_ID_RX_IRQ:
		ret = uart_case_test_rx_irq(0);//min value
		ret += uart_case_test_rx_irq(1);
		ret += uart_case_test_rx_irq(2);
		ret += uart_case_test_rx_irq(0x1F);//max value
		break;
	case UART_TC_ID_TX_IRQ:
		ret = uart_case_test_tx_irq(3);
		ret += uart_case_test_tx_irq(4);
		ret += uart_case_test_tx_irq(5);
		break;
	case UART_TC_ID_RX_OVF_IRQ:
		ret = uart_case_test_rx_ovf_irq();
		break;
	case UART_TC_ID_CTS_RTS:
		ret = uart_case_test_cts_rts();
		break;
	case UART_TC_ID_RX_OVF_STATE:
		ret = uart_case_test_rx_ovf_state();
		break;
	case UART_TC_ID_REG_DEFAULT_VALUE:
		ret = uart_case_test_default_value();
		break;
	case UART_TC_ID_TX_FIFO_DEPTH:
		ret = uart_case_test_tx_fifo_depth();
		break;
	case UART_TC_ID_RX_FIFO_LEVEL:
		ret = uart_case_test_rx_fifo_level();
		break;
	default:
		d_printf("unknown case id %d\n",case_id);
		ret =-1;
		break;
	}
	/* Print test result */
	serial_init();
	set_speed(UART_BAUD_RATE_DEFAULT);
	if(ret == 0)
		d_printf("UART test successful !!!\n");
	else
		d_printf("UART test failed ###\n");
	return ret;
}

/*
 * UART EASY TEST ENTRY
 *********************************************************
 */

int uart_test_entry(int uart_id)
{
	int ret = 0;

	/* Handle uart port */
	if(uart_id > 2) {
		d_printf("handle uart failed,uart id is %d\n",uart_id);
		return -1;
	}
	hwpt_uart = hwp_uart_tab[uart_id];
	if(hwpt_uart == NULL) {
		d_printf("uart port do not exist,uart_id is %d\n",uart_id);
		return -1;
	}
	d_printf("start to test uart %d\n ...",uart_id + 1);
	g_uart_id = uart_id;
	/* This is a simple case */
	ret = uart_simple_test_in(g_uart_id);
	/* These cases do not need a tester */
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_REG_DEFAULT_VALUE);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_RX_FIFO_LEVEL);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_TX_FIFO_DEPTH);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_TX_IRQ);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_RX_IRQ);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_RX_OVF_IRQ);
	ret += uart_full_test_in(g_uart_id,UART_TC_ID_RX_OVF_STATE);
	/* These cases need a tester for Tx/Rx data */
//	ret += uart_full_test_in(g_uart_id,UART_TC_ID_POL_TRANSMIT);
//	ret += uart_full_test_in(g_uart_id,UART_TC_ID_CTS_RTS);
	return ret;
}

int uart_test(int uart_id,int times)
{
	int ret = 0,n = 0;
	while(n < times) {
		n++;
		printf("\nuart_test: %d / %d\n",n,times);
		ret += uart_test_entry(uart_id);
		printf("\nuart_test end,ret is %d\n",ret);
	}
	return ret;
}
