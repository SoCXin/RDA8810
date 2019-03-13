/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This is file is based on
 * repository git.gitorious.org/u-boot-omap3/mainline.git,
 * branch omap3-dev-usb, file drivers/usb/host/omap3530_usb.c
 *
 * This is the unique part of its copyright :
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2009 Texas Instruments
 *
 * ------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include "rda.h"
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/rda_sys.h>
#include "musb_core.h"

#define MONITOR_REG	(MUSB_BASE + 0x80)
#define MONITOR_TRIG_REG	(MUSB_BASE + 0x84)
#define UDC_PHY_CLK_REG	(MUSB_BASE + 0x8c)

struct musb_config musb_cfg = {
	.regs		= (struct musb_regs *)MUSB_BASE,
	.timeout	= RDA_FPGA_USB_TIMEOUT,
	.musb_speed	= 0,
};

static void udc_soft_reset(void)
{
	serial_printf("reset musb otg core...\n");
	hwp_sysCtrlAp->AHB1_Rst_Set = SYS_CTRL_AP_SET_AHB1_RST_USBC;
	udelay(1000);
	hwp_sysCtrlAp->AHB1_Rst_Clr = SYS_CTRL_AP_CLR_AHB1_RST_USBC;
}
/*
static void set_udc_monitor(int testcase)
{
	u32 value;

	serial_printf("monitor trigger offset :%x \n", MONITOR_TRIG_REG);
	value = 0x100 | (testcase & 0xff);
	serial_printf("monitor trigger value :%x \n", value);
	writel(value, MONITOR_TRIG_REG);

	serial_printf("monitor offset :%x, result : %x\n", MONITOR_REG, readl(MONITOR_REG));
}
*/
void udc_power_on(void)
{
}

void udc_power_off(void)
{
}

/*
 udc maybe initialized by other module,always by ROM code
 */
int udc_is_initialized(void)
{
	/* some hacks for 8850E U02 USB issue, remove them in future. */
#if defined(CONFIG_MACH_RDA8850E)
	/* pdl1 always do hardware usb init */
#if	defined(CONFIG_SPL_BUILD) && \
	defined(CONFIG_RDA_PDL)
	return 0;
#endif
	/* u-boot calib/autocall/pdl2 mode, need to do hardware usb init */
#ifndef CONFIG_RDA_PDL
	return 0;
#endif
#endif

	return rda_bm_is_download();
}

static void udc_setup_pll(void)
{
	unsigned mask;
	unsigned locked;
	int cnt = 10; //timeout is in ms;

	hwp_sysCtrlAp->Cfg_Pll_Ctrl[3] =
	    SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE |
	    SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET |
	    SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW(6)
	    | SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH(30);

	mask = SYS_CTRL_AP_PLL_LOCKED_USB_MASK;

	locked = SYS_CTRL_AP_PLL_LOCKED_USB_LOCKED;

	while (((hwp_sysCtrlAp->Sel_Clock & mask) != locked) && cnt) {
		mdelay(1);
		cnt--;
	}
	if (cnt == 0)
		printf("ERROR, cannot lock usb pll\n");
}

static void musb_phy_init(void)
{
	udc_setup_pll();
	/*
	if (rda_metal_id_get() == 0) {
		writel(0xf001, UDC_PHY_CLK_REG);
	} else if (rda_metal_id_get() == 1) {
		writel(0x5900f000, UDC_PHY_CLK_REG);
	}
	*/
}

int musb_platform_init(void)
{
	udc_soft_reset();
	musb_phy_init();

	return 0;
}

void musb_platform_deinit(void)
{
	/* noop */
}

void print_reg(void)
{
	u8 power;
	u8 testmode;
	u8 devctl;
	musbr = musb_cfg.regs;
	power = readb(&musbr->power);
	testmode = readb(&musbr->testmode);
	devctl = readb(&musbr->devctl);
	printf("usb power: %x \n",power);
	printf("usb testmode: %x \n",testmode);
	printf("usb devctl: %x \n",devctl);
}

int do_usb_test(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	u8 devctl;
	u8 test;
	if (argc < 2 || argc > 3)
		return CMD_RET_USAGE;
	if (argc == 2) {
		if (strcmp(argv[1],"init") == 0) {
			musbr = musb_cfg.regs;
			musb_platform_init();
			musb_start();
			devctl = readb(&musbr->devctl);
			devctl |= MUSB_DEVCTL_SESSION;
			writeb(devctl,&musbr->devctl);
			printf("usb init finish\n");;
		} else if (strcmp(argv[1],"j") == 0) {
			test = readb(&musbr->testmode);
			test &= ~(MUSB_TEST_K | MUSB_TEST_J);
			test |= MUSB_TEST_J;
			writeb(test,&musbr->testmode);
			printf("usb test_j finish\n");
		} else if (strcmp(argv[1],"k") == 0) {
			test = readb(&musbr->testmode);
			test &= ~(MUSB_TEST_K | MUSB_TEST_J);
			test |= MUSB_TEST_K;
			writeb(test,&musbr->testmode);
			printf("usb test_k finish\n");
		} else if (strcmp(argv[1],"host") == 0) {
			writeb(MUSB_POWER_HSENAB,&musbr->power);
			test = MUSB_TEST_FORCE_HOST | MUSB_TEST_FORCE_HS;
			writeb(test,&musbr->testmode);
			printf("usb force host finish\n");
		} else {
			printf("error argument!\n");
			return CMD_RET_USAGE;
		}
#ifdef USB_TEST_DEBUG
		print_reg();
#endif
	}
	return 0;
}

U_BOOT_CMD(
	usb_test, 2, 1, do_usb_test,
	"rda usb slave test",
	"[init, j, k, host]\n"
	"	-init: 	USB INIT\n"
	"	-j: 	TEST_J\n"
	"	-k: 	TEST_k\n"
	"	-host:	FORCE_HOST\n"
);
