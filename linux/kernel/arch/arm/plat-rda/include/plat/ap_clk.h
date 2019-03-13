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

#ifndef __AP_CLK_H__
#define __AP_CLK_H__

#define MHZ			(1000000)
#define PLL_CPU_FREQ		(_TGT_AP_PLL_CPU_FREQ * MHZ)

/*
 * TODO move this to platform specific header file
 */
#define AP_PLL_LOW_FREQ             (800)
#define PLL_CPU_FREQ_LOW        (AP_PLL_LOW_FREQ * MHZ)


#ifndef CONFIG_RDA_AP_PLL_FREQ_ADJUST
#define rda_ap_pll_current_freq PLL_CPU_FREQ
#else
extern unsigned long rda_ap_pll_current_freq;
int apsys_ap_pll_adjust(bool high_freq);
#endif

void apsys_notify_exception(void);

int apsys_request_lp2(unsigned long arg);
#ifdef CONFIG_RDA_SLEEP_OFF_MODE
int apsys_request_power_off(unsigned long arg);
void apsys_acknowledge_sleep_off(void);
#else
int apsys_request_sleep(unsigned long arg);
#endif

void apsys_enable_gouda_clk(int on);

void apsys_enable_camera_clk(int on);

void apsys_enable_dpi_clk(int on);

void apsys_enable_dsi_clk(int on);

void apsys_enable_gpu_clk(int on);

void apsys_enable_vpu_clk(int on);

void apsys_enable_voc_clk(int on);

void apsys_enable_spiflash_clk(int on);

void apsys_enable_uart1_clk(int on);

void apsys_enable_uart2_clk(int on);

void apsys_enable_uart3_clk(int on);

void apsys_enable_bck_clk(int on);

void apsys_enable_csi_clk(int on);

void apsys_enable_debug_clk(int on);

void apsys_enable_clk_out(int on);

void apsys_enable_aux_clk(int on);

void apsys_enable_clk_32k(int on);

void apsys_enable_usb_clk(int on);

void apsys_enable_bus_clk(int on);

unsigned long apsys_get_cpu_clk_rate(void);

unsigned long apsys_get_bus_clk_rate(void);

unsigned long apsys_get_axi_clk_rate(void);

unsigned long apsys_get_ahb1_clk_rate(void);

unsigned long apsys_get_apb1_clk_rate(void);

unsigned long apsys_get_apb2_clk_rate(void);

unsigned long apsys_get_gcg_clk_rate(void);

unsigned long apsys_get_gpu_clk_rate(void);

unsigned long apsys_get_vpu_clk_rate(void);

unsigned long apsys_get_voc_clk_rate(void);

unsigned long apsys_get_spiflash_clk_rate(void);

unsigned long apsys_get_uart_clk_rate(unsigned int id);

unsigned long apsys_get_bck_clk_rate(void);

unsigned long apsys_get_mem_clk_rate(void);

unsigned long apsys_get_usb_clk_rate(void);

void apsys_set_cpu_clk_rate(unsigned long rate);

void apsys_adjust_cpu_clk_rate(unsigned long rate);

void apsys_set_bus_clk_rate(unsigned long rate);

void apsys_adjust_bus_clk_rate(unsigned long rate);

void apsys_set_axi_clk_rate(unsigned long rate);

void apsys_set_ahb1_clk_rate(unsigned long rate);

void apsys_set_apb1_clk_rate(unsigned long rate);

void apsys_set_apb2_clk_rate(unsigned long rate);

void apsys_set_gcg_clk_rate(unsigned long rate);

void apsys_set_gpu_clk_rate(unsigned long rate);

void apsys_set_vpu_clk_rate(unsigned long rate);

void apsys_set_voc_clk_rate(unsigned long rate);

void apsys_set_spiflash_clk_rate(unsigned long rate);

void apsys_set_uart_clk_rate(unsigned int id, unsigned long rate);

void apsys_set_bck_clk_rate(unsigned long rate);

void apsys_set_mem_clk_rate(unsigned long rate);

void apsys_set_usb_clk_rate(unsigned long rate);

void apsys_set_dsi_clk_rate(unsigned long rate);

void apsys_reset_usbc(void);

void apsys_cpupll_switch(int on);

void apsys_reset_vpu(void);
void apsys_reset_axi_vpu(void);
void apsys_reset_axi_set_vpu(void);
void apsys_reset_axi_clr_vpu(void);

void apsys_reset_voc(void);
void apsys_reset_set_voc(void);
void apsys_reset_clr_voc(void);
unsigned int apsys_get_reset_set_voc(void);

void apsys_reset_gouda(void);
void apsys_reset_lcdc(void);

void apsys_reset_cpu(int core);

int apsys_request_for_vpu_bug(unsigned long is_request);
#endif /* __AP_CLK_H__ */
