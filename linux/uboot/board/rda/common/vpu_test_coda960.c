/**********************************
vpu test.
modify ver 1.0	: for CODA960-8810
2014.12.29
modify ver 1.1
2015.01.07
sheen
***********************************/

#include "coda960_regs.h" //sync with cnm-coda-sw-pkg within android project.
//vpu firmware
#include "coda960_fw.h"

//test video stream.
#define FRAME_TEST_NUM	(14) //decode frmae num < bit_stream frame num.
const unsigned int bit_stream[] =
{
//don't rename bitstream!!
//#include "bs_h264_5f_720x480.txt"
#include "freh3_h264_99f_hp_cif.txt"
};
//16bytes each md5 digest.
const unsigned char yuv_md5_digest[]=
{
//bs_h264_5f_720x480.txt yuv md5,create by coda960 cmodel.
//#include "bs_h264_5f_720x480.md5"
#include "freh3_h264_30f_hp_cif_decOrder.md5"
};
//#include <u-boot/md5.h>
#include "rda_md5.h"
/*************************************************
	configs
**************************************************/
//0= little endian, 1= big endian.
#define VPU_FRAME_ENDIAN			0
#define VPU_STREAM_ENDIAN			0

#define VPU_ENABLE_BWB			0	//burst write back. wirtes output with 8 burst in linear map mode.
#define	CBCR_INTERLEAVE			0	//[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE
#define VPU_REPORT_USERDATA		0//if enabled, user data is writen to user data buffer
#define	USE_BIT_INTERNAL_BUF	1 //enable secondary AXI for prediction data of the bit-processor.
#define USE_IP_INTERNAL_BUF		1 //enable secondary AXI for row pixel data of IP/AC-DC.
#define	USE_DBKY_INTERNAL_BUF	1 //enable secondary AXI for temporal luminance data of the de-blocking filter.
#define	USE_DBKC_INTERNAL_BUF	1 //enable secondary AXI for temporal chrominance data of the de-blocking filter.
#define	USE_OVL_INTERNAL_BUF	0 //enable secondary AXI for temporal data of the overlap filter(VC1 only)
#define	USE_BTP_INTERNAL_BUF	0//enable secondary AXI for bit-plane data of the bit-processor(VC1 only).

typedef enum {
	INT_BIT_INIT = 0,
	INT_BIT_SEQ_INIT = 1,
	INT_BIT_SEQ_END = 2,
	INT_BIT_PIC_RUN = 3,
	INT_BIT_FRAMEBUF_SET = 4,
	INT_BIT_ENC_HEADER = 5,
	INT_BIT_DEC_PARA_SET = 7,
	INT_BIT_DEC_BUF_FLUSH = 8,
	INT_BIT_USERDATA = 9,
	INT_BIT_DEC_MB_ROWS = 13,
	INT_BIT_BIT_BUF_EMPTY = 14,
	INT_BIT_BIT_BUF_FULL = 15
}InterruptBit;

//ms
#define VPU_ENC_TIMEOUT				5000
#define VPU_DEC_TIMEOUT				5000


/*****************************************************
	register and memory base address.
******************************************************/
//vpu reg base addr. sheen
#define BIT_REG_BASE		0x20830000 //CODA960
#define BIT_REG_SIZE		0x4000

//second AXI base addr.
// SECOND AXI ACCCESS MEMORY ON CHIP
//#define	HD_SEC_AXI_BASE_ADDR	0x1C00000	//8810 CODA960
#define	HD_SEC_AXI_BASE_ADDR	0x100000	//8810 CODA960

//external sdram base addr. sheen
// FIRST AXI ACCCESS SDRAM
//#define	HD_BASE_ADDR					0x8db00000
#define	HD_BASE_ADDR					0x88000000  //minimum reserve 128M for 32 frames 1080P

