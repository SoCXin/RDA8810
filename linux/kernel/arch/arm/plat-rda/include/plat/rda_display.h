#ifndef __RDA_DISPLAY_H
#define __RDA_DISPLAY_H

#define FB_IOC_MAGIC	'R'
#define FB_STRETCH_BLIT	_IOW(FB_IOC_MAGIC, 0x1, unsigned int)
#define FB_SET_OVL_VAR	_IOW(FB_IOC_MAGIC, 0x2, unsigned int)
#define FB_GET_OVL_VAR	_IOWR(FB_IOC_MAGIC, 0x3, unsigned int)
#define FB_SET_OSD_BUF	_IOW(FB_IOC_MAGIC, 0x4, unsigned int)
#define FB_BLEND_LAYER	_IOW(FB_IOC_MAGIC, 0x5, unsigned int)
#define FB_CLOSE_LAYER	_IOW(FB_IOC_MAGIC, 0x6, unsigned int)
#define FB_SET_VID_BUF	_IOW(FB_IOC_MAGIC, 0x7, unsigned int)
#define FB_COMPOSE_LAYERS	_IOW(FB_IOC_MAGIC, 0x8, unsigned int)
#define FB_SET_MODE	_IOW(FB_IOC_MAGIC, 0x9, unsigned int)
#define FB_CLR_MODE	_IOW(FB_IOC_MAGIC, 0xa, unsigned int)
#define FB_SET_TE	_IOW(FB_IOC_MAGIC, 0xb, unsigned int)
#define FB_FACTORY_AUTO_TEST _IOR(FB_IOC_MAGIC, 0xc, unsigned int)
#define FB_HW_SYNC_EN _IOR(FB_IOC_MAGIC, 0xd, unsigned int)
#define FB_HW_SYNC _IOR(FB_IOC_MAGIC, 0xe, unsigned int)

#define RDA_MAX_LAYER_NUM	5
#define RDA_MAX_FENCE_FD	RDA_MAX_LAYER_NUM

enum gouda_input_fmt {
	GOUDA_IMG_FORMAT_RGB565 = 0,
	GOUDA_IMG_FORMAT_UYVY = 1,
	GOUDA_IMG_FORMAT_YUYV = 2,
	GOUDA_IMG_FORMAT_IYUV = 3,
	GOUDA_IMG_FORMAT_RGBA = 4
};

enum gouda_output_fmt {
	GOUDA_OUTPUT_FORMAT_RGB565 = 0,
	GOUDA_OUTPUT_FORMAT_RGB888 = 1,
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
	PHY_ADDR_TYPE = 0,
	ION_TYPE,
	FB_INDEX_TYPE,
};

enum gouda_ovl_layer_id {
	GOUDA_OVL_LAYER_ID0 = 0,
	GOUDA_OVL_LAYER_ID1,
	GOUDA_OVL_LAYER_ID2,
	GOUDA_OVL_LAYER_ID3,
	GOUDA_OVL_LAYER_ID4,
	GOUDA_VID_LAYER_ID,
	GOUDA_OVL_LAYER_ID_QTY
};

enum gouda_vid_layer_depth {
	/* Video layer behind all Overlay layers */
	GOUDA_VID_LAYER_BEHIND_ALL = 0,
	/* Video layer between Overlay layers 1 and 0 */
	GOUDA_VID_LAYER_BETWEEN_1_0,
	GOUDA_VID_LAYER_BETWEEN_2_1,
	GOUDA_VID_LAYER_BETWEEN_3_2,
	GOUDA_VID_LAYER_BETWEEN_4_3,
	GOUDA_VID_LAYER_BETWEEN_5_4,
	/* Video layer on top of all Overlay layers */
	GOUDA_VID_LAYER_OVER_ALL
};

struct gouda_rect {
	int tlx;
	int tly;
	int brx;
	int bry;
};

//Please nots: alpha, rotation, depth is not used for ARGB input
//and rotation & depth field is not useful for osd layer
struct gouda_blend_var {
	int chroma_key_en;
	int alpha;

	/*only video layer*/
	int rotation;
	int depth;
};

struct ion_buf {
	int ion_fd;
};

#ifndef UINT32
typedef unsigned int UINT32;
#endif

struct fb_buf {
	int fb_index;
};

struct gouda_layer_buf {
	int addr_type;
	union {
		struct ion_buf ionbuf;
		struct fb_buf fbbuf;
	};

	int stride;
	unsigned int buf;
	unsigned int src_offset;
	unsigned int dst_offset;
};

struct gouda_layer_blend_var {
	int layer_mask;
	struct gouda_layer_buf buf_var;
	struct gouda_rect blend_roi;
};

struct gouda_vl_input {
	unsigned int buf_y;
	unsigned int buf_u;
	unsigned int buf_v;
	int width;
	int height;
	enum gouda_input_fmt input_fmt;
	struct gouda_layer_buf buf_var;
};

struct gouda_vl_output {
	enum gouda_vid_rotation vl_rot;
	enum gouda_output_fmt output_fmt;
	struct gouda_layer_buf buf_var;
	struct gouda_rect vl_rect;
};

struct gouda_vid_layer_var {
	struct gouda_vl_input src;
	struct gouda_vl_output dst;
};

struct gouda_ovl_layer_var {
	enum gouda_ovl_layer_id ovl_id;
	enum gouda_input_fmt input_fmt;

	struct gouda_layer_buf buf_var;
	struct gouda_rect ovl_rect;
	struct gouda_blend_var blend_var;
};

struct gouda_compose_var {
	struct gouda_vid_layer_var vid_var;
	struct gouda_ovl_layer_var ovl_var[RDA_MAX_LAYER_NUM];
	struct gouda_layer_blend_var ctl_var;
};

struct gouda_buf_sync {
	int *acq_fence_fd;
	int *rel_fence_fd;
	int acq_fence_cnt;
	int hw_flag;
};

struct gouda_vid_layer_def {
	unsigned int *addr_y;
	unsigned int *addr_u;
	unsigned int *addr_v;

	unsigned short height;
	unsigned short width;
	unsigned short stride;

	enum gouda_input_fmt input_fmt;
	enum gouda_output_fmt output_fmt;
	struct gouda_rect rect;
};

/*
*lcdc
*/
enum rda_lcd_interface {
	LCD_IF_DBI = 0,	// 8080
	LCD_IF_DPI = 1,	// RGB
	LCD_IF_DSI = 2	// MIPI
};

enum lcd_cs {
	LCD_CS_0 = 0,
	LCD_CS_1 = 1,
	LCD_CS_QTY,
	LCD_MEMORY_IF = 2,
	LCD_IN_RAM = 3
};

struct lcd_img_rect {
	int tlx;
	int tly;
	int brx;
	int bry;
};

#define NUM_FRAMEBUFFERS	2
#define RDA_HW_ON_DEBUG	0

#endif /* __RDA_DISPLAY_H */
