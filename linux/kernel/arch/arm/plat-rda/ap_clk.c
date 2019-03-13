 /*
  * Copyright (C) 2013 RDA Microelectronics Inc.
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
  */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <plat/reg_sysctrl.h>
#include <plat/reg_aif.h>
#include <plat/reg_comregs.h>
#include <plat/reg_ap_irq.h>
#include <plat/ap_clk.h>
#include <plat/rda_debug.h>
#include <plat/md_sys.h>
#include <plat/cpufreq.h>
#include <plat/ispi.h>
#include <linux/module.h>
#include <rda/tgt_ap_clock_config.h>
#include <linux/irqchip/arm-gic.h>

#define COMREGS_SLEEP_CTRL	(1<<3)
#define COMREGS_EXCEPTION_CTRL	(1<<4)

#define PLL_BUS_FREQ		(_TGT_AP_PLL_BUS_FREQ * MHZ)
#define PLL_MEM_FREQ		(_TGT_AP_PLL_MEM_FREQ * MHZ)
#define PLL_USB_FREQ		(_TGT_AP_PLL_USB_FREQ * MHZ)

enum {
	AP_CPU_CLK_IDX = 0,
	AP_BUS_CLK_IDX,
	AP_MEM_CLK_IDX,
	AP_USB_CLK_IDX,
};

typedef enum {
	PLL_REG_CPU_BASE = 0x00,
	PLL_REG_BUS_BASE = 0x20,
	PLL_REG_MEM_BASE = 0x60,
	PLL_REG_USB_BASE = 0x80,
} PLL_REG_BASE_INDEX_t;

typedef enum {
	PLL_REG_OFFSET_01H = 1,
	PLL_REG_OFFSET_02H = 2,
	PLL_REG_OFFSET_DIV = 3,
	PLL_REG_OFFSET_04H = 4,
	PLL_REG_OFFSET_MAJOR = 5,
	PLL_REG_OFFSET_MINOR = 6,
	PLL_REG_OFFSET_07H = 7,
} PLL_REG_OFFSET_INDEX_t;

static unsigned long pll_bus_freq = PLL_BUS_FREQ;

static HWP_AIF_T *hwp_apAif = NULL;
static HWP_SYS_CTRL_AP_T *hwp_apSysCtrl = NULL;
static HWP_COMREGS_T *hwp_apComregs = NULL;
static HWP_AP_IRQ_T *hwp_apIrq = NULL;

#ifndef CONFIG_RDA_FPGA
static struct msys_device *clk_msys;
#endif /* CONFIG_RDA_FPGA */

#ifndef CONFIG_RDA_CLK_SIMU
/* clock division value map */
const static u8 clk_div_map[] = {
	4*60,	/* 0 */
	4*60,	/* 1 */
	4*60,	/* 2 */
	4*60,	/* 3 */
	4*60,	/* 4 */
	4*60,	/* 5 */
	4*60,	/* 6 */
	4*60,	/* 7 */
	4*40,	/* 8 */
	4*30,	/* 9 */
	4*24,	/* 10 */
	4*20,	/* 11 */
	4*17,	/* 12 */
	4*15,	/* 13 */
	4*13,	/* 14 */
	4*12,	/* 15 */
	4*11,	/* 16 */
	4*10,	/* 17 */
	4*9,	/* 18 */
	4*8,	/* 19 */
	4*7,	/* 20 */
	4*13/2,	/* 21 */
	4*6,	/* 22 */
	4*11/2,	/* 23 */
	4*5,	/* 24 */
	4*9/2,	/* 25 */
	4*4,	/* 26 */
	4*7/2,	/* 27 */
	4*3,	/* 28 */
	4*5/2,	/* 29 */
	4*2,	/* 30 */
	4*1,	/* 31 */
};

struct low_freq_clk_param {
	u32 enable;
};

static u32 apsys_get_divreg(u32 basefreq, u32 reqfreq, u32 *pdiv2)
{
	int i;
	int index;
	u32 adiv;
	u32 ndiv;

	adiv = basefreq / (reqfreq >> 2);
	if (pdiv2) {
		/* try div2 mode first */
		ndiv = adiv >> 1;
	} else {
		ndiv = adiv;
	}

	for (i = ARRAY_SIZE(clk_div_map) - 1; i >= 1; i--)
		if (ndiv < ((clk_div_map[i] + clk_div_map[i-1]) >> 1))
			break;
	index = i;

	if (pdiv2) {
		if (adiv == (clk_div_map[index] << 1)) {
			/* div2 mode is OK */
			*pdiv2 = 1;
		} else {
			/* try div1 mode */
			for (i = ARRAY_SIZE(clk_div_map) - 1; i >= 1; i--)
				if (adiv < ((clk_div_map[i] + clk_div_map[i-1]) >> 1))
					break;
			/* compare the results between div1 and div2 */
			if (abs(adiv - (clk_div_map[index] << 1)) <=
					abs(adiv - clk_div_map[i])) {
				*pdiv2 = 1;
			} else {
				*pdiv2 = 0;
				index = i;
			}
		}
	}

	return index;
}

static u32 apsys_cal_freq_by_divreg(u32 basefreq, u32 reg, u32 div2)
{
	u32 newfreq;

	if (reg >= ARRAY_SIZE(clk_div_map)) {
		pr_warn("Invalid div reg: %u\n", reg);
		reg = ARRAY_SIZE(clk_div_map) - 1;
	}
	/* Assuming basefreq is smaller than 2^31 (2.147G Hz) */
	newfreq = (basefreq << (div2 ? 0 : 1)) / (clk_div_map[reg] >> 1);
	return newfreq;
}