// Base address of the bitstream buffer. for the whole bitstream.
#define	HD_ADDR_BIT_STREAM				(HD_BASE_ADDR + 0x000000)
// Size of the bitstream buffer in byte,(1M bytes)
#define	HD_STREAM_BUF_SIZE				0x100000
// Base address for the firmware image, (firmware size 126976x2)
#define	HD_ADDR_BIT_CODE				(HD_ADDR_BIT_STREAM + HD_STREAM_BUF_SIZE)
//bitcode buf size 260KB
#define CODE_BUF_SIZE	  (260*1024)//CODA960
// Base address for the firmware common parameters buffer.
//1.record decode frame buf YUV addr,384= 4*3*MAX32, 2.record MvColBuf.sheen
#define	HD_ADDR_BIT_PARA				(HD_ADDR_BIT_CODE + CODE_BUF_SIZE)
//common parameters buffer size 10KB
#define PARA_BUF_SIZE	 (10*1024) //CODA960
// Base address for the firmware common working buffer.
#define	HD_ADDR_BIT_WORK				(HD_ADDR_BIT_PARA + PARA_BUF_SIZE)
//common work buffer size, AVC MAX= (WORK_BUF_SIZE + PS_SAVE_SIZE)=(80KB+320KB)= 400KB
#define WORK_BUF_SIZE	  (400*1024) //CODA960
//base address of the firmware common temp buffer.
#define HD_ADDR_TEMP_BUF				(HD_ADDR_BIT_WORK + WORK_BUF_SIZE)
//common temp buffer size.
#define TEMP_BUF_SIZE	  (204*1024) //CODA960
// Slice Buffer of decoder. max as frame buffer.
#define HD_ADDR_SLICE_BUFFER			(HD_ADDR_TEMP_BUF + TEMP_BUF_SIZE)
// For VP8. reuse HD_ADDR_SLICE_BUFFER
#define HD_ADDR_VP8DEC_MB_BUF	  		HD_ADDR_SLICE_BUFFER
// For MP4. reuse HD_ADDR_SLICE_BUFFER
#define HD_ADDR_MP4ENC_DP_BUF	  		HD_ADDR_SLICE_BUFFER
//max slice save buffer size. max 1920*1088*1.5
#define SLICE_SAVE_SIZE				 (1920*1088*3/4)
//SPS/PPS save buffer. Parameter Set Buffer for H.264 decoder
#define HD_ADDR_PS_SAVE_BUFFER			(HD_ADDR_SLICE_BUFFER + SLICE_SAVE_SIZE)
//SPS/PPS save buffer size. CODA960 320KB, CODA7L 512KB.
#define HD_PS_SAVE_SIZE					0x100000
// Base address of the DPB.yuv frame buffer.
#define HD_ADDR_FRAME_BASE				(HD_ADDR_PS_SAVE_BUFFER	+ HD_PS_SAVE_SIZE)
//register yuv buf num for decode. MAX=32, sheen
#define HD_REG_FRAME_BUF_NUM   32
// End address of the DPB(128MB-21MB)
//resolution is limited to 1920x1088 (1920x1088x1.75x32=112 MB frame buffer size)
#define HD_MAX_FRAME_BASE				(HD_ADDR_FRAME_BASE + (1920*1088*7/4)*HD_REG_FRAME_BUF_NUM)


/********second AXI addr config***********/
//second AXI SRAM size 0x10000 (8810 CODA960)
//max size for 1920x1088(120x68MB) H.264 8810 CODA960
//Y Deblocking FIlter buffer, size 120*128= 0x3C00 bytes
#define HD_ADDR_SEC_AXI_DBKY  			(HD_SEC_AXI_BASE_ADDR)
//C Deblocking FIlter buffer, size 120*128= 0x3C00 bytes
#define HD_ADDR_SEC_AXI_DBKC			(HD_ADDR_SEC_AXI_DBKY + 0x3C00)
//MB Information, size 120*144= 0x4380 bytes
#define HD_ADDR_SEC_AXI_BIT 			(HD_ADDR_SEC_AXI_DBKC + 0x3C00)
//Intra Prediciton buffer,  size 120*64=0x1e00 bytes
#define HD_ADDR_SEC_AXI_IP			(HD_ADDR_SEC_AXI_BIT + 0x4380)
//VC1 only overlap Filter, size 0x2580 bytes
#define HD_ADDR_SEC_AXI_OVL  			(HD_ADDR_SEC_AXI_IP + 0x1E00)
//VC1 only BIT PLANE, size 0xF00  bytes
#define HD_ADDR_SEC_AXI_BTP  			(HD_ADDR_SEC_AXI_OVL + 0x2580)
#define HD_ADDR_SEC_AXI_BUF_END  		(HD_SEC_AXI_BASE_ADDR + 0x10000)

/************************************************************/

