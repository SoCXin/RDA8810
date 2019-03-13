#ifndef __ASM_ARCH_RDA_DEBUG_H
#define __ASM_ARCH_RDA_DEBUG_H
#include <linux/interrupt.h>

#define RDA_DEBUG
#define MACH_NAME "RDA"

extern unsigned int rda_debug;
extern int rda_bus_gating;
void rda_puts_no_irq(const char *fmt, ...);

#define RDA_DBG_ENTER		0x00000001
#define RDA_DBG_LEAVE		0x00000002
#define RDA_DBG_MACH		0x00000004
#define RDA_DBG_ION		0x00000008
#define RDA_DBG_MMC		0x00000010
#define RDA_DBG_GOUDA		0x00000020
#define RDA_DBG_I2C		0x00000040
#define RDA_DBG_TS		0x00000080
#define RDA_DBG_NAND		0x00000100
#define RDA_DBG_KEYPAD		0x00000200
#define RDA_DBG_CAMERA		0x00000400
#define RDA_DBG_GSENSOR		0x00000800
#define RDA_DBG_GPIO		0x00001000
#define RDA_DBG_CLK		0x00002000
#define RDA_DBG_PM		0x00004000
#define RDA_DBG_AUDIO   0x00008000
#define RDA_DBG_MDCOM	0x00010000
#define RDA_DBG_NET	0x00020000
#define RDA_DBG_GPS	0x00040000
#define RDA_DBG_LCDC		0x00080000

#ifdef RDA_DEBUG
#define RDA_DBG_LL(grp, grpnam, fmt, args...) \
	do { if (unlikely(rda_debug & (grp))) \
		printk(MACH_NAME grpnam "%s: " fmt, \
		in_interrupt() ? " (INT)" : "", ##args); \
	} while (0)
#else
#define RDA_DBG_LL(grp, grpnam, fmt, args...) do {} while (0)
#endif

#define rda_dbg_enter(grp) \
	RDA_DBG_LL(grp | RDA_DBG_ENTER, " enter", "%s()\n", __func__);
#define rda_dbg_enter_args(grp, fmt, args...) \
	RDA_DBG_LL(grp | RDA_DBG_ENTER, " enter", "%s(" fmt ")\n", \
	__func__, ##args);
#define rda_dbg_leave(grp) \
	RDA_DBG_LL(grp | RDA_DBG_LEAVE, " leave", "%s()\n", __func__);
#define rda_dbg_leave_args(grp, fmt, args...) \
	RDA_DBG_LL(grp | RDA_DBG_LEAVE, " leave", "%s(), " fmt "\n", \
	__func__, ##args);

#define rda_dbg_mach(fmt, args...)      RDA_DBG_LL(RDA_DBG_MACH, " mach", fmt, ##args)
#define rda_dbg_ion(fmt, args...)       RDA_DBG_LL(RDA_DBG_ION, " ion", fmt, ##args)
#define rda_dbg_mmc(fmt, args...)       RDA_DBG_LL(RDA_DBG_MMC, " mmc", fmt, ##args)
#define rda_dbg_gouda(fmt, args...)     RDA_DBG_LL(RDA_DBG_GOUDA, " gouda", fmt, ##args)
#define rda_dbg_lcdc(fmt, args...)      RDA_DBG_LL(RDA_DBG_LCDC, " lcdc", fmt, ##args)
#define rda_dbg_i2c(fmt, args...)       RDA_DBG_LL(RDA_DBG_I2C, " i2c", fmt, ##args)
#define rda_dbg_ts(fmt, args...)        RDA_DBG_LL(RDA_DBG_TS, " ts", fmt, ##args)
#define rda_dbg_nand(fmt, args...)      RDA_DBG_LL(RDA_DBG_NAND, " nand", fmt, ##args)
#define rda_dbg_keypad(fmt, args...)    RDA_DBG_LL(RDA_DBG_KEYPAD, " keypad", fmt, ##args)
#define rda_dbg_camera(fmt, args...)    RDA_DBG_LL(RDA_DBG_CAMERA, " camera", fmt, ##args)
#define rda_dbg_gsensor(fmt, args...)   RDA_DBG_LL(RDA_DBG_GSENSOR, " gsensor", fmt, ##args)
#define rda_dbg_gpio(fmt, args...)      RDA_DBG_LL(RDA_DBG_GPIO, " gpio", fmt, ##args)
#define rda_dbg_clk(fmt, args...)       RDA_DBG_LL(RDA_DBG_CLK, " clk", fmt, ##args)
#define rda_dbg_pm(fmt, args...)        RDA_DBG_LL(RDA_DBG_PM, " pm", fmt, ##args)
#define rda_dbg_audio(fmt, args...)     RDA_DBG_LL(RDA_DBG_AUDIO, " audio", fmt, ##args)
#define rda_dbg_mdcom(fmt, args...)     RDA_DBG_LL(RDA_DBG_MDCOM, " mdcom", fmt, ##args)
#define rda_dbg_net(fmt, args...)     RDA_DBG_LL(RDA_DBG_NET, " net", fmt, ##args)
#define rda_dbg_gps(fmt, args...)     RDA_DBG_LL(RDA_DBG_GPS, " gps", fmt, ##args)

void rda_dump_buf(char *data, size_t len);

int __init rda_debug_init_sysfs(void);
#endif /* __ASM_ARCH_RDA_DEBUG_H */