static u32 calc_dsi_phy_pll(u32 rate)
{
	u64 tmp;
	u32 sft, sftacc= 32;
	u32 from = 26;
	u32 to = 0x2000000;
	u32 max = 0x50000000;

	tmp = ((u64)max * from) >> 32;
	while (tmp) {
		tmp >>=1;
		sftacc--;
	}

	for (sft = 32; sft > 0; sft--) {
		tmp = (u64) to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}

	return ((rate*tmp) >> sft);
}
#endif

void apsys_notify_exception(void)
{
	u32 reg;
	reg = COMREGS_IRQ0_SET(COMREGS_EXCEPTION_CTRL);
	iowrite32(reg, &hwp_apComregs->ItReg_Clr);
}
#ifndef CONFIG_RDA_FPGA
static int __apsys_request_low_power(unsigned long flag)
{
#if 0
	/*
	 * FOR OFF MODE testing
	 * this should be handled by modem
	 * when turn this ON, to wakeup, need to use coolwatcher
	 *	 write 1 to AP_SYS_CTRL->CPU_Rst_Clr (009000020)
	 */
	apsys_reset_cpu(0);
#else
	u32 reg;
	reg = ioread32(&hwp_apIrq->Cause);
	if (reg == 0) {
		unsigned long old_rate, min;
		int need_adjust = 0;
		/* Lock interrupts.
		 * Modem is responsible for unlocking.
		 */
		ioread32(&hwp_apIrq->SC);
		rda_md_plls_shutdown(flag);

		old_rate = apsys_get_cpu_clk_rate();
		min = rda_get_cpufreq_min();
		if (min != old_rate)
			need_adjust = 1;
		if (need_adjust)
			apsys_adjust_cpu_clk_rate(min);
		/* Clear previous sleep state */
		reg = COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Clr);
		/* Request to sleep */
		reg = COMREGS_IRQ0_SET(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Set);

		/* CPU WFI HERE */
		asm volatile("dsb \n wfi" : : : "memory");

		// Modem should have finished processing the request
#ifndef CONFIG_MACH_RDA8810E
		//something wrong with 8810E modem image
		BUG_ON((ioread32(&hwp_apComregs->ItReg_Clr) &
				COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL)) == 0);
#endif
		// Tell modem that AP knows it can start to run
		reg = COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Clr);
		if (need_adjust)
			apsys_adjust_cpu_clk_rate(old_rate);
	}
#endif
	return 0;
}


#ifdef CONFIG_MACH_RDA8810
int apsys_request_sleep(unsigned long arg)
{
	unsigned long flag;

	flag = AP_DDR_PLL | AP_CPU_PLL | AP_BUS_PLL;
	return __apsys_request_low_power(flag);
}

int apsys_request_lp2(unsigned long arg)
{
	unsigned long flag;

	flag = AP_DDR_PLL | AP_CPU_PLL;
	return __apsys_request_low_power(flag);
}
#else



#ifndef CONFIG_RDA_SLEEP_OFF_MODE
int apsys_request_sleep(unsigned long arg)
{
	unsigned long flag;

	flag = AP_CPU_PLL | AP_BUS_PLL;
	__apsys_request_low_power(flag);
	return 0;
}

#else
void apsys_acknowledge_sleep_off(void)
{
	u32 reg;

	// Tell modem that AP knows it can start to run
	reg = COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL);
	iowrite32(reg, &hwp_apComregs->ItReg_Clr);
	dsb();
}

void __apsys_request_power_off(unsigned long flag)
{
	u32 reg;
	reg = ioread32(&hwp_apIrq->Cause);
	if (reg == 0) {
		/* Lock interrupts.
		 * Modem is responsible for unlocking.
		 */
		ioread32(&hwp_apIrq->SC);
		/* when GIC is in use, this critial section is not working anymore
		 * (that should be a hardware bug)
		 * so disable gic cpu interface here
		 * */
#ifdef CONFIG_ARM_GIC
		gic_disable_gicc();
#endif
		rda_md_plls_shutdown(flag);

		/* Clear previous sleep state */
		reg = COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Clr);

		/* Request to sleep */
		reg = COMREGS_IRQ0_SET(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Set);

		/* CPU WFI HERE */
		asm volatile("dsb \n wfi" : : : "memory");

		/* immediately cancel the sleep request
		 * so that XCPU won't power off AP
		 * */
		reg = COMREGS_IRQ0_CLR(COMREGS_SLEEP_CTRL);
		iowrite32(reg, &hwp_apComregs->ItReg_Clr);
		dsb();

#ifdef CONFIG_ARM_GIC
		/* TODO should use restoring instead of just re-enable it?*/
		/* is this really needed ? this means that no possibility of
		 * exit from WFI
		 */
		gic_enable_gicc();
#endif
		rda_puts_no_irq("power off failed wake from wfi\n");
	}

	rda_puts_no_irq("power off failed due to irq\n");
}

int apsys_request_power_off(unsigned long arg)
{
	unsigned long flag;

	flag = AP_CPU_PLL | AP_BUS_PLL;
	__apsys_request_power_off(flag);
	return 0;
}


#endif
#endif /* CONFIG_MACH_RDA8810 */

#else
static int __apsys_request_low_power(unsigned long flag)
{
	return 0;
}