//vpu registers read/write
#define VpuWriteReg(ADDR, DATA)   *((volatile unsigned int *)(ADDR + (unsigned int)BIT_REG_BASE)) = DATA
#define VpuReadReg(ADDR)		*((volatile unsigned int *)(ADDR + (unsigned int)BIT_REG_BASE))

//directly addr read/write
#define MREAD_WORD(ADDR)	*((volatile int *)(ADDR))
#define MWRITE_WORD(ADDR,DATA)	*((volatile int *)(ADDR)) = DATA

static int _vpu_test(int md5_check)
{
	int  k;
	int  ret;
	int  sizeX;
	int  sizeY;
	int  FrameBufNum = 0;
	int  PicSize = 0;
	int  DispFrameIdx;
	int  DecDecFrameIdx;
	int  DecPicType;
	unsigned int data;
	int  dec_num=0;
	int  stride;
	unsigned int  DpbLum[HD_REG_FRAME_BUF_NUM];
	unsigned int  DpbCb[HD_REG_FRAME_BUF_NUM];
	unsigned int  DpbCr[HD_REG_FRAME_BUF_NUM];
	unsigned int  MvColBuf[HD_REG_FRAME_BUF_NUM];
	int dataSize;
	int bitStreamSize;
	unsigned char md5Data[16];
	// VPP_MEDIA_NODE p_node;

	printf("%s: start\n", __func__);

	//store firmware in sdram and then dma firmware to internal PMEM by vpu. sheen
	dataSize = sizeof(bit_code) / sizeof(bit_code[0]);//126976
	for (k = 0; k < dataSize; k += 4) {
		int dataH = 0;
		int dataL = 0;
		dataH = (bit_code[k+0] << 16) | bit_code[k+1];
		dataL = (bit_code[k+2] << 16) | bit_code[k+3];
		// 64 BIT BIG Endian
		MWRITE_WORD(HD_ADDR_BIT_CODE + k * 2, dataL);
		MWRITE_WORD(HD_ADDR_BIT_CODE+k * 2 + 4, dataH);
		//j=50;   // which is 338us low level
		//while(--j);
	}

	flush_dcache_range(HD_ADDR_BIT_CODE, HD_ADDR_BIT_CODE+CODE_BUF_SIZE);

	//init_vpu register
	for(k = 0; k < 0x200; k += 4)
		MWRITE_WORD(BIT_REG_BASE + k, 0);

	VpuWriteReg(BIT_FRM_DIS_FLG, 0);//BIT_FRM_DIS_FLG

	// Start decoding configuration
	VpuWriteReg(BIT_BASE + 0xffc, 0x01); // enable clk, any other value than 0xA1B2C3D4
	VpuWriteReg(BIT_CODE_RUN, 0x0);  // BIT_CODE_RUN
	VpuWriteReg(BIT_INT_ENABLE, 0x0);  // BIT_INT_ENABLE, Disable interrupt

	//load bit stream to sdram. sheen
	dataSize = sizeof(bit_stream) / sizeof(bit_stream[0]);
	if(dataSize > (HD_STREAM_BUF_SIZE / 4))
		dataSize = HD_STREAM_BUF_SIZE / 4;
	bitStreamSize = dataSize * 4;
	for(k = 0; k < dataSize; k++) {
		data = bit_stream[k];
		MWRITE_WORD(HD_ADDR_BIT_STREAM + k * 4, data);
	}

	if(dataSize < (HD_STREAM_BUF_SIZE / 4)) {
		//set 0
		for(k = dataSize; k < (HD_STREAM_BUF_SIZE / 4); k++)
			MWRITE_WORD(HD_ADDR_BIT_STREAM + k * 4, 0);
	}

	flush_dcache_range(HD_ADDR_BIT_STREAM, HD_ADDR_BIT_STREAM+HD_STREAM_BUF_SIZE);
	// Download init common firmware. sheen
	// BIT_CODE_DOWN
	for(k = 0; k < 512; k += 4) {
		data = bit_code[k];
		VpuWriteReg(BIT_CODE_DOWN,(k << 16) | data);

		data = bit_code[k+1];
		VpuWriteReg(BIT_CODE_DOWN,((k+1) << 16) | data);

		data = bit_code[k+2];
		VpuWriteReg(BIT_CODE_DOWN,((k+2) << 16) | data);

		data = bit_code[k+3];
		VpuWriteReg(BIT_CODE_DOWN,((k+3) << 16) | data);
	}

	// Initialize the CODA
	VpuWriteReg(BIT_PARA_BUF_ADDR, HD_ADDR_BIT_PARA);	  // BIT_PARA_BUF_ADDR
	//VpuWriteReg(BIT_BASE+0x104, HD_ADDR_BIT_WORK);	  // BIT_WORK_BUF_ADDR
	VpuWriteReg(BIT_CODE_BUF_ADDR, HD_ADDR_BIT_CODE);	  // BIT_CODE_BUF_ADDR
	VpuWriteReg(BIT_BIT_STREAM_CTRL, 0x0);  					// BIT_BIT_STREAM_CTRL, 0= 64bit little endian
	VpuWriteReg(BIT_BIT_STREAM_PARAM, 0x0);  					// BIT_BIT_STREAM_PARAM
	//VpuWriteReg(BIT_BASE+0x110, 0x1); 					// Dpb Endian mode:1, 64bit big endian
	VpuWriteReg(BIT_FRAME_MEM_CTRL , 0x0); 					//BIT_FRAME_MEM_CTRL Dpb Endian mode:1, 0=64bit little endian

	VpuWriteReg(BIT_INT_ENABLE, 0x0); 					// BIT_INT_ENABLE
	VpuWriteReg(BIT_AXI_SRAM_USE, 0x0f0f);  				// BIT_AXI_SRAM_USE, 6 secAxi enable bit.(disable VC1(Ovl,Btp) )
	//VpuWriteReg(BIT_BASE+0x140, 0x78f);  				// BIT_AXI_SRAM_USE
	VpuWriteReg(BIT_BUSY_FLAG, 0x1); 					// BIT_BUSY_FLAG= 1
	VpuWriteReg(BIT_CODE_RESET, 0x1); 					// BIT_CODE_RESET
	VpuWriteReg(BIT_CODE_RUN , 0x1); 					// BIT_CODE_RUN
	while (VpuReadReg(BIT_BUSY_FLAG) == 1);   			// BIT_BUSY_FLAG = 0

	debug_printf("%s:(VPU_Init,InitializeVPU) load base code and init vpu done.\n", __func__);

	VpuWriteReg(BIT_TEMP_BUF_ADDR, HD_ADDR_TEMP_BUF);
	VpuWriteReg(BIT_RD_PTR, HD_ADDR_BIT_STREAM);	 // BIT_RD_PTR_0
	VpuWriteReg(BIT_WR_PTR, HD_ADDR_BIT_STREAM+HD_STREAM_BUF_SIZE); // BIT_WR_PTR_0, maximum is 15M, this value is for real bitstream write point position
	VpuWriteReg(BIT_BIT_STREAM_PARAM , 4); // BIT_BIT_STREAM_PARAM (bs input mode) If all the streams are feeded, the value is 4 , in demo stream, all the stream is feeded, should not exceed 15M

	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);	  // BIT_WORK_BUF_ADDR

	// wait for BIT_BUSY_FLAG = 0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);   // BIT_BUSY_FLAG = 0

	data= VpuReadReg(BIT_RD_PTR);//cur bitstream addr

	debug_printf("%s:set bit stream buf done. BIT_RD_PTR 0x%x\n", __func__,data);

	//VpuWriteReg(BIT_BASE+0x124, HD_ADDR_BIT_STREAM); // BIT_WR_PTR_0, maximum is 15M, this value is for real bitstream write point position
	//VpuWriteReg(BIT_BASE+0x110, 0x1);  // Dpb Endian mode:1, 64bit little endian
	//VpuWriteReg(BIT_BASE+0x114, 0x0);  // BIT_BIT_STREAM_PARA

	//for BIT_RUN_COMMAND: SEQ_INIT. (BIT_BASE+0x180... reuse). sheen
	VpuWriteReg(CMD_DEC_SEQ_BB_START, HD_ADDR_BIT_STREAM);//CMD_DEC_SEQ_BB_START,seq Bitstream buffer SDRAM byte address
	VpuWriteReg(CMD_DEC_SEQ_BB_SIZE, HD_STREAM_BUF_SIZE/1024);		//CMD_DEC_SEQ_BB_SIZE, 15K, Bitstream buffer size in kilo bytes count
	//VpuWriteReg(BIT_BASE+0x190, 0x0);// ? sheen
	VpuWriteReg(CMD_DEC_SEQ_OPTION, 0x2);//bit[1],enable display buffer reordering.
	//VpuWriteReg(BIT_BASE+0x194, HD_ADDR_PS_SAVE_BUFFER);// CMD_DEC_SEQ_PS_BB_START ? sheen
	//VpuWriteReg(BIT_BASE+0x198, 0x200);							// CMD_DEC_SEQ_PS_BB_SIZE
	//VpuWriteReg(BIT_BASE+0x198, 0x80);// CMD_DEC_SEQ_PS_BB_SIZE ? sheen
	VpuWriteReg(CMD_DEC_SEQ_X264_MV_EN, 0x1);//support x264. sheen
	VpuWriteReg(CMD_DEC_SEQ_SPP_CHUNK_SIZE, 512);//GBU(get bit unit) size.

	//VpuWriteReg(0xA0000000+0x198, HD_ADDR_PS_SAVE_BUFFER+0x200);// CMD_DEC_SEQ_PS_BB_SIZE

	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);   // BIT_BUSY_FLAG = 0

	debug_printf("%s: seq dec set done.\n", __func__);

	// CMD_DEC_SEQ_OPTION
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);// set busy.sheen
	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
	VpuWriteReg(BIT_RUN_INDEX, 0x0); 		//BIT_RUN_INDEX
	VpuWriteReg(BIT_RUN_COD_STD, 0x0); 		//BIT_RUN_COD_STD 0: H.264 DECODER
	// Command the CODA to initialize for the sequence level
	VpuWriteReg(BIT_RUN_AUX_STD, 0x0); 		//BIT_RUN_AUX_STD

	VpuWriteReg(BIT_RUN_COMMAND, 0x1); 		//BIT_RUN_COMMAND, SEQ_INIT=1  //////////source header
	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG) == 1);   // BIT_BUSY_FLAG = 0

	data= VpuReadReg(BIT_RD_PTR);//cur bitstream addr

	debug_printf("%s:(VPU_DecGetInitialInfo) seq header init done. BIT_RD_PTR 0x%x\n", __func__,data);

