#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_panel.h"
#include "rda_lcdc.h"
#include "rda_mipi_dsi.h"

static struct rda_lcd_info otm8019a_cpt45_mipi_info;

/*mipi dsi phy 260MHz*/
static const struct rda_dsi_phy_ctrl otm8019a_cpt45_pll_phy_260mhz = {
	{
                {0x6C, 0x3F}, {0x10C, 0x3}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
                {0x120, 0xC}, {0x124, 0x2}, {0x128, 0x3}, {0x80, 0xE}, {0x84, 0xC},
                {0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
        },
        {
                {0x64, 0x3}, {0x134, 0x3}, {0x138, 0xB}, {0x14C, 0x2C}, {0x13C, 0x7},
                {0x114, 0xB}, {0x170, 0x87A}, {0x140, 0xFF},
        },
};

static const char otm8019a_cpt45_cmd0[]  = 	{0x00,0x00};
static const char otm8019a_cpt45_cmd1[]  =	{0xFF,0x80,0x19,0x01};
static const char otm8019a_cpt45_cmd2[]  =	{0x00,0x80};
static const char otm8019a_cpt45_cmd3[]  =	{0xFF,0x80,0x19};
static const char otm8019a_cpt45_cmd4[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd5[]  =	{0xD8,0x6F,0x6F};
static const char otm8019a_cpt45_cmd6[]  =	{0x00,0x82};
static const char otm8019a_cpt45_cmd7[]  =	{0xC5,0xB0};
static const char otm8019a_cpt45_cmd8[]  =	{0x00,0xA1};
static const char otm8019a_cpt45_cmd9[]  =	{0xC1,0x08};
static const char otm8019a_cpt45_cmd10[]  =	{0x00,0xA3};
static const char otm8019a_cpt45_cmd11[]  =	{0xC0,0x1B};
static const char otm8019a_cpt45_cmd12[]  =	{0x00,0xB4};
static const char otm8019a_cpt45_cmd13[]  =	{0xC0,0x00};
static const char otm8019a_cpt45_cmd14[]  =	{0x00,0x81};
static const char otm8019a_cpt45_cmd15[]  =	{0xC4,0x83};
static const char otm8019a_cpt45_cmd16[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd17[]  =	{0xC5,0x4E,0xA7,0x01};
static const char otm8019a_cpt45_cmd18[]  =	{0x00,0xB1};
static const char otm8019a_cpt45_cmd19[]  =	{0xC5,0xA9};
static const char otm8019a_cpt45_cmd20[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd21[]  =	{0xD9,0x3C};
static const char otm8019a_cpt45_cmd22[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd23[]  =	{0xE1,0x00,0x07,0x0e,0x22,0x39,0x4e,0x57,0x87,0x77,0x8e,0x78,0x66,0x7d,0x68,0x6e,0x66,0x5f,0x55,0x4a,0x00};
static const char otm8019a_cpt45_cmd24[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd25[]  =	{0xE2,0x00,0x07,0x0e,0x22,0x39,0x4e,0x57,0x87,0x77,0x8e,0x78,0x66,0x7d,0x68,0x6e,0x66,0x5f,0x55,0x4a,0x00};
static const char otm8019a_cpt45_cmd26[]  =	{0x00,0xA7};
static const char otm8019a_cpt45_cmd27[]  =	{0xB3,0x00};
static const char otm8019a_cpt45_cmd28[]  =	{0x00,0x92};
static const char otm8019a_cpt45_cmd29[]  =	{0xB3,0x45};
static const char otm8019a_cpt45_cmd30[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd31[]  =	{0xB3,0x02};
static const char otm8019a_cpt45_cmd32[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd33[]  =	{0xC0,0x00,0x15,0x00,0x00,0x00,0x03};
static const char otm8019a_cpt45_cmd34[]  =	{0x00,0xa0};
static const char otm8019a_cpt45_cmd35[]  =	{0xc1,0xe8};
static const char otm8019a_cpt45_cmd36[]  =	{0x00,0x80};
static const char otm8019a_cpt45_cmd37[]  =	{0xCE,0x87,0x03,0x00,0x86,0x03,0x00};
static const char otm8019a_cpt45_cmd38[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd39[]  =	{0xCE,0x33,0x54,0x00,0x33,0x55,0x00};
static const char otm8019a_cpt45_cmd40[]  =	{0x00,0xA0};
static const char otm8019a_cpt45_cmd41[]  =	{0xCE,0x38,0x03,0x03,0x58,0x00,0x00,0x00,0x38,0x02,0x03,0x59,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd42[]  =	{0x00,0xB0};
static const char otm8019a_cpt45_cmd43[]  =	{0xCE,0x38,0x01,0x03,0x5A,0x00,0x00,0x00,0x38,0x00,0x03,0x5B,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd44[]  =	{0x00,0xC0};
static const char otm8019a_cpt45_cmd45[]  =	{0xCE,0x30,0x00,0x03,0x5C,0x00,0x00,0x00,0x30,0x01,0x03,0x5D,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd46[]  =	{0x00,0xD0};
static const char otm8019a_cpt45_cmd47[]  =	{0xCE,0x38,0x05,0x03,0x5E,0x00,0x00,0x00,0x38,0x04,0x03,0x5F,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd48[]  =	{0x00,0xC0};
static const char otm8019a_cpt45_cmd49[]  =	{0xCF,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x09};
static const char otm8019a_cpt45_cmd50[]  =	{0x00,0xC0};
static const char otm8019a_cpt45_cmd51[]  =	{0xCB,0x00,0x01,0x01,0x01,0x01,0x00,0x00,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd52[]  =	{0x00,0xD0};
static const char otm8019a_cpt45_cmd53[]  =	{0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x00};
static const char otm8019a_cpt45_cmd54[]  =	{0x00,0xE0};
static const char otm8019a_cpt45_cmd55[]  =	{0xCB,0x00,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd56[]  =	{0x00,0x80};
static const char otm8019a_cpt45_cmd57[]  =	{0xCC,0x00,0x26,0x25,0x02,0x06,0x00,0x00,0x0A,0x0E,0x0C};
static const char otm8019a_cpt45_cmd58[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd59[]  =	{0xCC,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd60[]  =	{0x00,0xA0};
static const char otm8019a_cpt45_cmd61[]  =	{0xCC,0x0f,0x0b,0x0d,0x09,0x00,0x00,0x05,0x01,0x25,0x26,0x00,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd62[]  =	{0x00,0xB0};
static const char otm8019a_cpt45_cmd63[]  =	{0xCC,0x00,0x25,0x26,0x05,0x01,0x00,0x00,0x0D,0x09,0x0B};
static const char otm8019a_cpt45_cmd64[]  =	{0x00,0xC0};
static const char otm8019a_cpt45_cmd65[]  =	{0xCC,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd66[]  =	{0x00,0xD0};
static const char otm8019a_cpt45_cmd67[]  =	{0xCC,0x10,0x0c,0x0a,0x0e,0x00,0x00,0x02,0x06,0x26,0x25,0x00,0x00,0x00,0x00,0x00};
static const char otm8019a_cpt45_cmd68[]  =	{0x00,0x80};
static const char otm8019a_cpt45_cmd69[]  =	{0xC4,0x30};
static const char otm8019a_cpt45_cmd70[]  =	{0x00,0x98};
static const char otm8019a_cpt45_cmd71[]  =	{0xC0,0x00};
static const char otm8019a_cpt45_cmd72[]  =	{0x00,0xa9};
static const char otm8019a_cpt45_cmd73[]  =	{0xC0,0x0A};
static const char otm8019a_cpt45_cmd74[]  =	{0x00,0xb0};
static const char otm8019a_cpt45_cmd75[]  =	{0xC1,0x20,0x00,0x00};
static const char otm8019a_cpt45_cmd76[]  =	{0x00,0xe1};
static const char otm8019a_cpt45_cmd77[]  =	{0xC0,0x40,0x30};
static const char otm8019a_cpt45_cmd78[]  =	{0x00,0x80};
static const char otm8019a_cpt45_cmd79[]  =	{0xC1,0x03,0x33};
static const char otm8019a_cpt45_cmd80[]  =	{0x00,0xA0};
static const char otm8019a_cpt45_cmd81[]  =	{0xC1,0xe8};
static const char otm8019a_cpt45_cmd82[]  =	{0x00,0x90};
static const char otm8019a_cpt45_cmd83[]  =	{0xb6,0xb4};
static const char otm8019a_cpt45_cmd84[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd85[]  =	{0xfb,0x01};
static const char otm8019a_cpt45_cmd86[]  =	{0x00,0x00};
static const char otm8019a_cpt45_cmd87[]  =	{0xFF,0xFF,0xFF,0xFF};

static const char otm8019a_cpt45_exit_sleep[] = {0x11, 0x00};
static const char otm8019a_cpt45_display_on[] = {0x29, 0x00};
static const char otm8019a_cpt45_enter_sleep[] = {0x10, 0x00};
static const char otm8019a_cpt45_display_off[] = {0x28, 0x00};

static const char otm8019a_cpt45_read_id[] = {0xa1, 0x00};
static const char otm8019a_cpt45_max_pkt[] = {0x4, 0x00};

static const struct rda_dsi_cmd otm8019a_cpt45_init_cmd_part[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd0),otm8019a_cpt45_cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd1),otm8019a_cpt45_cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd2),otm8019a_cpt45_cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd3),otm8019a_cpt45_cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd4),otm8019a_cpt45_cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd5),otm8019a_cpt45_cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd6),otm8019a_cpt45_cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd7),otm8019a_cpt45_cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd8),otm8019a_cpt45_cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd9),otm8019a_cpt45_cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd10),otm8019a_cpt45_cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd11),otm8019a_cpt45_cmd11},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd12),otm8019a_cpt45_cmd12},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd13),otm8019a_cpt45_cmd13},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd14),otm8019a_cpt45_cmd14},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd15),otm8019a_cpt45_cmd15},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd16),otm8019a_cpt45_cmd16},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd17),otm8019a_cpt45_cmd17},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd18),otm8019a_cpt45_cmd18},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd19),otm8019a_cpt45_cmd19},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd20),otm8019a_cpt45_cmd20},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd21),otm8019a_cpt45_cmd21},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd22),otm8019a_cpt45_cmd22},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd23),otm8019a_cpt45_cmd23},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd24),otm8019a_cpt45_cmd24},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd25),otm8019a_cpt45_cmd25},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd26),otm8019a_cpt45_cmd26},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd27),otm8019a_cpt45_cmd27},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd28),otm8019a_cpt45_cmd28},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd29),otm8019a_cpt45_cmd29},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd30),otm8019a_cpt45_cmd30},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd31),otm8019a_cpt45_cmd31},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd32),otm8019a_cpt45_cmd32},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd33),otm8019a_cpt45_cmd33},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd34),otm8019a_cpt45_cmd34},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd35),otm8019a_cpt45_cmd35},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd36),otm8019a_cpt45_cmd36},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd37),otm8019a_cpt45_cmd37},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd38),otm8019a_cpt45_cmd38},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd39),otm8019a_cpt45_cmd39},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd40),otm8019a_cpt45_cmd40},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd41),otm8019a_cpt45_cmd41},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd42),otm8019a_cpt45_cmd42},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd43),otm8019a_cpt45_cmd43},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd44),otm8019a_cpt45_cmd44},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd45),otm8019a_cpt45_cmd45},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd46),otm8019a_cpt45_cmd46},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd47),otm8019a_cpt45_cmd47},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd48),otm8019a_cpt45_cmd48},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd49),otm8019a_cpt45_cmd49},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd50),otm8019a_cpt45_cmd50},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd51),otm8019a_cpt45_cmd51},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd52),otm8019a_cpt45_cmd52},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd53),otm8019a_cpt45_cmd53},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd54),otm8019a_cpt45_cmd54},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd55),otm8019a_cpt45_cmd55},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd56),otm8019a_cpt45_cmd56},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd57),otm8019a_cpt45_cmd57},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd58),otm8019a_cpt45_cmd58},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd59),otm8019a_cpt45_cmd59},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd60),otm8019a_cpt45_cmd60},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd61),otm8019a_cpt45_cmd61},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd62),otm8019a_cpt45_cmd62},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd63),otm8019a_cpt45_cmd63},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd64),otm8019a_cpt45_cmd64},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd65),otm8019a_cpt45_cmd65},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd66),otm8019a_cpt45_cmd66},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd67),otm8019a_cpt45_cmd67},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd68),otm8019a_cpt45_cmd68},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd69),otm8019a_cpt45_cmd69},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd70),otm8019a_cpt45_cmd70},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd71),otm8019a_cpt45_cmd71},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd72),otm8019a_cpt45_cmd72},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd73),otm8019a_cpt45_cmd73},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd74),otm8019a_cpt45_cmd74},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd75),otm8019a_cpt45_cmd75},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd76),otm8019a_cpt45_cmd76},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd77),otm8019a_cpt45_cmd77},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd78),otm8019a_cpt45_cmd78},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd79),otm8019a_cpt45_cmd79},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd80),otm8019a_cpt45_cmd80},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd81),otm8019a_cpt45_cmd81},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd82),otm8019a_cpt45_cmd82},
	{DTYPE_DCS_LWRITE,10,sizeof(otm8019a_cpt45_cmd83),otm8019a_cpt45_cmd83},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd84),otm8019a_cpt45_cmd84},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd85),otm8019a_cpt45_cmd85},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd86),otm8019a_cpt45_cmd86},
	{DTYPE_DCS_LWRITE,0,sizeof(otm8019a_cpt45_cmd87),otm8019a_cpt45_cmd87},
};