int apsys_request_sleep(unsigned long arg)
{
	return 0;
}

int apsys_request_lp2(unsigned long arg)
{
	return 0;
}
#endif
/*
 * CPU low power 2, in this state, cpu in WFI, and DDR shutdown;
 */



int apsys_request_for_vpu_bug(unsigned long is_request)
{
	unsigned long flag;

	/* modem will set the DDR clock to 200MHZ , clear the bit
	 * and wakeup AP
	 * */
	if (is_request)
		flag = AP_DDR_PLL_VPUBUG_SET;
	else
		flag = AP_DDR_PLL_VPUBUG_RESTORE;
	__apsys_request_low_power(flag);
	if (rda_md_plls_read() & flag)
		return 1;
	return 0;

}

#ifndef CONFIG_RDA_CLK_SIMU

void apsys_enable_bus_clk(int on)
{
	u32 val;

	if (!rda_bus_gating)
		return;

	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");

	iowrite32(AP_CTRL_PROTECT_UNLOCK, &hwp_apSysCtrl->REG_DBG);

	if (on) {
		val = ioread32(&hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_BUS_CLK_IDX]);
		val |= SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE;
		val |= SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_ENABLE;
		iowrite32(val, &hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_BUS_CLK_IDX]);

		val = ioread32(&hwp_apSysCtrl->Sel_Clock);
		val &= ~SYS_CTRL_AP_BUS_SEL_FAST_SLOW;
		val |= SYS_CTRL_AP_BUS_SEL_FAST_FAST;
		iowrite32(val, &hwp_apSysCtrl->Sel_Clock);

	} else {
		val = ioread32(&hwp_apSysCtrl->Sel_Clock);
		val |= SYS_CTRL_AP_BUS_SEL_FAST_SLOW;
		iowrite32(val, &hwp_apSysCtrl->Sel_Clock);

		val = ioread32(&hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_BUS_CLK_IDX]);
		val &= ~SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE;
		val &= ~SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_ENABLE;
		val |= SYS_CTRL_AP_AP_PLL_ENABLE_POWER_DOWN;
		val |= SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_DISABLE;
		iowrite32(val, &hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_BUS_CLK_IDX]);
	}
}

#ifdef CONFIG_MACH_RDA8810
void apsys_enable_usb_clk(int on)
{
}
#else
#include <plat/cpu.h>
void apsys_enable_usb_clk(int on)
{
	u32 val = 0;
	u32 mask;
	u32 locked;
	int cnt = 10;
	u16 metal_id = rda_get_soc_metal_id();
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");

	iowrite32(AP_CTRL_PROTECT_UNLOCK, &hwp_apSysCtrl->REG_DBG);

	if (on) {
		if (metal_id >= 3) {
			ispi_reg_write(0x82,0x020b);
		}
		val = ioread32(&hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_USB_CLK_IDX]);
		val &= SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW_MASK
			| SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH_MASK;
		val |= SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE
			| SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET;
		iowrite32(val, &hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_USB_CLK_IDX]);
		mask = SYS_CTRL_AP_PLL_LOCKED_USB_MASK;
		locked = SYS_CTRL_AP_PLL_LOCKED_USB_LOCKED;
		while (((hwp_sysCtrlAp->Sel_Clock & mask) != locked) && cnt) {
			mdelay(1);
			cnt--;
		}
		if (cnt == 0)
			pr_err("ERROR, cannot lock usb pll\n");
	} else {
		if (metal_id >= 3) {
			ispi_reg_write(0x82,0x020a);
		}
		val = ioread32(&hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_USB_CLK_IDX]);
		val &= SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW_MASK
			| SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH_MASK;
		val |= SYS_CTRL_AP_AP_PLL_ENABLE_POWER_DOWN
			| SYS_CTRL_AP_AP_PLL_LOCK_RESET_RESET
			| SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_DISABLE;
		iowrite32(val, &hwp_apSysCtrl->Cfg_Pll_Ctrl[AP_USB_CLK_IDX]);
	}
}
#endif

void apsys_enable_gouda_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on) {
		iowrite32(SYS_CTRL_AP_ENABLE_GCG_GOUDA,
				&hwp_apSysCtrl->Clk_GCG_Enable);
	} else {
		iowrite32(SYS_CTRL_AP_DISABLE_GCG_GOUDA,
				&hwp_apSysCtrl->Clk_GCG_Disable);
	}
}

void apsys_enable_dsi_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on) {
		ispi_reg_write(0xA2,0x20B);
		udelay(100);
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_DSI,
				&hwp_apSysCtrl->Clk_APO_Enable);
	} else {
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_DSI,
				&hwp_apSysCtrl->Clk_APO_Disable);
		ispi_reg_write(0xA2,0x20A);
		udelay(100);
	}
}

void apsys_enable_dpi_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on) {
		iowrite32(SYS_CTRL_AP_ENABLE_GCG_DPI,
				&hwp_apSysCtrl->Clk_GCG_Enable);
	} else {
		iowrite32(SYS_CTRL_AP_DISABLE_GCG_DPI,
				&hwp_apSysCtrl->Clk_GCG_Disable);
	}
}

void apsys_enable_camera_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_GCG_CAMERA,
				&hwp_apSysCtrl->Clk_GCG_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_GCG_CAMERA,
				&hwp_apSysCtrl->Clk_GCG_Disable);
}

void apsys_enable_gpu_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on) {
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_GPU,
				&hwp_apSysCtrl->Clk_APO_Enable);