#if 1
	//CONFIG IPB BUS
	for(k = 0; k < 144; k += 4)
		VpuWriteReg(BIT_BASE + 0x1800 + k, 0x4040);

	VpuWriteReg(BIT_BASE+0x1890, 0x0);
	VpuWriteReg(BIT_BASE+0x18a0, 0xc30);
	VpuWriteReg(BIT_BASE+0x18a4, 0xc30);
	VpuWriteReg(BIT_BASE+0x18a8, 0xc30);

	for(k = 0; k < 9; k++)
		VpuWriteReg(BIT_BASE + 0x18ac + 4 * k, k * 65);

	VpuWriteReg(BIT_BASE+0x18d0, 0x410);
	VpuWriteReg(BIT_BASE+0x18d4, 0x451);
	VpuWriteReg(BIT_BASE+0x18d8, 0x820);
	VpuWriteReg(BIT_BASE+0x18dc, 0x861);

	VpuWriteReg(BIT_BASE+0x18e0, 0x8a2);
	VpuWriteReg(BIT_BASE+0x18e4, 0x8e3);
	VpuWriteReg(BIT_BASE+0x18e8, 0x924);
	VpuWriteReg(BIT_BASE+0x18ec, 0x965);

	VpuWriteReg(BIT_BASE+0x18f0, 0x9a6);
	VpuWriteReg(BIT_BASE+0x18f4, 0x9e7);
	VpuWriteReg(BIT_BASE+0x18f8, 0xa28);
	VpuWriteReg(BIT_BASE+0x18fc, 0xa69);

	VpuWriteReg(BIT_BASE+0x1900, 0xaaa);
	VpuWriteReg(BIT_BASE+0x1904, 0xaeb);
	VpuWriteReg(BIT_BASE+0x1908, 0xb2c);
	VpuWriteReg(BIT_BASE+0x190c, 0xb6d);

	VpuWriteReg(BIT_BASE+0x1910, 0xbae);
	VpuWriteReg(BIT_BASE+0x1914, 0xbef);
	VpuWriteReg(BIT_BASE+0x1918, 0xc30);
	VpuWriteReg(BIT_BASE+0x191c, 0xc30);

	VpuWriteReg(BIT_BASE+0x1920, 0xc000000);
