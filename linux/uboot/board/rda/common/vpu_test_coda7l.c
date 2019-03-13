/**********************************
vpu test.
ver 1.0	: for CODA7L
It's only for normal vpu decode, the advanced functions like on-the-fly down scale and YUV422 encoder will be test in vpurun.
2015.03.30

ver 1.1
add md5 check.
2015.09.29

sheen
***********************************/

#include "coda7l_regs.h" //sync with cnm-coda-sw-pkg within android project.
//vpu firmware
//Falcon 94208*2Bytes
#include "coda7l_fw.h"  //sync with cnm-coda-sw-pkg within android project.

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
//coda7l and coda960 decode yuv is same.
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
#define BIT_REG_BASE		0x20830000 //CODA7L 8810E
#define BIT_REG_SIZE		0x4000

//second AXI base addr.
//80k SECOND AXI ACCCESS MEMORY ON CHIP //8810E CODA7L
//VC1 HD decoder MAX need 0x17D00=96k
#define	HD_SEC_AXI_BASE_ADDR	0x100000
#define	INTERNAL_SRAM_SIZE	0x1C000 //80k

//external sdram base addr. sheen
// FIRST AXI ACCCESS SDRAM
//#define	HD_BASE_ADDR					0x8db00000
#define	HD_BASE_ADDR					0x88000000  //minimum reserve 128M for 32 frames 1080P

// Base address of the bitstream buffer. for the whole bitstream.
#define	HD_ADDR_BIT_STREAM				(HD_BASE_ADDR + 0x000000)
// Size of the bitstream buffer in byte,(1M bytes)
#define	HD_STREAM_BUF_SIZE				0x100000
// Base address for the Falcon.h firmware image, (firmware size 94208x2)
#define	HD_ADDR_BIT_CODE				(HD_ADDR_BIT_STREAM + HD_STREAM_BUF_SIZE)
//bitcode buf size 260KB
#define CODE_BUF_SIZE	  (248*1024)//for CODA7L
// Base address for the firmware common parameters buffer.
//1.record decode frame buf YUV addr,384= 4*3*MAX32, 2.record MvColBuf.sheen
#define	HD_ADDR_BIT_PARA				(HD_ADDR_BIT_CODE + CODE_BUF_SIZE)
//common parameters buffer size 10KB
#define PARA_BUF_SIZE	 (10*1024) //CODA7L
// Base address for the firmware common working buffer.
#define	HD_ADDR_BIT_WORK				(HD_ADDR_BIT_PARA + PARA_BUF_SIZE)
//common work buffer size.
//WORK_BUF_SIZE= (512*1024) + (MAX_NUM_INSTANCE*48*1024)  + MINI_PIPP    EN_SCALER_COEFFCIENT_ARRAY_SIZE= 512k+48k+256=564k
#define WORK_BUF_SIZE	  (564*1024) //CODA7L
// Slice Buffer of decoder. max as frame buffer.
#define HD_ADDR_SLICE_BUFFER			(HD_ADDR_BIT_WORK + WORK_BUF_SIZE)
//max slice save buffer size. max 1920*1088*1.5
#define SLICE_SAVE_SIZE				 (1920*1088*3/4)
//SPS/PPS save buffer. Parameter Set Buffer for H.264 decoder
#define HD_ADDR_PS_SAVE_BUFFER			(HD_ADDR_SLICE_BUFFER + SLICE_SAVE_SIZE)
//SPS/PPS save buffer size. CODA7L 512KB.
#define PS_SAVE_SIZE				(512*1024)
// Base address of the DPB.yuv frame buffer.
#define HD_ADDR_FRAME_BASE				(HD_ADDR_PS_SAVE_BUFFER	+ PS_SAVE_SIZE)
//register yuv buf num for decode. MAX=32, sheen
#define HD_REG_FRAME_BUF_NUM   32
// End address of the DPB(128MB-21MB)
//resolution is limited to 1920x1088 (1920x1088x1.75x32=112 MB frame buffer size)
#define HD_MAX_FRAME_BASE				(HD_ADDR_FRAME_BASE + (1920*1088*7/4)*HD_REG_FRAME_BUF_NUM)