#ifndef CONFIG_ARCH_RDA8810H
		iowrite32(SYS_CTRL_AP_ENABLE_MEM_GPU,
				&hwp_apSysCtrl->Clk_MEM_Enable);
#endif
	} else {
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_GPU,
				&hwp_apSysCtrl->Clk_APO_Disable);
#ifndef CONFIG_ARCH_RDA8810H
		iowrite32(SYS_CTRL_AP_DISABLE_MEM_GPU,
				&hwp_apSysCtrl->Clk_MEM_Disable);
#endif
	}
}

void apsys_enable_vpu_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on) {
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_VPU,
				&hwp_apSysCtrl->Clk_APO_Enable);
#ifndef CONFIG_ARCH_RDA8810H
		iowrite32(SYS_CTRL_AP_ENABLE_MEM_VPU,
				&hwp_apSysCtrl->Clk_MEM_Enable);
#endif
	} else {
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_VPU,
				&hwp_apSysCtrl->Clk_APO_Disable);
#ifndef CONFIG_ARCH_RDA8810H
		iowrite32(SYS_CTRL_AP_DISABLE_MEM_VPU,
				&hwp_apSysCtrl->Clk_MEM_Disable);
#endif
	}
}

void apsys_enable_voc_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_VOC
				| SYS_CTRL_AP_ENABLE_APOC_VOC_CORE
				| SYS_CTRL_AP_ENABLE_APOC_VOC_ALWAYS,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_VOC
				| SYS_CTRL_AP_DISABLE_APOC_VOC_CORE
				| SYS_CTRL_AP_DISABLE_APOC_VOC_ALWAYS,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_spiflash_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_SPIFLASH,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_SPIFLASH,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_uart1_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_UART1,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_UART1,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_uart2_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_UART2,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_UART2,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_uart3_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_UART3,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_UART3,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_bck_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_BCK,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_BCK,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_csi_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_CSI,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_CSI,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

void apsys_enable_debug_clk(int on)
{
	rda_dbg_clk("%s %s\n", __func__, on?"on":"off");
	if (on)
		iowrite32(SYS_CTRL_AP_ENABLE_APOC_PDGB,
				&hwp_apSysCtrl->Clk_APO_Enable);
	else
		iowrite32(SYS_CTRL_AP_DISABLE_APOC_PDGB,
				&hwp_apSysCtrl->Clk_APO_Disable);
}

#ifndef CONFIG_RDA_FPGA
void apsys_enable_clk_out(int on)
{
	struct client_cmd cmd_set;
	struct low_freq_clk_param clk_param;
	unsigned int ret = 0;

	rda_dbg_clk("%s %s\n", __func__, on ? "on" : "off");

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = clk_msys;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_CLK_OUT;

	clk_param.enable = on;

	cmd_set.pdata = (void *)&clk_param;
	cmd_set.data_size = sizeof(clk_param);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("Failed as setting clk_out\n");
	}

	return;
}

void apsys_enable_aux_clk(int on)
{
	struct client_cmd cmd_set;
	struct low_freq_clk_param clk_param;
	unsigned int ret = 0;

	rda_dbg_clk("%s %s\n", __func__, on ? "on" : "off");

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = clk_msys;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_AUX_CLK;

	clk_param.enable = on;

	cmd_set.pdata = (void *)&clk_param;
	cmd_set.data_size = sizeof(clk_param);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("Failed as setting aux_clk\n");
	}

	return;
}

void apsys_enable_clk_32k(int on)
{
	struct client_cmd cmd_set;
	struct low_freq_clk_param clk_param;
	unsigned int ret = 0;

	rda_dbg_clk("%s %s\n", __func__, on ? "on" : "off");

	memset(&cmd_set, 0, sizeof(cmd_set));

	cmd_set.pmsys_dev = clk_msys;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_CLK_32K;

	clk_param.enable = on;

	cmd_set.pdata = (void *)&clk_param;
	cmd_set.data_size = sizeof(clk_param);

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0) {
		pr_err("Failed as setting clk_32k\n");
	}

	return;
}
#else
void apsys_enable_clk_out(int on)
{
}

void apsys_enable_aux_clk(int on)
{
}

void apsys_enable_clk_32k(int on)
{
}
#endif