#endif

	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);   // BIT_BUSY_FLAG = 0

	//VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);  // BIT_WORK_BUF_ADDR

	// Allocate DPB to the IP
	//REG32(0x10000000) = VpuReadReg(BIT_BASE+0x1c0);   // seqInitStatus, 0 error, 1 success
	ret = VpuReadReg(RET_DEC_SEQ_SUCCESS);   	// seqInitStatus, 0 error, 1 success
	debug_printf("%s: seqInitStatus = %d\n", __func__, ret);
	if (ret == 1) {
		//REG32(0x10000004) = VpuReadReg(BIT_BASE+0x1c4);   // RET_DEC_SEQ_SRC_SIZE
		sizeX = VpuReadReg(RET_DEC_SEQ_SRC_SIZE) >> 16; // [31:16]
		sizeY = VpuReadReg(RET_DEC_SEQ_SRC_SIZE) & 0xffff; // [15: 0]
		//PicSize = sizeX * sizeY;
		//debug_printf("%s: sizex = %d, sizey = %d\n", __func__, sizeX, sizeY);

		//REG32(0x10000008) = VpuReadReg(BIT_BASE+0x1cc); // Minimum decoded frame buffer need to decode stream successfully.
		FrameBufNum = VpuReadReg(RET_DEC_SEQ_FRAME_NEED);
		//FrameBufNum = 32; //Allocate 32 frame buffer for decoding. resolution is limited to 1920x1080 (1920x1080x1.75x32=111 MB frame buffer size)
		if(FrameBufNum > HD_REG_FRAME_BUF_NUM)
			FrameBufNum = HD_REG_FRAME_BUF_NUM;
		debug_printf("%s: minimal request of frame buf num= %d\n", __func__, FrameBufNum);
		//for BIT_RUN_COMMAND: SET_FRAME_BUF
		VpuWriteReg(CMD_SET_FRAME_BUF_NUM, FrameBufNum); // Number of frames used for reference or output reodering.

		stride = ((sizeX + 15) & ~15);
		VpuWriteReg(CMD_SET_FRAME_BUF_STRIDE, stride);// 8 multiplier, resolution width
		PicSize = stride * ((sizeY + 15) & ~15);
		debug_printf("%s: bufStride= %d sizex = %d, sizey = %d\n", __func__, stride, sizeX, sizeY);

	} else {
		// Report Error message
		debug_printf("%s: seq dec err!!\n", __func__);
		return -1;
	}
	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);// BIT_BUSY_FLAG = 0


	VpuWriteReg(CMD_SET_FRAME_AXI_BIT_ADDR, HD_ADDR_SEC_AXI_BIT);//second AXI address,ADDR_AXI_BIT
	VpuWriteReg(CMD_SET_FRAME_AXI_IPACDC_ADDR, HD_ADDR_SEC_AXI_IP);//second AXI address,ADDR_AXI_IP
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKY_ADDR, HD_ADDR_SEC_AXI_DBKY);//second AXI address,ADDR_AXI_DBKY
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKC_ADDR, HD_ADDR_SEC_AXI_DBKC);//second AXI address,ADDR_AXI_DBKC
#if 0 //for VC1 only
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR, HD_ADDR_SEC_AXI_OVL);//second AXI address,ADDR_AXI_OVL
	VpuWriteReg(CMD_SET_FRAME_AXI_BTP_ADDR, HD_ADDR_SEC_AXI_BTP);//second AXI address,ADDR_AXI_BTP