static const struct rda_dsi_cmd otm8019a_cpt45_exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(otm8019a_cpt45_exit_sleep),otm8019a_cpt45_exit_sleep},
};

static const struct rda_dsi_cmd otm8019a_cpt45_display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(otm8019a_cpt45_display_on),otm8019a_cpt45_display_on},
};

static const struct rda_dsi_cmd otm8019a_cpt45_enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(otm8019a_cpt45_enter_sleep),otm8019a_cpt45_enter_sleep},
};

static const struct rda_dsi_cmd otm8019a_cpt45_display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(otm8019a_cpt45_display_off),otm8019a_cpt45_display_off},
};

static const struct rda_dsi_cmd otm8019a_cpt45_read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(otm8019a_cpt45_max_pkt),otm8019a_cpt45_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(otm8019a_cpt45_read_id),otm8019a_cpt45_read_id},
};

static int otm8019a_cpt45_mipi_reset_gpio(void)
{
	printk("%s\n",__func__);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(120);

	return 0;
}

static bool otm8019a_cpt45_mipi_match_id(void)
{
	struct lcd_panel_info *lcd = (void *)&(otm8019a_cpt45_mipi_info.lcd);
	u8 id[4];

	rda_lcdc_pre_enable_lcd(lcd, 1);
	otm8019a_cpt45_mipi_reset_gpio();
	rda_dsi_spkt_cmds_read(otm8019a_cpt45_read_id_cmd, ARRAY_SIZE(otm8019a_cpt45_read_id_cmd));
	rda_dsi_read_data(id, 4);
	rda_lcdc_reset();

	printk("get id 0x%02x%02x\n", id[2],id[3]);

	rda_lcdc_pre_enable_lcd(lcd, 0);

	if ((id[2] == 0x80) && (id[3] == 0x19))
		return true;

	return false;

}