unsigned long apsys_get_cpu_clk_rate(void)
{
	register u32 reg;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_CPU);
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_CPU_FREQ);
	rate = apsys_cal_freq_by_divreg(rda_ap_pll_current_freq, reg, 0);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_bus_clk_rate(void)
{
	unsigned long rate = pll_bus_freq;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_axi_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_AXI);
	div2 = reg & SYS_CTRL_AP_AP_AXI_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_AXI_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_ahb1_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_AHB1);
	div2 = reg & SYS_CTRL_AP_AP_AHB1_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_AHB1_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_apb1_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_APB1);
	div2 = reg & SYS_CTRL_AP_AP_APB1_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_APB1_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_apb2_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_APB2);
	div2 = reg & SYS_CTRL_AP_AP_APB2_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_APB2_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_gcg_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_GCG);
	div2 = reg & SYS_CTRL_AP_AP_GCG_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_GCG_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_gpu_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_GPU);
	div2 = reg & SYS_CTRL_AP_AP_GPU_SRC_DIV2;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_GPU_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_vpu_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_VPU);
	div2 = reg & SYS_CTRL_AP_AP_VPU_SRC_DIV2;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_VPU_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_voc_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_VOC);
	div2 = reg & SYS_CTRL_AP_AP_VOC_SRC_DIV2;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_VOC_FREQ);
	rate = apsys_cal_freq_by_divreg(pll_bus_freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_spiflash_clk_rate(void)
{
	register u32 reg;
	u32 div2;
	unsigned long rate;
	u32 ahb1freq = apsys_get_ahb1_clk_rate();

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_SFLSH);
	div2 = reg & SYS_CTRL_AP_AP_SFLSH_SRC_DIV2;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_SFLSH_FREQ);
	rate = apsys_cal_freq_by_divreg(ahb1freq, reg, div2);
	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_uart_clk_rate(unsigned int id)
{
	register u32 reg;
	u32 clksrc;
	u32 divmode;
	u32 div;
	unsigned long rate;

	if (id > 2) {
		pr_warn("Invalid uart ID: %u\n", id);
		return 0;
	}

	reg = ioread32(&hwp_apSysCtrl->Cfg_Clk_Uart[id]);
	if (reg & SYS_CTRL_AP_UART_SEL_PLL_PLL)
		clksrc = pll_bus_freq / 8;
	else
		clksrc = 26000000;
	div = GET_BITFIELD(reg, SYS_CTRL_AP_UART_DIVIDER);
	if (0 /*ctrl & UART_DIVISOR_MODE*/)
		divmode = 16;
	else
		divmode = 4;
	rate = clksrc / divmode / (div + 2);

	rda_dbg_clk("%s %d %ld\n", __func__, id, rate);
	return rate;
}

unsigned long apsys_get_bck_clk_rate(void)
{
	register u32 reg;
	unsigned long rate;
	u32 div;

	reg = ioread32(&hwp_apAif->Cfg_Clk_AudioBCK);
	if ((reg & AIF_BCK_PLL_SOURCE) == AIF_BCK_PLL_SOURCE_PLL_150M) {
		div = GET_BITFIELD(reg, AIF_AUDIOBCK_DIVIDER);
		rate = pll_bus_freq / 8 / (div + 2);
	} else {
		/* Codec PLL is used and we do not know the PLL freq */
		rate = 0;
	}

	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_mem_clk_rate(void)
{
	unsigned long rate;
	u32 div2;

	/* Ddr phy will process data at clock rising edge only,
	 * other than at both edges.
	 * So the nominal ddr freq is one half of the actual ddr freq.
	 * Dividers:
	 * nominal_issue=2, ddr_phy_div=2, digital_logic_div=2 or 1
	 */
	div2= (ioread32(&hwp_apSysCtrl->Cfg_Clk_AP_MEM) &
			SYS_CTRL_AP_AP_MEM_SRC_DIV2) ? 1 : 0;
	rate = PLL_MEM_FREQ >> (2 + div2);

	rda_dbg_clk("%s %ld\n", __func__, rate);
	return rate;
}

unsigned long apsys_get_usb_clk_rate(void)
{
	return PLL_USB_FREQ;
}

void apsys_set_cpu_clk_rate(unsigned long rate)
{
	register u32 reg = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_CPU_FREQ(
			apsys_get_divreg(rda_ap_pll_current_freq, rate, NULL));
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_CPU);
}

void apsys_adjust_cpu_clk_rate(unsigned long rate)
{
	unsigned long ladder_rate;

	ladder_rate = rda_get_cpufreq_ladder();
	if (rate <= ladder_rate ) {
		apsys_set_cpu_clk_rate(rate);
	} else {
		apsys_set_cpu_clk_rate(ladder_rate);
		udelay(10);
		apsys_set_cpu_clk_rate(rate);
		udelay(10);
	}
}

void apsys_set_bus_clk_rate(unsigned long rate)
{
#if 0
	u32 val = 0;
	u32 major, minor;
	u16 value_02h = 0x020B;
	u32 freq_m = rate / MHZ;

	if(freq_m > 1000)
		return;

	/* calculate major & minor by freq, it's not very exact  */
	val = (2 << 20) * freq_m / 26;
	while(!(val >> 28))
		val = val << 4;
	major = val >> 16;
	minor = val & 0xffff;

	ispi_reg_write(PLL_REG_BUS_BASE + PLL_REG_OFFSET_02H, value_02h);
	ispi_reg_write(PLL_REG_BUS_BASE + PLL_REG_OFFSET_MAJOR, major);
	ispi_reg_write(PLL_REG_BUS_BASE + PLL_REG_OFFSET_MINOR, minor);
	ispi_reg_write(PLL_REG_BUS_BASE + PLL_REG_OFFSET_07H, 0x0012);
	mdelay(1);
	ispi_reg_write(PLL_REG_BUS_BASE + PLL_REG_OFFSET_07H, 0x0013);

	rda_dbg_clk("%s %ld\n", __func__, rate);
	pll_bus_freq = rate;
#endif
}

void apsys_adjust_bus_clk_rate(unsigned long rate)
{
	apsys_set_cpu_clk_rate(rate);
}

void apsys_set_axi_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_AXI_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		   reg |= SYS_CTRL_AP_AP_AXI_SRC_SEL;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_AXI);
}

void apsys_set_ahb1_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_AHB1_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_AHB1_SRC_SEL;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_AHB1);
}

void apsys_set_apb1_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_APB1_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_APB1_SRC_SEL;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_APB1);
}

void apsys_set_apb2_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_APB2_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_APB2_SRC_SEL;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_APB2);
}