/********second AXI addr config***********/
//second AXI SRAM size 0x1C000 (8810 CODA7L)
//max size for 1920x1088(120x68MB) H.264 8810 CODA7L
//H.264 Y Deblocking FIlter buffer, size 120*128= 0x3C00 bytes
#define HD_ADDR_SEC_AXI_DBKY  			(HD_SEC_AXI_BASE_ADDR)
//H.264 C Deblocking FIlter buffer, size 120*128= 0x3C00 bytes
#define HD_ADDR_SEC_AXI_DBKC			(HD_ADDR_SEC_AXI_DBKY + 0x3C00)
//H.264 MB Information, size 120*128= 0x3C00 bytes
#define HD_ADDR_SEC_AXI_BIT 			(HD_ADDR_SEC_AXI_DBKC + 0x3C00)
//H.264 Intra Prediciton buffer,  size 120*64=0x1e00 bytes
#define HD_ADDR_SEC_AXI_IP			(HD_ADDR_SEC_AXI_BIT + 0x3C00)
//VC1 only overlap Filter, size 120*80= 0x2580 bytes
#define HD_ADDR_SEC_AXI_OVL  			(HD_ADDR_SEC_AXI_IP + 0x1E00)
//VC1 only BIT PLANE, size 0xF00  bytes
#define HD_ADDR_SEC_AXI_BTP  			(HD_ADDR_SEC_AXI_OVL + 0x2580)
//CODA7L add for motion estimation buffers, size 120*16*36+2048=0x11600 bytes. size exceed internal sram, not use it.
#define HD_ADDR_SEC_AXI_ME  			(HD_ADDR_SEC_AXI_OVL + 0xF00)
//CODA7L add for on-the-fly scaler
#define HD_ADDR_SEC_AXI_SCALER
#define HD_ADDR_SEC_AXI_BUF_END  		(HD_SEC_AXI_BASE_ADDR + INTERNAL_SRAM_SIZE)

/************************************************************/

//vpu registers read/write
#define VpuWriteReg(ADDR, DATA)   *((volatile unsigned int *)(ADDR + (unsigned int)BIT_REG_BASE)) = DATA
#define VpuReadReg(ADDR)		*((volatile unsigned int *)(ADDR + (unsigned int)BIT_REG_BASE))

//directly addr read/write
#define MREAD_WORD(ADDR)	*((volatile int *)(ADDR))
#define MWRITE_WORD(ADDR,DATA)	*((volatile int *)(ADDR)) = DATA