#else
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR, 0); //second AXI address,ADDR_AXI_OVL
	VpuWriteReg(CMD_SET_FRAME_AXI_BTP_ADDR, 0); //second AXI address,ADDR_AXI_BTP
#endif
	VpuWriteReg(CMD_SET_FRAME_CACHE_CONFIG, 0x7e0);//2D CACHE CONFIG

	VpuWriteReg(CMD_SET_FRAME_SLICE_BB_START, HD_ADDR_SLICE_BUFFER);//ADDR_SLICE_BUFFER
	VpuWriteReg(CMD_SET_FRAME_SLICE_BB_SIZE, SLICE_SAVE_SIZE);//ADDR_SLICE_SIZE

	while (VpuReadReg(BIT_BUSY_FLAG ) == 1); // BIT_BUSY_FLAG = 0
   /*
	VpuWriteReg(BIT_RUN_INDEX, 0x0); 	// BIT_RUN_INDEX
	VpuWriteReg(BIT_RUN_COD_STD, 0x0); 	// BIT_RUN_COD_STD, H.264
	VpuWriteReg(BIT_RUN_AUX_STD, 0x0); 	// BIT_RUN_AUX_STD
	VpuWriteReg(BIT_FRM_DIS_FLG, 0x0);  // BIT_FRM_DIS_FLG
	*/

	// 4b0000: H.264 DECODER
	// 4b0001: VC-1 DECODER
	// 4b0010: MPEG-2 DECODER
	// 4b0011: MPEG-4/DivX-3 DECODER
	// 4b0100: RV DECODER
	// 4b0101: AVS DECODER
	// 4b1000: MJPEG DECODER

	// Calculate frame buffer addresses
	{
	int  addrNextLuma = HD_ADDR_FRAME_BASE;

	for (k=0; k < FrameBufNum; k = k + 1) {
		DpbLum[k]	= addrNextLuma;
		DpbCb[k]	 = DpbLum[k]   + PicSize;
		DpbCr[k]	 = DpbCb[k]	+ PicSize/4;
		MvColBuf[k]  = DpbCr[k]	+ PicSize/4;
		addrNextLuma = MvColBuf[k] + PicSize/4;
		debug_printf("k=%d y=0x%x u=0x%x v=0x%x\n", k,DpbLum[k],DpbCb[k],DpbCr[k]);
	}

	// registering the base addresses of created frame buffers, little endian is ok?
	for (k=0; k < FrameBufNum; k = k + 2) {
		// 64 BIT BIG Endian
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12)	  , DpbCb[k]   );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) +  4, DpbLum[k]  );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) +  8, DpbLum[k+1]);
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 12, DpbCr[k]   );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 16, DpbCr[k+1] );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 20, DpbCb[k+1] );
	}

	//mvCol buf
	for (k=0; k < FrameBufNum; k = k + 2) {
		// 64 BIT BIG Endian
		MWRITE_WORD(HD_ADDR_BIT_PARA + 384+(k*4)	 , MvColBuf[k+1]);
		MWRITE_WORD(HD_ADDR_BIT_PARA + 384+(k*4) +  4, MvColBuf[k]);
	}

	flush_dcache_range(HD_ADDR_BIT_PARA, HD_ADDR_BIT_PARA+PARA_BUF_SIZE);
	}

	VpuWriteReg(BIT_BUSY_FLAG, 0x1);// set busy.sheen
	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
	VpuWriteReg(BIT_RUN_INDEX, 0x0); 	// BIT_RUN_INDEX
	VpuWriteReg(BIT_RUN_COD_STD, 0x0); 	// BIT_RUN_COD_STD, H.264
	VpuWriteReg(BIT_RUN_AUX_STD, 0x0); 	// BIT_RUN_AUX_STD
	VpuWriteReg(BIT_FRM_DIS_FLG, 0x0);  // BIT_FRM_DIS_FLG

	VpuWriteReg(BIT_RUN_COMMAND, 0x4); // BIT_RUN_COMMAND: SET_FRAME_BUF

	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);   // BIT_BUSY_FLAG = 0

	// decode processing
	while(1) {
		//ret= VpuReadReg(BIT_BIT_STREAM_PARAM);
		debug_printf("%s: decoding...%d\n", __func__, dec_num);
		//VpuWriteReg(BIT_BIT_STREAM_PARAM, 0);  // BIT_BIT_STREAM_PARAM
		//VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);	  // BIT_WORK_BUF_ADDR
		/*
		if(dec_num==0 || dec_num==1){
			for(k=0;k<0x200;k+=4)
			debug_printf("0x%03X: 0x%08X\n",k,VpuReadReg(k));
		}*/

		//for BIT_RUN_COMMAND: PIC_RUN
		VpuWriteReg(CMD_DEC_PIC_OPTION, 0x0); //I search,B skip,user data report...sheen
		VpuWriteReg(BIT_BASE+0x198, 0x0); //CMD_DEC_FRAME_SKIP_NUM .sheen
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE, 0x0); //rotation and mirroring.sheen

		VpuWriteReg(BIT_BUSY_FLAG, 0x1);// set busy.sheen
		VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
		VpuWriteReg(BIT_RUN_INDEX, 0x0); //instance or process index.sheen
		VpuWriteReg(BIT_RUN_COD_STD, 0x0); //codec index, H.264 =0
		VpuWriteReg(BIT_RUN_AUX_STD, 0x0); //auxiliary codec index. H.264/AVC =0

		VpuWriteReg(BIT_RUN_COMMAND , 0x3); // BIT_RUN_COMMAND :PIC_RUN

		// decode 1 frame complete
		// Wait until busyFlag==0
		while (VpuReadReg(BIT_BUSY_FLAG ) == 1);	// BIT_BUSY_FLAG = 0

		// Read output information
		//REG32(0x1000000c)= VpuReadReg(BIT_BASE+0x1c4);   // Display frame index
		DispFrameIdx = VpuReadReg(RET_DEC_PIC_DISPLAY_IDX);   // Display frame index
		//REG32(0x10000010) = VpuReadReg(BIT_BASE+0x1dc);   // Decoded frame index
		DecDecFrameIdx = VpuReadReg(RET_DEC_PIC_DECODED_IDX);

		DecPicType = VpuReadReg(RET_DEC_PIC_TYPE) & 7;   // REC_DEC_PIC_TYPE
		ret= VpuReadReg(RET_DEC_PIC_SUCCESS);//PIC_RUN result
		PicSize= VpuReadReg(BIT_RD_PTR)-data; //frame bytes size
		data= VpuReadReg(BIT_RD_PTR);//cur bitstream addr

		debug_printf("%s:ret 0x%x type %d disIDX %d decIDX %d BIT_RD_PTR 0x%x frmSz %d\n", __func__, ret, DecPicType,DispFrameIdx, DecDecFrameIdx,data,PicSize);

		if(ret != 1 || PicSize <=0){
			debug_printf("%s: One frame dec err!\n", __func__);
			break;
		}

		if (DispFrameIdx == -1) // -1, no output ,end of the sequence.
		{
			// Decode successful
			debug_printf("%s: EOS 1.\n", __func__);
			break;

		} else if (DispFrameIdx >=0 /*DispFrameIdx != -3 && DispFrameIdx != -2*/) { // -2,-3, no display frame
			// Host handle output picture, such as display, then clear display flag
			// For example, if host wants to display index0 frame, the Y, CB, CR addresses are // located in DpbLum[0]), DpbCb[0], DpbCr[0];
			//  By calling VpuWriteReg(BIT_BASE+0x150, 0x0) to clear display flag of index0 after displaying index0 frame
			//32 bits, each bit match one frame index.sheen
			DispFrameIdx = VpuReadReg(BIT_FRM_DIS_FLG) & (~(1<<DispFrameIdx));

			VpuWriteReg(BIT_FRM_DIS_FLG, DispFrameIdx);
		}

		//check md5
		if(md5_check && DecDecFrameIdx>=0){
			flush_dcache_range(DpbLum[DecDecFrameIdx],DpbLum[DecDecFrameIdx]+sizeX*sizeY*3/2);
			md5((unsigned char*)DpbLum[DecDecFrameIdx], sizeX*sizeY*3/2, md5Data);

			debug_printf("md5 digest[0-7]=[0x%x %x %x %x %x %x %x %x]\n",md5Data[0],md5Data[1],md5Data[2],md5Data[3],md5Data[4],md5Data[5],md5Data[6],md5Data[7]);
			debug_printf("md5 digest[8-15]=[0x%x %x %x %x %x %x %x %x]\n",md5Data[8],md5Data[9],md5Data[10],md5Data[11],md5Data[12],md5Data[13],md5Data[14],md5Data[15]);
			for(k=0;k<16;k++){
				if(md5Data[k]!=yuv_md5_digest[dec_num*16+k]){
					debug_printf("md5 check fail!!!\n");
					return -2;
				}
			}
			debug_printf("md5 check pass!!!\n");
		}


		// copying yuv data to a file
		// You should use your own method to output decoded frames to outside, such as using USB library.
		dec_num++;

		if(dec_num >= FRAME_TEST_NUM )
			break;

		if((data - HD_ADDR_BIT_STREAM) >= (bitStreamSize - 7)){
			debug_printf("%s: EOS 2.\n", __func__);
			break;
		}
	}

	debug_printf("%s: end seq.\n", __func__);
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);// set busy.sheen
	VpuWriteReg(BIT_RUN_COMMAND , 0x2); // BIT_RUN_COMMAND :SEQ_END
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);	// BIT_BUSY_FLAG = 0

	// You can directly output these 32 frame buffers to outside using USB driver.
	printf("%s: test success finish.\n", __func__);
	return 0;
}

