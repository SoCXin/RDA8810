#include <common.h>
#include <asm/arch/rda_iomap.h>

#include "ddr3.h"
#include "tgt_ap_clock_config.h"

#define DDR_TYPE	3
#define DF_DELAY	(0x100000)

#define DMC_REG_BASE RDA_DMC400_BASE
#define PHY_REG_BASE RDA_DDRPHY_BASE

#define  PHY_RDLAT		0
#define  PHYWRDATA		1
#define  STA			3
#define  CLKSEL			4
#define  PSSTART		5
#define  PSDONE			6
#define  LOCKED			6
#define  CTRL_DELAY		7
#define  RDELAY_SEL		8
#define  WDELAY_SEL		9
#define  PHY_RESET		10
#define  RESET_DDR3		11
#define  ODT_DELAY		12
#define  DDR3_USED		13
#define  WRITE_ENABLE_LAT	14
#define  WRITE_DATA_LAT		15
#define  DQOUT_ENABLE_LAT	16
#define  DMC_READY          128
#define  DATA_ODT_ENABLE_REG    20
#define  DATA_WRITE_ENABLE_REG  48   

#define ARRAY_NUM(n) (sizeof(n))/(sizeof(n[0]))

#define DMCREG_BADDR DMC_REG_BASE
#define PHY_BADDR PHY_REG_BASE

void DDR_CONFIG_DELAY(int delay)
{
    int i;
    i = delay;
    while(i--);
}

void config_ddr_phy(UINT16 flag)
{
    int cnt;
    volatile int * addr;
    cnt = 16;
    while(cnt--);
        
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (PHY_BADDR + DMC_READY*4);
    *addr = 0;    
    
    addr = (int *) (PHY_BADDR + RESET_DDR3*4);
    *addr = 1;

    addr = (int *) (PHY_BADDR + WRITE_DATA_LAT*4);
    *addr = 1;    
    
    addr = (int *) (PHY_BADDR + DDR3_USED*4);
    *addr = 1;    
   
    addr = (int *) (PHY_BADDR + WDELAY_SEL*4);
    *addr = 3;    
    
    addr = (int *) (PHY_BADDR + DQOUT_ENABLE_LAT*4);
    *addr = 1;

    addr = (int *) (PHY_BADDR + DATA_ODT_ENABLE_REG*4);
    *addr = 0xf;
    
    addr = (int *) (PHY_BADDR + DATA_WRITE_ENABLE_REG*4);
    *addr = 0xf;  
          	
    serial_puts("ddr3 phy init done!\n");  
}

#define DMC_WAIT_IDLE     1

void config_dmc400(UINT16 flag, UINT32 para)
{
    int wait_idle;
    volatile int *addr;
    volatile int temp;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x200);
    *addr = 0x3f;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x204);
    *addr = 0x23008c;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x204);
    *addr = 0x8c008c;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x208);
    *addr = 0x4;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x20c);
    *addr = 0xc;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x218);
    *addr = 0x6;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x21c);
    *addr = 0xf;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x220);
    *addr = 0x6;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x224);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x228);
    *addr = 0x4;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x22c);
    *addr = 0x14;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x234);
    *addr = 0x6;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x238);
    *addr = 0x6;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x23c);
    *addr = 0x4;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x244);
    *addr = 0x10;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x248);
    *addr = 0x4000d;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x248);
    *addr = 0x6000f;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x24c);
    *addr = 0x60000;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x230);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x240);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x300);
    *addr = 0x2;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x304);
    *addr = 0x105;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x304);
    *addr = 0x105;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x258);
    *addr = 0x3;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x25c);
    *addr = 0x20003;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x25c);
    *addr = 0xa0003;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x260);
    *addr = 0x4;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x264);
    *addr = 0x1000090;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x264);
    *addr = 0x2000090;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x268);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x26c);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x250);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x254);
    *addr = 0x5;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x148);
    *addr = 0x0;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x10);
    *addr = 0x30200;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x10);
    *addr = 0x30200;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x10);
    *addr = 0x30200;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x18);
    //*addr = 0x22000201;
    *addr = 0x22000202;

    wait_idle = DMC_WAIT_IDLE * 0x1d;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x10);
    *addr = 0x30200;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x10);
    //*addr = 0x30401;
    *addr = 0x30302;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x14);
    *addr = 0x50;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x14);
    //*addr = 0x10;
    *addr = 0x00;

    wait_idle = DMC_WAIT_IDLE * 0x6;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x0;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x10020008;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x10030000;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x10010001;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x10000520;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x50000400;

    wait_idle = DMC_WAIT_IDLE;
    while(wait_idle--);
    DDR_CONFIG_DELAY(DF_DELAY); 
    addr = (int *) (DMCREG_BADDR + 0x108);
    *addr = 0x30000000;

    wait_idle = DMC_WAIT_IDLE * 0x6;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x148);
    *addr = 0x0;

    wait_idle = DMC_WAIT_IDLE * 0x8;
    while(wait_idle--);
    addr = (int *) (DMCREG_BADDR + 0x8);
    *addr = 0x3;

    addr = (int * ) (DMCREG_BADDR + 0x0);
    temp = 0;
    while (temp != 3)
        temp = *addr;

    serial_puts("ddr3 dmc400 init done!\n");
}

void axi_prio_init(void)
{
    *(volatile UINT32*)(0x20900100) = 0xaa0000aa;
    *(volatile UINT32*)(0x21054100) = 0x00000008;
    *(volatile UINT32*)(0x21054104) = 0x00000008;
}

void axi_outstandings_init(void)
{
    *(volatile UINT32*)(0x2105110C) = 0x00000060;
    *(volatile UINT32*)(0x21051110) = 0x00000300;
}

int ddr_init(UINT16 flags, UINT32 para)
{
    UINT16 dll_off;
    UINT32 mem_width;

    axi_prio_init();
    axi_outstandings_init();

    dll_off = flags & DDR_FLAGS_DLLOFF;
    mem_width = (para & DDR_PARA_MEM_BITS_MASK) >> DDR_PARA_MEM_BITS_SHIFT;

    switch (mem_width) {
    case 0:
        serial_puts("8bit ");
        break;
    case 1:
        serial_puts("16bit ");
        break;
    case 2:
        serial_puts("32bit ");
        break;
    }
    if (dll_off)
        serial_puts("dll-off Mode ...\n");
    else
        serial_puts("dll-on Mode ...\n");

    config_ddr_phy(flags);
    config_dmc400(flags, para);
    //serial_puts("dram ");
    //print_u32(DDR_TYPE);
    //serial_puts(" init done ...\n");
    printf("ddr%d init done\n");

    return 0;
}
