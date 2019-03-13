#ifndef __REG_GOUDA_H
#define __REG_GOUDA_H

#define FB_IOC_MAGIC	'R'
#define FB_STRETCH_BLIT	_IOW(FB_IOC_MAGIC, 0x1, unsigned int)
#define FB_SET_OVL_VAR	_IOW(FB_IOC_MAGIC, 0x2, unsigned int)
#define FB_GET_OVL_VAR	_IOWR(FB_IOC_MAGIC, 0x3, unsigned int)
#define FB_SET_OSD_BUF	_IOW(FB_IOC_MAGIC, 0x4, unsigned int)
#define FB_OPEN_LAYER	_IOW(FB_IOC_MAGIC, 0x5, unsigned int)
#define FB_CLOSE_LAYER	_IOW(FB_IOC_MAGIC, 0x6, unsigned int)
#define FB_SET_VID_BUF	_IOW(FB_IOC_MAGIC, 0x7, unsigned int)
#define FB_COMPOSE_LAYERS	_IOW(FB_IOC_MAGIC, 0x8, unsigned int)
#define FB_SET_MODE	_IOW(FB_IOC_MAGIC, 0x9, unsigned int)
#define FB_CLR_MODE	_IOW(FB_IOC_MAGIC, 0xa, unsigned int)
#define FB_SET_TE	_IOW(FB_IOC_MAGIC, 0xb, unsigned int)
#define FB_FACTORY_AUTO_TEST _IOR(FB_IOC_MAGIC, 0xc, unsigned int)

#define GD_MAX_OUT_WIDTH		(640)
#define GD_NB_BITS_LCDPOS		(10)
#define GD_FP_FRAC_SIZE 		(8)
#define GD_FIXEDPOINT_SIZE		(3+GD_FP_FRAC_SIZE)
#define GD_NB_BITS_STRIDE		(13)
#define GD_NB_WORKBUF_WORDS 		(GD_MAX_OUT_WIDTH*2)
#define GD_NB_LCD_CMD_WORDS 		(64)
#define GD_SRAM_SIZE			((GD_NB_WORKBUF_WORDS+GD_NB_LCD_CMD_WORDS)*2)
#define GD_SRAM_ADDR_WIDTH		(11)

#define GD_MAX_SLCD_READ_LEN		(4)
#define GD_MAX_SLCD_CLK_DIVIDER 	(255)

enum gouda_lcd_interface {
	GOUDA_LCD_IF_DBI = 0,	// 8080
	GOUDA_LCD_IF_DPI = 1,	// RGB
	GOUDA_LCD_IF_DSI = 2	// MIPI
};

enum gouda_buf_dir {
	GOUDA_BUF_IN = 0,
	GOUDA_BUF_OUT
};

enum gouda_work_mode {
	GOUDA_NONBLOCKING = 0,
	GOUDA_BLOCKING
};

enum gouda_lcd_output_fmt {
	/// 8-bit - RGB3:3:2 - 1cycle/1pixel - RRRGGGBB
	GOUDA_LCD_OUTPUT_FORMAT_8_BIT_RGB332 = 0,
	/// 8-bit - RGB4:4:4 - 3cycle/2pixel - RRRRGGGG/BBBBRRRR/GGGGBBBB
	GOUDA_LCD_OUTPUT_FORMAT_8_bit_RGB444 = 1,
	/// 8-bit - RGB5:6:5 - 2cycle/1pixel - RRRRRGGG/GGGBBBBB
	GOUDA_LCD_OUTPUT_FORMAT_8_bit_RGB565 = 2,
	/// 16-bit - RGB3:3:2 - 1cycle/2pixel - RRRGGGBBRRRGGGBB
	GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB332 = 4,
	/// 16-bit - RGB4:4:4 - 1cycle/1pixel - XXXXRRRRGGGGBBBB
	GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB444 = 5,
	/// 16-bit - RGB5:6:5 - 1cycle/1pixel - RRRRRGGGGGGBBBBB
	GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565 = 6,
	/// 32-bit ARGB8:8:8:8 
	GOUDA_LCD_OUTPUT_FORMAT_32_bit_ARGB8888 = 7
};