void apsys_set_gcg_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_GCG_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_GCG_SRC_SEL;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_GCG);
}

void apsys_set_gpu_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_GPU_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_GPU_SRC_DIV2;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_GPU);
}

void apsys_set_vpu_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_VPU_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_VPU_SRC_DIV2;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_VPU);
}

void apsys_set_voc_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_VOC_FREQ(
			apsys_get_divreg(pll_bus_freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_VOC_SRC_DIV2;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_VOC);
}

void apsys_set_spiflash_clk_rate(unsigned long rate)
{
	register u32 reg = 0;
	u32 div2 = 0;
	u32 ahb1freq = apsys_get_ahb1_clk_rate();

	rda_dbg_clk("%s %ld\n", __func__, rate);
	reg = SYS_CTRL_AP_AP_SFLSH_FREQ(
			apsys_get_divreg(ahb1freq, rate, &div2));
	if (div2)
		reg |= SYS_CTRL_AP_AP_SFLSH_SRC_DIV2;
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_SFLSH);
}

void apsys_set_uart_clk_rate(unsigned int id, unsigned long rate)
{
	u32 clksrc;
	u32 divmode;
	u32 div;

	rda_dbg_clk("%s %d %ld\n", __func__, id, rate);
	if (id > 2) {
		pr_warn("Invalid uart ID: %u\n", id);
		return;
	}

	if (rate > 3250000 /*26000000/4/(0+2)*/) {
#if 1
		rate = 3250000;
		divmode = 4;
#else
		pr_warn("Uart rate is too high: %u", (u32)rate);
		return;
#endif
	}

	clksrc = SYS_CTRL_AP_UART_SEL_PLL_SLOW;
	if (rate < 6342 /*26000000/4/(0x3FF+2)*/) {
		pr_warn("Uart rate is too low: %u", (u32)rate);
#if 1
		rate = 6342;
		divmode = 4;
#else
		/* To support these low rates,
		 * divmode = 16; and then set UART_DIVISOR_MODE
		 * in the ctrl register
		 */
		return;
#endif
	} else {
		divmode = 4;
	}

	div = (26000000 + divmode / 2 * rate) / (divmode * rate) - 2;

	iowrite32(clksrc | SYS_CTRL_AP_UART_DIVIDER(div),
			&hwp_apSysCtrl->Cfg_Clk_Uart[id]);
}

void apsys_set_bck_clk_rate(unsigned long rate)
{
	u32 div = pll_bus_freq / 8 / rate - 2;
	u32 reg = AIF_BCK_PLL_SOURCE_PLL_150M
		| AIF_BCK_POL_NORMAL | AIF_AUDIOBCK_DIVIDER(div);

	rda_dbg_clk("%s %ld\n", __func__, rate);
	iowrite32(reg, &hwp_apAif->Cfg_Clk_AudioBCK);
}

void apsys_set_mem_clk_rate(unsigned long rate)
{
	/* Ddr phy will process data at clock rising edge only,
	 * other than at both edges.
	 * So the nominal ddr freq is one half of the actual ddr freq.
	 * Dividers:
	 * nominal_issue=2, ddr_phy_div=2, digital_logic_div=2 or 1
	 */
	u32 thresh = ((PLL_MEM_FREQ >> 3) + (PLL_MEM_FREQ >> 2)) >> 1;
	u32 reg = (rate < thresh) ?  SYS_CTRL_AP_AP_MEM_SRC_DIV2 : 0;

	rda_dbg_clk("%s %ld\n", __func__, rate);
	iowrite32(reg, &hwp_apSysCtrl->Cfg_Clk_AP_MEM);
}

void apsys_set_usb_clk_rate(unsigned long rate)
{
}

void apsys_set_dsi_clk_rate(unsigned long rate)
{
	u32 dsi_phy,flag;
	unsigned long dsi_rate;
	printk("%s %ld\n", __func__, rate);

	flag = rate < 300000000 ? 1 : 0;

	dsi_rate = flag ? rate * 2 : rate;
	dsi_rate = dsi_rate / 1000000;
	dsi_phy = calc_dsi_phy_pll(dsi_rate);

	ispi_reg_write(0xA5,(dsi_phy >> 16) & 0xFFFF);
	ispi_reg_write(0xA6,dsi_phy & 0xFFFF);
	ispi_reg_write(0xA7,0x10);
	ispi_reg_write(0xA3,flag ? 0xBC00 : 0xEC00);
	ispi_reg_write(0xA2,0x20A);
	udelay(100);
	ispi_reg_write(0xA2,0x20B);
	udelay(100);
}
#endif /* !CONFIG_RDA_CLK_SIMU */

void apsys_reset_usbc(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->AHB1_Rst_Set = SYS_CTRL_AP_SET_AHB1_RST_USBC;
	udelay(1000);
	hwp_apSysCtrl->AHB1_Rst_Clr = SYS_CTRL_AP_CLR_AHB1_RST_USBC;
}

void apsys_reset_vpu(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->AXI_Rst_Set = SYS_CTRL_AP_AXI_RST_CLR_VPU;
	mdelay(1);
	hwp_apSysCtrl->AXI_Rst_Clr = SYS_CTRL_AP_AXI_RST_CLR_VPU;

	mdelay(1);
	hwp_apSysCtrl->AXIDIV2_Rst_Set = SYS_CTRL_AP_AXIDIV2_RST_CLR_VPU;
	mdelay(1);
	hwp_apSysCtrl->AXIDIV2_Rst_Clr = SYS_CTRL_AP_AXIDIV2_RST_CLR_VPU;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_vpu);

