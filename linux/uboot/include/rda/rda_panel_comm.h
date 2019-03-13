#ifndef __RDA_PANEL_COMM_H
#define __RDA_PANEL_COMM_H

/* 
****************************************************************
 please do not change the format
 panle name should follow rule : mode_name
 mode can be: rgb/mcu/dsi
 name is the actual panel name
****************************************************************
*/
#define VGA_LCDD_DISP_X		480
#define VGA_LCDD_DISP_Y		640

#define QVGA_LCDD_DISP_X	240
#define QVGA_LCDD_DISP_Y	320

#define HVGA_LCDD_DISP_X	320
#define HVGA_LCDD_DISP_Y	480

#define SVGA_LCDD_DISP_X	600
#define	SVGA_LCDD_DSIP_Y	800

#define WVGA_LCDD_DISP_X	480
#define WVGA_LCDD_DISP_Y	800

#define WQVGA_LCDD_DISP_X	240
#define WQVGA_LCDD_DISP_Y	400

#define FWVGA_LCDD_DISP_X	480
#define FWVGA_LCDD_DISP_Y	854

#define FWQVGA_LCDD_DSIP_X	240
#define FWQVGA_LCDD_DSIP_Y	432

#define SWVGA_LCDD_DISP_X	600
#define SWVGA_LCDD_DISP_Y	1024

#define HD_LCDD_DISP_X		720
#define HD_LCDD_DISP_Y		1280

#define qHD_LCDD_DISP_X		540
#define qHD_LCDD_DISP_Y		960

#define QHD_LCDD_DISP_X		1440
#define QHD_LCDD_DISP_Y		2560

#define AUTO_LCDD_WVGA_DISP_X	480
#define AUTO_LCDD_WVGA_DISP_Y	800

/* definition for panel ili9486l */
#define ILI9486L_PANEL_NAME		"rgb_ili9486l"

/* definition for panel ili9488l */
#define ILI9488L_PANEL_NAME		"rgb_ili9488l"

/* definition for panel ili9488l */
#define ILI9488L_MCU_PANEL_NAME		"mcu_ili9488l"

/* definition for panel ili9488l */
#define ILI9488_MCU_PANEL_NAME		"mcu_ili9488"

/* definition for panel r61581b */
#define R61581B_MCU_PANEL_NAME		"mcu_r61581b"

/* definition for panel rm68140 */
#define RM68140_MCU_PANEL_NAME         "mcu_rm68140"

/* definition for panel ili9806c */
#define ILI9806C_PANEL_NAME		"rgb_ili9806c"

/* definition for panel ili9806h mcu */
#define ILI9806H_MCU_PANEL_NAME		"mcu_ili9806h"

#define ILI9806G_MCU_PANEL_NAME		"mcu_ili9806g"

/* definition for panel hx8664 */
#define HX8664_PANEL_NAME		"rgb_hx8664"

/* definition for panel hx8363*/
#define HX8363_MCU_PANEL_NAME		"mcu_hx8363"

/* definition for panel truly1p6365 */
#define TRYLYLP6365_PANEL_NAME		"rgb_truly1p6365"

/* definition for panel nt35510 */
#define NT35510_PANEL_NAME		"rgb_nt35510"
#define NT35510_MCU_PANEL_NAME		"mcu_nt35510"

/* definition for panel nt35510s */
#define NT35510S_MCU_PANEL_NAME		"mcu_nt35510s"

/* definition for panel nt35510 */
#define NT35310_PANEL_NAME		"rgb_nt35310"
#define NT35310_MCU_PANEL_NAME		"mcu_nt35310"

/* definition for panel hx8357 */
#define HX8357_MCU_PANEL_NAME		"mcu_hx8357"

/* definition for panel ili9327 */
#define ILI9327_PANEL_NAME		"mcu_ili9327"

/* definition for panel kd070d10 */
#define JB070SZ03A_PANEL_NAME		"rgb_jb070sz03a"

/* definition for panel kd070d10 */
#define KD070D10_PANEL_NAME		"rgb_kd070d10"

/* definition for panel t50bmpl10 */
#define T50BMPL10_PANEL_NAME		"rgb_t50bmpl10"

/* definition for panel wy070ml521cp18b */
#define WY070ML521CP18B_PANEL_NAME	"rgb_wy070ml521cp18b"

/* definition for panel wy070ml521cp21a */
#define WY070ML521CP21A_PANEL_NAME	"rgb_wy070ml521cp21a"

