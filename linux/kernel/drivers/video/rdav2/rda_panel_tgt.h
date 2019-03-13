#ifndef __RDA_PANEL_TGT_H
#define __RDA_PANEL_TGT_H
/*LCD panel driver init*/
#ifdef TGT_AP_PANEL_HIMAX_8379C_MIPI_PANEL
#include "rda_panel_himax8379c_mipi.c"
#endif
#ifdef TGT_AP_PANEL_ILI9488_MCU_PANEL
#include "rda_panel_ili9488_mcu.c"
#endif
#ifdef TGT_AP_PANEL_HX8357_MCU
#include "rda_panel_hx8357_mcu.c"
#endif
#ifdef TGT_AP_PANEL_KD070D10_RGB
#include "rda_panel_kd070d10.c"
#endif
#ifdef TGT_AP_PANEL_RM68180_MCU
#include "rda_panel_rm68180_mcu.c"
#endif
#ifdef TGT_AP_PANEL_ST7796_MCU
#include "rda_panel_st7796_mcu.c"
#endif
#ifdef TGT_AP_PANEL_OTM8019A_MIPI
#include "rda_panel_otm8019a_mipi.c"
#endif
#ifdef TGT_AP_PANEL_OTM9605A_MIPI
#include "rda_panel_otm9605a_mipi.c"
#endif
#ifdef TGT_AP_PANEL_RM68172_MIPI
#include "rda_panel_rm68172_mipi.c"
#endif
#ifdef TGT_AP_PANEL_HX379C_BOE397_MIPI
#include "rda_panel_hx8379c_boe397_mipi.c"
#endif
#ifdef TGT_AP_PANEL_OTM8019A_BOE397_MIPI
#include "rda_panel_otm8019a_boe397_mipi.c"
#endif
#ifdef TGT_AP_PAENL_ST7796S_BOE35_MIPI
#include "rda_panel_st7796s_boe35_mipi.c"
#endif
#ifdef TGT_AP_PANEL_HX8379C_CPT45_MIPI
#include "rda_panel_hx8379c_cpt45_mipi.c"
#endif
#ifdef TGT_AP_PANEL_ILI9881C_MIPI
#include "rda_panel_otm9605a_mipi.c"
#endif
#ifdef TGT_AP_PANEL_OTM8019A_CPT45_MIPI
#include "rda_panel_otm8019a_cpt45_mipi.c"
#endif
#ifdef TGT_AP_PANEL_JD9161BA_MIPI
#include "rda_panel_jd9161ba_mipi.c"
#endif
#ifdef TGT_AP_PANEL_ILI9806E_MIPI
#include "rda_panel_ili9806e_mipi.c"
#endif
static struct rda_panel_driver *panel_driver[] = {
#ifdef TGT_AP_PANEL_ILI9488_MCU_PANEL
	&ili9488_mcu_panel_driver,
#endif
#ifdef TGT_AP_PANEL_HIMAX_8379C_MIPI_PANEL
	&himax8379c_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_HX8357_MCU
	&hx8357_mcu_panel_driver,
#endif
#ifdef TGT_AP_PANEL_KD070D10_RGB
	&kd070d10_mcu_panel_driver,
#endif
#ifdef TGT_AP_PANEL_RM68180_MCU
	&rm68180_mcu_panel_driver,
#endif
#ifdef TGT_AP_PANEL_ST7796_MCU
	&st7796_mcu_panel_driver,
#endif
#ifdef TGT_AP_PANEL_OTM8019A_MIPI
	&otm8019a_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_OTM9605A_MIPI
	&otm9605a_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_RM68172_MIPI
	&rm68172_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_HX379C_BOE397_MIPI
	&hx8379c_boe397_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_OTM8019A_BOE397_MIPI
	&otm8019a_boe397_mipi_panel_driver,
#endif
#ifdef TGT_AP_PAENL_ST7796S_BOE35_MIPI
	&st7796s_boe35_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_HX8379C_CPT45_MIPI
	&hx8379c_cpt45_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_ILI9881C_MIPI
	&otm9605a_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_OTM8019A_CPT45_MIPI
	&otm8019a_cpt45_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_JD9161BA_MIPI
	&jd9161ba_mipi_panel_driver,
#endif
#ifdef TGT_AP_PANEL_ILI9806E_MIPI
	&ili9806e_mipi_panel_driver,
#endif
};
#endif