void apsys_reset_axi_vpu(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->AXI_Rst_Set = SYS_CTRL_AP_AXI_RST_CLR_VPU;
	mdelay(1);
	hwp_apSysCtrl->AXI_Rst_Clr = SYS_CTRL_AP_AXI_RST_CLR_VPU;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_axi_vpu);

void apsys_reset_axi_set_vpu(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->AXI_Rst_Set = SYS_CTRL_AP_AXI_RST_CLR_VPU;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_axi_set_vpu);

void apsys_reset_axi_clr_vpu(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->AXI_Rst_Clr = SYS_CTRL_AP_AXI_RST_CLR_VPU;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_axi_clr_vpu);

void apsys_reset_voc(void)
{
	rda_dbg_clk("%s\n", __func__);
		hwp_apSysCtrl->AXI_Rst_Set = SYS_CTRL_AP_SET_AXI_RST_VOC;
	mdelay(1);
		hwp_apSysCtrl->AXI_Rst_Clr = SYS_CTRL_AP_CLR_AXI_RST_VOC;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_voc);

unsigned int apsys_get_reset_set_voc(void)
{
	register u32 reg;
	rda_dbg_clk("%s\n", __func__);
	reg = ioread32(&hwp_apSysCtrl->AXI_Rst_Set);
	mdelay(1);
	return (unsigned int)(reg & SYS_CTRL_AP_SET_AXI_RST_VOC);
}

EXPORT_SYMBOL(apsys_get_reset_set_voc);

void apsys_reset_set_voc(void)
{
	rda_dbg_clk("%s\n", __func__);
		hwp_apSysCtrl->AXI_Rst_Set = SYS_CTRL_AP_SET_AXI_RST_VOC;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_set_voc);

void apsys_reset_clr_voc(void)
{
	rda_dbg_clk("%s\n", __func__);
		hwp_apSysCtrl->AXI_Rst_Clr = SYS_CTRL_AP_CLR_AXI_RST_VOC;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_clr_voc);

void apsys_reset_gouda(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->GCG_Rst_Set= SYS_CTRL_AP_GCG_RST_CLR_GOUDA;
	mdelay(1);
	hwp_apSysCtrl->GCG_Rst_Clr= SYS_CTRL_AP_GCG_RST_CLR_GOUDA;
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_gouda);

void apsys_reset_lcdc(void)
{
	rda_dbg_clk("%s\n", __func__);
	hwp_apSysCtrl->GCG_Rst_Set= SYS_CTRL_AP_GCG_RST_CLR_LCDC;
	udelay(1);
	hwp_apSysCtrl->GCG_Rst_Clr= SYS_CTRL_AP_GCG_RST_CLR_LCDC;
	udelay(1);
}
EXPORT_SYMBOL(apsys_reset_lcdc);

void apsys_reset_cpu(int core)
{
	hwp_apSysCtrl = ((HWP_SYS_CTRL_AP_T*)IO_ADDRESS(RDA_SYSCTRL_BASE));

	rda_dbg_clk("%s, core %d\n", __func__, core);
	hwp_apSysCtrl->CPU_Rst_Set= (1 << core);
	mdelay(1);
	hwp_apSysCtrl->CPU_Rst_Clr= (1 << core);
	mdelay(1);
}

EXPORT_SYMBOL(apsys_reset_cpu);


#ifdef CONFIG_RDA_AP_PLL_FREQ_ADJUST
unsigned long rda_ap_pll_current_freq = PLL_CPU_FREQ;

struct pll_freq {
	u32 freq_mhz;
	u16 major;
	u16 minor;
	u16 with_div;
	u16 div;
};


#ifdef CONFIG_ARCH_RDA8850E
#include <plat/cpu.h>

#define PLL_FREQ_TABLE_COUNT        21

static const struct pll_freq pll_freq_table[] = {
	/* MHz Major   Minor   div */
	{1600, 0x7B13, 0xB138, 0, 0x0000},
	{1200, 0x5C4E, 0xC4EC, 0, 0x0000},
	{1150, 0x5876, 0x2762, 0, 0x0000},
	{1100, 0x549D, 0x89D8, 0, 0x0000},
	{1050, 0x50C4, 0xEC4E, 0, 0x0000},
	{1020, 0x4E76, 0x2762, 0, 0x0000},
	{1010, 0x4DB1, 0x3B13, 0, 0x0000},
	{1000, 0x4CEC, 0x4EC4, 0, 0x0000},
	{ 988, 0x4C00, 0x0000, 0, 0x0000},
	{ 962, 0x4A00, 0x0000, 0, 0x0000},
	{ 936, 0x4800, 0x0000, 0, 0x0000},
	{ 910, 0x4600, 0x0000, 0, 0x0000},
	{ 884, 0x4400, 0x0000, 0, 0x0000},
	{ 858, 0x4200, 0x0000, 0, 0x0000},
	{ 832, 0x4000, 0x0000, 0, 0x0000},
	{ 806, 0x3E00, 0x0000, 0, 0x0000},
	{ 800, 0x3D89, 0xD89C, 0, 0x0000},
	{ 780, 0x3C00, 0x0000, 0, 0x0000},
	{ 750, 0x39B1, 0x3B13, 0, 0x0000},
	{ 600, 0x2E27, 0x6274, 0, 0x0000},
	{ 520, 0x2800, 0x0000, 0, 0x0000},
	{ 519, 0x27EC, 0x4EC4, 1, 0x0007},
	{ 500, 0x2676, 0x2762, 1, 0x0007},
	{ 480, 0x24EC, 0x4EC4, 0, 0x0000},
	{ 455, 0x2300, 0x0000, 1, 0x0007},
	{ 416, 0x2000, 0x0000, 1, 0x0007},
	{ 400, 0x1EC4, 0xEC4C, 1, 0x0007},
};