/* definition for panel r70 */
#define R70_PANEL_NAME			"rgb_r70"

/* definition for panel otm8019a use mcu interface */
#define OTM8019A_PANEL_NAME		"rgb_otm8019a"

/* definition for panel otm8019a use mipi interface */
#define OTM8019A_MIPI_PANEL_NAME	"mipi_otm8019a"
#define OTM8019A_BOE397_MIPI_PANEL_NAME	"mipi_otm8019a_boe397"
#define OTM8019A_CPT45_MIPI_PANEL_NAME  "mipi_otm8019a_cpt45"

/* definition for panel jd9161ba */
#define JD9161BA_PANEL_NAME              "rgb_jd9161ba"

/* definition for panel himax8379c */
#define HIMAX_8379C_MIPI_PANEL_NAME	"mipi_himax_8379c"
#define HX8379C_BOE397_MIPI_PANEL_NAME	"mipi_hx8379c_boe397"
#define HX8379C_CPT45_MIPI_PANEL_NAME	"mipi_hx8379c_cpt45"

/* definition for panel rm68180 */
#define RM68180_MCU_PANEL_NAME		"mcu_rm68180"

/* definition for panel st7796 */
#define ST7796_MCU_PANEL_NAME		"mcu_st7796"

/* definition for panel rm68172 */
#define RM68172_MIPI_PANEL_NAME		"mipi_rm68172"

/* definitoin for panel st7796s */
#define ST7796S_BOE35_MIPI_PANEL_NAME	"mipi_st7796s_boe35"

/* definitoin for panel jd9161ba mipi */
#define JD9161BA_MIPI_PANEL_NAME        "mipi_jd9161ba"

/* definition for panel sgt mipi */
#define ILI9806E_MIPI_PANEL_NAME	"mipi_ili9806e"

/*define auto detect lcd panel*/
#define AUTO_DET_LCD_PANEL_NAME		"auto"


#define DEFAULT_PANEL_LIST " "
#define AUTO_DETECT_SUPPORTED_PANEL_NUM -1
#define AUTO_DETECT_SUPPORTED_PANEL_LIST  DEFAULT_PANEL_LIST


#define RDA_PANEL_SUPPORT_LIST		\
	ILI9486L_PANEL_NAME		\
	ILI9488L_PANEL_NAME		\
	ILI9806C_PANEL_NAME		\
	ILI9488_MCU_PANEL_NAME		\
	RM68140_MCU_PANEL_NAME		\
	R61581B_MCU_PANEL_NAME		\
	HX8664_PANEL_NAME		\
	HX8363_MCU_PANEL_NAME		\
	HX8357_MCU_PANEL_NAME    	\
	TRYLYLP6365_PANEL_NAME		\
	NT35510_PANEL_NAME		\
	NT35510_MCU_PANEL_NAME		\
	NT35310_MCU_PANEL_NAME		\
	NT35510S_MCU_PANEL_NAME		\
	ILI9327_PANEL_NAME		\
	JB070SZ03A_PANEL_NAME		\
	KD070D10_PANEL_NAME		\
	T50BMPL10_PANEL_NAME		\
	WY070ML521CP18B_PANEL_NAME	\
	WY070ML521CP21A_PANEL_NAME	\
	ILI9806H_MCU_PANEL_NAME		\
	ILI9806G_MCU_PANEL_NAME		\
        R70_PANEL_NAME          	\
	OTM8019A_PANEL_NAME       	\
	JD9161BA_PANEL_NAME       	\
	OTM8019A_MIPI_PANEL_NAME	\
	RM68180_MCU_PANEL_NAME		\
        ST7796_MCU_PANEL_NAME		\
	RM68172_MIPI_PANEL_NAME		\
	HX8379C_BOE397_MIPI_PANEL_NAME  \
	OTM8019A_BOE397_MIPI_PANEL_NAME	\
	ST7796S_BOE35_MIPI_PANEL_NAME	\
	HX8379C_CPT45_MIPI_PANEL_NAME	\
	OTM8019A_CPT45_MIPI_PANEL_NAME	\
	JD9161BA_MIPI_PANEL_NAME	\
	ILI9806E_MIPI_PANEL_NAME		\
        AUTO_DET_LCD_PANEL_NAME

#define ASPACE(S)	S" "
#endif /* __TGT_AP_PANEL_SETTING_H */