static int otm8019a_cpt45_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);

	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = otm8019a_cpt45_init_cmd_part;
	cmdlist->cmds_cnt = ARRAY_SIZE(otm8019a_cpt45_init_cmd_part);
	cmdlist->flags = DSI_LP_MODE;
	cmdlist->rxlen = 0;

	trans = kzalloc(sizeof(struct rda_dsi_transfer), GFP_KERNEL);
	if (!trans) {
		pr_err("%s : no memory for trans\n", __func__);
		goto err_no_memory;
	}
	trans->cmdlist = cmdlist;

	rda_dsi_cmdlist_send_lpkt(trans);

	/*display on*/
	rda_dsi_cmd_send(otm8019a_cpt45_exit_sleep_cmd);
	rda_dsi_cmd_send(otm8019a_cpt45_display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int otm8019a_cpt45_mipi_sleep(void)
{
	printk("%s\n",__func__);

	mipi_dsi_switch_lp_mode();

	/*display off*/
	rda_dsi_cmd_send(otm8019a_cpt45_display_off_cmd);
	rda_dsi_cmd_send(otm8019a_cpt45_enter_sleep_cmd);

	return 0;
}

static int otm8019a_cpt45_mipi_wakeup(void)
{
	printk("%s\n",__func__);

	/* as we go entire power off, we need to re-init the LCD */
	otm8019a_cpt45_mipi_reset_gpio();
	otm8019a_cpt45_mipi_init_lcd();

	return 0;
}

static int otm8019a_cpt45_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int otm8019a_cpt45_mipi_set_rotation(int rotate)
{
	return 0;
}

static int otm8019a_cpt45_mipi_close(void)
{
	return 0;
}

static int otm8019a_cpt45_lcd_dbg_w(void *p,int n)
{
	return 0;
}


static struct rda_lcd_info otm8019a_cpt45_mipi_info = {
	.name = OTM8019A_CPT45_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = otm8019a_cpt45_mipi_reset_gpio,
		.s_open = otm8019a_cpt45_mipi_init_lcd,
		.s_match_id = otm8019a_cpt45_mipi_match_id,
		.s_active_win = otm8019a_cpt45_mipi_set_active_win,
		.s_rotation = otm8019a_cpt45_mipi_set_rotation,
		.s_sleep = otm8019a_cpt45_mipi_sleep,
		.s_wakeup = otm8019a_cpt45_mipi_wakeup,
		.s_close = otm8019a_cpt45_mipi_close,
		.s_rda_lcd_dbg_w = otm8019a_cpt45_lcd_dbg_w},
	.lcd = {
		.width = FWVGA_LCDD_DISP_X,
		.height = FWVGA_LCDD_DISP_Y,
		.bpp = 16,
		.lcd_interface = LCD_IF_DSI,
		.mipi_pinfo = {
			.data_lane = 2,
			.vc = 0,
			.mipi_mode = DSI_VIDEO_MODE,
			.pixel_format = RGB_PIX_FMT_RGB565,
			.dsi_format = DSI_FMT_RGB565,
			.rgb_order = RGB_ORDER_BGR,
			.trans_mode = DSI_BURST,
			.bllp_enable = true,
                        .h_sync_active = 0x14,
                        .h_back_porch = 0x1D,
                        .h_front_porch = 0x2A,
                        .v_sync_active = 0x8,
                        .v_back_porch = 0x8,
                        .v_front_porch = 0x10,
			.vat_line = 854,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 260,
			.debug_mode = 1,
			.dsi_phy_db = &otm8019a_cpt45_pll_phy_260mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_otm8019a_cpt45_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&otm8019a_cpt45_mipi_info);

	dev_info(&pdev->dev, "rda panel otm8019a_cpt45_mipi registered\n");

	return 0;
}

static int rda_panel_otm8019a_cpt45_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_otm8019a_cpt45_mipi_driver = {
	.probe = rda_panel_otm8019a_cpt45_mipi_probe,
	.remove = rda_panel_otm8019a_cpt45_mipi_remove,
	.driver = {
		.name = OTM8019A_CPT45_MIPI_PANEL_NAME}
};

static struct rda_panel_driver otm8019a_cpt45_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &otm8019a_cpt45_mipi_info,
	.pltaform_panel_driver = &rda_panel_otm8019a_cpt45_mipi_driver,
};