static int _vpu_test(int md5_check)
{
	int  i,k;
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

//RE_INIT:
	printf("%s: start\n", __func__);

	//store firmware in sdram and then dma firmware to internal PMEM by vpu. sheen
	dataSize = sizeof(bit_code) / sizeof(bit_code[0]);//94208*2
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
	for(k = 0; k <(64*1024) ; k += 4)
		MWRITE_WORD(BIT_REG_BASE + k,0);

	//32bits index for buf display.
	VpuWriteReg(BIT_FRM_DIS_FLG, 0);

	// Start decoding configuration
	VpuWriteReg(BIT_BASE + 0xffc, 0x01); // enable clk, any other value than 0xA1B2C3D4
	VpuWriteReg(BIT_CODE_RUN, 0x0);//control bit processor
	VpuWriteReg(BIT_INT_ENABLE, 0x0);//Disable interrupt

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
	for(k = 0; k <2048; k++) {
		data = bit_code[k];
		VpuWriteReg(BIT_CODE_DOWN,(k << 16) | data);
	    i=50;
	    while(i--);
	}

	// Initialize the CODA
	//buffer for bit processor command execution argument and return data.
	VpuWriteReg(BIT_PARA_BUF_ADDR, HD_ADDR_BIT_PARA);
	//buffer for firmware code.
	VpuWriteReg(BIT_CODE_BUF_ADDR, HD_ADDR_BIT_CODE);
	//bitstream buffer endian format,0= 64bit little endian
	VpuWriteReg(BIT_BIT_STREAM_CTRL, 0x0);
	//if the whole stream feeded,set bit2=1.
	VpuWriteReg(BIT_BIT_STREAM_PARAM, 0x0);
	//Dpb YUV buffer Endian mode:bit0=0 64bit little endian, bit2=0 CbCr separate format.
	VpuWriteReg(BIT_FRAME_MEM_CTRL , 0x0);
	//disable interrupt.
	VpuWriteReg(BIT_INT_ENABLE, 0x0);
	//7 secAxi enable bit.(disable VC1(Ovl,Btp),ME )
	VpuWriteReg(BIT_AXI_SRAM_USE, 0x0f0f);
	//bit processor busy flag.
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);
	//bit processor code reset.
	VpuWriteReg(BIT_CODE_RESET, 0x1);
	//start vpu.
	VpuWriteReg(BIT_CODE_RUN , 0x1);
	while (VpuReadReg(BIT_BUSY_FLAG) == 1);

	debug_printf("%s:(VPU_Init,InitializeVPU) load base code and init vpu done.CUR_PC=%x\n", __func__, VpuReadReg(BIT_CUR_PC));

	//bit stream buffer read ptr.
	VpuWriteReg(BIT_RD_PTR, HD_ADDR_BIT_STREAM);
	//maximum is 15M, this value is for real bitstream write point position
	VpuWriteReg(BIT_WR_PTR, HD_ADDR_BIT_STREAM+HD_STREAM_BUF_SIZE);
	//(bs input mode) If all the streams are feeded, the value is 4 , in demo stream, all the stream is feeded, should not exceed 15M
	VpuWriteReg(BIT_BIT_STREAM_PARAM , 4);
	//common working buffer
	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
	// wait for BIT_BUSY_FLAG = 0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);
	//current bitstream read address.(512bytes increased each time)
	data= VpuReadReg(BIT_RD_PTR);

	debug_printf("%s:set bit stream buf done. BIT_RD_PTR 0x%x\n", __func__,data);

	//for BIT_RUN_COMMAND: SEQ_INIT.
	//(BIT_BASE+0x180... reuse). sheen
	//seq Bitstream buffer SDRAM byte address.(align with mem bus width)
	VpuWriteReg(CMD_DEC_SEQ_BB_START, HD_ADDR_BIT_STREAM);
	//seq Bitstream buffer size in kilo bytes count
	VpuWriteReg(CMD_DEC_SEQ_BB_SIZE, HD_STREAM_BUF_SIZE/1024);
	//valid seq stream offset from start addr.(only valid in SEQ_INIT).
	VpuWriteReg(CMD_DEC_SEQ_START_BYTE, 0x0);
	//bit[1],enable display buffer reordering.(for B frame, will make delay)
	VpuWriteReg(CMD_DEC_SEQ_OPTION, 0x2);
	//buffer for saving sps/pps info.(parameter sets)
	VpuWriteReg(CMD_DEC_SEQ_PS_BB_START, HD_ADDR_PS_SAVE_BUFFER);
	//ps buffer size.
	VpuWriteReg(CMD_DEC_SEQ_PS_BB_SIZE, PS_SAVE_SIZE/1024);
	//support x264.
	VpuWriteReg(CMD_DEC_SEQ_X264_MV_EN, 0x1);

	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);   // BIT_BUSY_FLAG = 0

	debug_printf("%s: seq dec set done.\n", __func__);

	// set busy.
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);
	//common work buffer.
	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
	//vpu instance index.
	VpuWriteReg(BIT_RUN_INDEX, 0x0);
	//codec standard:0= H.264 DECODER
	VpuWriteReg(BIT_RUN_COD_STD, 0x0);
	//auxiliary codec standard index.(exp.when COD_STD MPEG4,0=MPEG4,1=DivX3)
	VpuWriteReg(BIT_RUN_AUX_STD, 0x0);
	//run vpu command. 1=SEQ_INIT,decode sps and report sequence header info.
	VpuWriteReg(BIT_RUN_COMMAND, 0x1);

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!
	need coolwatch write BUSY_FLAG to 0 when the first time hang after power on.
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG) == 1);

	//cur bitstream addr after sps decode.
	data= VpuReadReg(BIT_RD_PTR);

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
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);

	// Allocate DPB to the IP
	// seqInitStatus for DEC_SEQ_INIT, 0 error, 1 success
	ret = VpuReadReg(RET_DEC_SEQ_SUCCESS);
	debug_printf("%s: seqInitStatus = %d\n", __func__, ret);
	if (ret == 1) {
	    //bit[31:16],pic width
		sizeX = VpuReadReg(RET_DEC_SEQ_SRC_SIZE) >> 16;
	    //bit[15:0],pic height
		sizeY = VpuReadReg(RET_DEC_SEQ_SRC_SIZE) & 0xffff;
		//minimum frame buffer number to decode.
		FrameBufNum = VpuReadReg(RET_DEC_SEQ_FRAME_NEED);
		if(FrameBufNum > HD_REG_FRAME_BUF_NUM)
			FrameBufNum = HD_REG_FRAME_BUF_NUM;
		debug_printf("%s: minimal request of frame buf num= %d\n", __func__, FrameBufNum);
		//for BIT_RUN_COMMAND: SET_FRAME_BUF
		// Number of frames used for reference or output reodering.
		VpuWriteReg(CMD_SET_FRAME_BUF_NUM, FrameBufNum);

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
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);

	VpuWriteReg(CMD_SET_FRAME_AXI_BIT_ADDR, HD_ADDR_SEC_AXI_BIT);//second AXI address,ADDR_AXI_BIT
	VpuWriteReg(CMD_SET_FRAME_AXI_IPACDC_ADDR, HD_ADDR_SEC_AXI_IP);//second AXI address,ADDR_AXI_IP
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKY_ADDR, HD_ADDR_SEC_AXI_DBKY);//second AXI address,ADDR_AXI_DBKY
	VpuWriteReg(CMD_SET_FRAME_AXI_DBKC_ADDR, HD_ADDR_SEC_AXI_DBKC);//second AXI address,ADDR_AXI_DBKC