static int pll_freq_set(u32 reg_base, u32 freq_mhz)
{
	int i;
	const struct pll_freq *freq;
	unsigned int major, minor;
	unsigned short value_02h;

	/* find pll_freq */
	for (i = 0; i < ARRAY_SIZE(pll_freq_table); i++) {
		if (pll_freq_table[i].freq_mhz == freq_mhz)
			break;
	}
	if (i >= ARRAY_SIZE(pll_freq_table)) {
		return -1;
	}

	freq = &pll_freq_table[i];
	if (freq->with_div && (reg_base == PLL_REG_MEM_BASE)) {
		if (freq_mhz >= 200)
			ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, freq->div & 0x7fff);
		mdelay(5);
		ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV,
				freq->div);
		// Calculate the real MEM PLL freq
		freq_mhz *= (1 << (8 - (freq->div & 0x7)));
	}
	if (freq_mhz >= 800 || reg_base == PLL_REG_USB_BASE) {
		value_02h = 0x030B;
		// Div PLL freq by 2
		minor = ((freq->major & 0xFFFF) << 14) |
			((freq->minor >> 2) & 0x3FFF);
		minor >>= 1;
		// Recalculate the divider
		major = (minor >> 14) & 0xFFFF;
		minor = (minor << 2) & 0xFFFF;
	} else {
		value_02h = 0x020B;
		major = freq->major;
		minor = freq->minor;
	}

	if(reg_base == PLL_REG_USB_BASE)
		ispi_reg_write(reg_base + 0x9, 0x7100);
	if (reg_base == PLL_REG_USB_BASE)
		value_02h ^= (1 << 8);
#ifndef _TGT_AP_CPU_PLL_FREQ_AUTO_REGULATE
	if(reg_base == PLL_REG_CPU_BASE) {
		/* Disable pll freq divided by 2 when VCore voltage is lower than a threshold value. */
		if(rda_get_soc_metal_id() > 1)
			ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, 0x72A2);
	}
#endif
	ispi_reg_write(reg_base + PLL_REG_OFFSET_02H, value_02h);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MAJOR, major);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MINOR, minor);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0012);
	mdelay(1);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0013);
	return 0;
}


#else
#error "not supported in platform other than 8850E"
#endif

/*
 * TODO - how to do this in SMP?
 * TODO - too long delay inside pll_freq_set?
 */
int apsys_ap_pll_adjust(bool high_freq)
{
	unsigned long flags;


	if ((high_freq == 0) && (rda_ap_pll_current_freq == AP_PLL_LOW_FREQ))
		goto exit;
	if ((high_freq) && (rda_ap_pll_current_freq == PLL_CPU_FREQ))
		goto exit;

	local_irq_save(flags);

	apsys_cpupll_switch(0);

	if (high_freq) {
		rda_ap_pll_current_freq = PLL_CPU_FREQ;
		pll_freq_set(PLL_REG_CPU_BASE, _TGT_AP_PLL_CPU_FREQ);
	} else {
		rda_ap_pll_current_freq = PLL_CPU_FREQ_LOW;
		pll_freq_set(PLL_REG_CPU_BASE, AP_PLL_LOW_FREQ);
	}
	apsys_cpupll_switch(1);

	local_irq_restore(flags);
exit:
	return 0;
}
#endif

void apsys_cpupll_switch(int on)
{
	volatile u32 *clock_reg = &hwp_apSysCtrl->Sel_Clock;
	unsigned long flags;

	if (on) {
		u32 clk_value = readl(clock_reg);

		local_irq_save(flags);

		clk_value &= ~(1 << 4);
		writel(clk_value, clock_reg);
		local_irq_restore(flags);
	} else {
		u32 clk_value = readl(clock_reg);
		local_irq_save(flags);
		clk_value |= SYS_CTRL_AP_CPU_SEL_FAST_SLOW;
		writel(clk_value, clock_reg);
		local_irq_restore(flags);
	}
}

int __init rda_apsys_init(void)
{
	hwp_apSysCtrl = ((HWP_SYS_CTRL_AP_T*)IO_ADDRESS(RDA_SYSCTRL_BASE));
	hwp_apAif = ((HWP_AIF_T*)IO_ADDRESS(RDA_AIF_BASE));
	hwp_apComregs = ((HWP_COMREGS_T*)IO_ADDRESS(RDA_COMREGS_BASE));
	hwp_apIrq = ((HWP_AP_IRQ_T*)IO_ADDRESS(RDA_INTC_BASE));

	return 0;
}

#ifndef CONFIG_RDA_FPGA
/* This function should be invoked after mdsys had been initialized. */
static int __init rda_apsys_late_init(void)
{
	clk_msys = rda_msys_alloc_device();
	if (!clk_msys) {
		pr_err("Invalid pointer of mdsys\n");
		return -EINVAL;
	}

	clk_msys->module = SYS_GEN_MOD;
	clk_msys->name = "rda-apsys";
	return rda_msys_register_device(clk_msys);
}
subsys_initcall(rda_apsys_late_init);
#endif