enum gouda_img_fmt {
	GOUDA_IMG_FORMAT_RGB565 = 0,
	GOUDA_IMG_FORMAT_UYVY = 1,
	GOUDA_IMG_FORMAT_YUYV = 2,
	GOUDA_IMG_FORMAT_IYUV = 3,
	GOUDA_IMG_FORMAT_RGBA = 4
};

enum gouda_ckey_mask {
	/* exact color match */
	GOUDA_CKEY_MASK_OFF = 0,
	/* disregard 1 LSBit of each color component for matching */
	GOUDA_CKEY_MASK_1LSB = 1,
	/* disregard 2 LSBit of each color component for matching */
	GOUDA_CKEY_MASK_2LSB = 3,
	/* disregard 3 LSBit of each color component for matching */
	GOUDA_CKEY_MASK_3LSB = 7
};

enum gouda_vid_rotation {
	GOUDA_VID_NO_ROTATION = 0,
	GOUDA_VID_90_ROTATION,
	GOUDA_VID_180_ROTATION,
	GOUDA_VID_270_ROTATION
};

enum gouda_addr_type {
	PhyAddrType = 0,
	IonFdType,
	FrmBufIndexType,
};


enum gouda_ovl_layer_id {
	GOUDA_OVL_LAYER_ID0 = 0,
	GOUDA_OVL_LAYER_ID1,
	GOUDA_OVL_LAYER_ID2,
	GOUDA_VID_LAYER_ID3,
	GOUDA_OVL_LAYER_ID_QTY
};

struct gouda_input {
	unsigned int bufY;
	unsigned int bufU;
	unsigned int bufV;
	int srcOffset;
	int stride;
	int width;
	int height;
	int cacheflag;
	enum gouda_addr_type addr_type;
	enum gouda_img_fmt image_fmt;
	//enum gouda_vid_rotation rotation;
};

struct gouda_rect {
	short tlX;
	short tlY;
	short brX;
	short brY;
};

struct gouda_output {
	unsigned int buf;
	enum gouda_addr_type addr_type;
	enum gouda_vid_rotation rot;
	struct gouda_rect pos;
};

struct gouda_vid_blit_strech_var {
	struct gouda_input src;
	struct gouda_output dst;
};

#ifndef UINT32
typedef unsigned int UINT32;
#endif

//Please nots: alpha, rotation, depth is not used for ARGB input
//and rotation & depth field is not useful for osd layer
struct gouda_ovl_blend {
	UINT32 chroma_key_b:5;
	UINT32 chroma_key_g:6;
	UINT32 chroma_key_r:5;
	UINT32 chroma_key_en:1;
	UINT32 chroma_key_mask:3;
	UINT32 alpha:8; //one_minus_alpha , 0 means 255 for current layer
	UINT32 rotation:2;
	UINT32 depth:2;
};

struct gouda_ovl_composition_var {
	enum gouda_ovl_layer_id ovl_id;
	struct gouda_ovl_blend ovl_blend_var;
};

struct gouda_ovl_buf_var {
	//relative pos comparing to ROI
	enum gouda_ovl_layer_id ovl_id;
	struct gouda_ovl_blend blend_var;
	unsigned int buf;
	int srcOffset;
	struct gouda_rect pos;
	int stride;
	enum gouda_img_fmt image_fmt;
	int cacheflag;
	enum gouda_addr_type addr_type;
};

struct gouda_ovl_open_var{
	int layerMask;
	int dstBuf;
	int dstOffset;
	struct gouda_rect roi;
	enum gouda_addr_type addr_type;
	int isLast;
};

struct gouda_compose_var {
	struct gouda_vid_blit_strech_var vid_var;
	struct gouda_ovl_buf_var osd_var[3];
	struct gouda_ovl_open_var ctl_var;
};

/*lcdc*/
struct lcd_img_rect {
	short tlX;
	short tlY;
	short brX;
	short brY;
};

#endif /* __RDA_GOUDA_H */