#if 0 //for VC1 only
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR, HD_ADDR_SEC_AXI_OVL);//second AXI address,ADDR_AXI_OVL
	VpuWriteReg(CMD_SET_FRAME_AXI_BTP_ADDR, HD_ADDR_SEC_AXI_BTP);//second AXI address,ADDR_AXI_BTP
#else
	VpuWriteReg(CMD_SET_FRAME_AXI_OVL_ADDR, 0); //second AXI address,ADDR_AXI_OVL
	//VpuWriteReg(CMD_SET_FRAME_AXI_BTP_ADDR, 0); //second AXI address,ADDR_AXI_BTP
#endif
	//VpuWriteReg(CMD_SET_FRAME_CACHE_CONFIG, 0x7e0);//2D CACHE CONFIG
	//buffer for saving SLICE RBSP.
	VpuWriteReg(CMD_SET_FRAME_SLICE_BB_START, HD_ADDR_SLICE_BUFFER);
	//SILCE RBSP buffer size in kilo bytes.
	VpuWriteReg(CMD_SET_FRAME_SLICE_BB_SIZE, SLICE_SAVE_SIZE/1024);

	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);

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
		debug_printf("dump addr yuv[%d]:0x%x size %d\n",k,DpbLum[k],PicSize*3/2 );
	}

	// registering the base addresses of created frame buffers, little endian is ok?
	for (k=0; k < FrameBufNum; k = k + 2) {
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12)	  , DpbCb[k]   );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) +  4, DpbLum[k]  );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) +  8, DpbLum[k+1]);
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 12, DpbCr[k]   );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 16, DpbCr[k+1] );
		MWRITE_WORD(HD_ADDR_BIT_PARA + (k*12) + 20, DpbCb[k+1] );
	}

	//mvCol buf
	for (k=0; k < FrameBufNum; k = k + 2) {
		MWRITE_WORD(HD_ADDR_BIT_PARA + 384+(k*4)	 , MvColBuf[k+1]);
		MWRITE_WORD(HD_ADDR_BIT_PARA + 384+(k*4) +  4, MvColBuf[k]);

	}

	flush_dcache_range(HD_ADDR_BIT_PARA, HD_ADDR_BIT_PARA+PARA_BUF_SIZE);

	}
	// set busy flag.
	VpuWriteReg(BIT_BUSY_FLAG, 0x1);
	VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
	//vpu run instance index.
	VpuWriteReg(BIT_RUN_INDEX, 0x0);
	//codec standard index.0= H.264
	VpuWriteReg(BIT_RUN_COD_STD, 0x0);
	//auxiliary codec standard index.
	VpuWriteReg(BIT_RUN_AUX_STD, 0x0);
	//n-th bit means display buffer index.
	VpuWriteReg(BIT_FRM_DIS_FLG, 0x0);
	// BIT_RUN_COMMAND: SET_FRAME_BUF= 0x4
	VpuWriteReg(BIT_RUN_COMMAND, 0x4);

	// Wait until busyFlag==0
	while (VpuReadReg(BIT_BUSY_FLAG ) == 1);

	// decode processing
	while(1) {
		debug_printf("%s: decoding...%d\n", __func__, dec_num);
		/*
		if(dec_num==0 || dec_num==1){
			for(k=0;k<0x200;k+=4)
			debug_printf("0x%03X: 0x%08X\n",k,VpuReadReg(k));
		}*/

		//for BIT_RUN_COMMAND: PIC_RUN
		//n-th bit means: preScan,I search,B skip,user data report...
		VpuWriteReg(CMD_DEC_PIC_OPTION, 0x0);
		//CMD_DEC_PIC_SKIP_NUM:the number of frame decoder skips.
		VpuWriteReg(BIT_BASE+0x198, 0x0);
		//rotation and mirroring.
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE, 0x0);
		// set busy flag
		VpuWriteReg(BIT_BUSY_FLAG, 0x1);
		VpuWriteReg(BIT_WORK_BUF_ADDR, HD_ADDR_BIT_WORK);
		//vpu run instance index.
		VpuWriteReg(BIT_RUN_INDEX, 0x0);
		//codec standard. 0= H.264
		VpuWriteReg(BIT_RUN_COD_STD, 0x0);
		//auxiliary codec index. H.264/AVC =0
		VpuWriteReg(BIT_RUN_AUX_STD, 0x0);
		// BIT_RUN_COMMAND :PIC_RUN
		VpuWriteReg(BIT_RUN_COMMAND , 0x3);

		// decode 1 frame complete
		// Wait until busyFlag==0
		while (VpuReadReg(BIT_BUSY_FLAG ) == 1);

		// Read output information
		// Display frame index
		DispFrameIdx = VpuReadReg(RET_DEC_PIC_DISPLAY_IDX);
		// Decoded frame index
		DecDecFrameIdx = VpuReadReg(RET_DEC_PIC_DECODED_IDX);
		//0=I,1=P,2=B
		DecPicType = VpuReadReg(RET_DEC_PIC_TYPE) & 0xff;
		//PIC_RUN result.0=err in header.1=ok....
		ret= VpuReadReg(RET_DEC_PIC_SUCCESS);
		//frame bytes size
		PicSize= VpuReadReg(BIT_RD_PTR)-data;
		//cur bitstream addr
		data= VpuReadReg(BIT_RD_PTR);

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

