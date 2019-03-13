#ifndef _SDMMC_H_
#define _SDMMC_H_

#include <asm/arch/hardware.h>

// =============================================================================
// TYPES 
// =============================================================================

typedef struct
{
    u8   mid;
    u32  oid;
    u32  pnm1;
    u8   pnm2;
    u8   prv;
    u32  psn;
    u32  mdt;
    u8   crc;
} MCD_CID_FORMAT_T;


// =============================================================================
// MCD_CARD_STATE_T
// -----------------------------------------------------------------------------
/// The state of the card when receiving the command. If the command execution 
/// causes a state change, it will be visible to the host in the response to 
/// the next command. The four bits are interpreted as a binary coded number 
/// between 0 and 15.
// =============================================================================
typedef enum
{
    MCD_CARD_STATE_IDLE    = 0,
    MCD_CARD_STATE_READY   = 1,
    MCD_CARD_STATE_IDENT   = 2,
    MCD_CARD_STATE_STBY    = 3,
    MCD_CARD_STATE_TRAN    = 4,
    MCD_CARD_STATE_DATA    = 5,
    MCD_CARD_STATE_RCV     = 6,
    MCD_CARD_STATE_PRG     = 7,
    MCD_CARD_STATE_DIS     = 8
} MCD_CARD_STATE_T;


// =============================================================================
// MCD_CARD_STATUS_T
// -----------------------------------------------------------------------------
/// Card status as returned by R1 reponses (spec V2 pdf p.)
// =============================================================================
typedef union
{
    u32 reg;
    struct
    {
        u32                          :3;
        u32 akeSeqError              :1;
        u32                          :1;
        u32 appCmd                   :1;
        u32                          :2;
        u32 readyForData             :1;
        MCD_CARD_STATE_T currentState   :4;
        u32 eraseReset               :1;
        u32 cardEccDisabled          :1;
        u32 wpEraseSkip              :1;
        u32 csdOverwrite             :1;
        u32                          :2;
        u32 error                    :1;
        u32 ccError                  :1;
        u32 cardEccFailed            :1;
        u32 illegalCommand           :1;
        u32 comCrcError              :1;
        u32 lockUnlockFail           :1;
        u32 cardIsLocked             :1;
        u32 wpViolation              :1;
        u32 eraseParam               :1;
        u32 eraseSeqError            :1;
        u32 blockLenError            :1;
        u32 addressError             :1;
        u32 outOfRange               :1;
    } fields;
} MCD_CARD_STATUS_T;

// =============================================================================
// MCD_CSD_T
// -----------------------------------------------------------------------------
/// This structure contains the fields of the MMC chip's register.
/// For more details, please refer to your MMC specification.
// =============================================================================
typedef struct
{                                   // Ver 2. // Ver 1.0 (if different)   
    u8   csdStructure;           // 127:126
    u8   specVers;                   // 125:122
    u8   taac;                   // 119:112
    u8   nsac;                   // 111:104
    u8   tranSpeed;              // 103:96
    UINT16  ccc;                    //  95:84
    u8   readBlLen;              //  83:80
    BOOL    readBlPartial;          //  79:79
    BOOL    writeBlkMisalign;       //  78:78
    BOOL    readBlkMisalign;        //  77:77
    BOOL    dsrImp;                 //  76:76
    u32  cSize;                  //  69:48 // 73:62
    u8   vddRCurrMin;            //           61:59
    u8   vddRCurrMax;            //           58:56
    u8   vddWCurrMin;             //           55:53
    u8   vddWCurrMax;            //           52:50
    u8   cSizeMult;              //           49:47
    // FIXME 
    u8   eraseBlkEnable;
    u8   eraseGrpSize;           //  ??? 46:42
    // FIXME
    u8   sectorSize;
    u8   eraseGrpMult;           //  ??? 41:37

    u8   wpGrpSize;              //  38:32
    BOOL    wpGrpEnable;            //  31:31
    u8   defaultEcc;             //  30:29
    u8   r2wFactor;              //  28:26
    u8   writeBlLen;             //  25:22
    BOOL    writeBlPartial;         //  21:21
    BOOL    contentProtApp;         //  16:16
    BOOL    fileFormatGrp;          //  15:15
    BOOL    copy;                   //  14:14
    BOOL    permWriteProtect;       //  13:13
    BOOL    tmpWriteProtect;        //  12:12
    u8   fileFormat;             //  11:10
    u8   ecc;                    //   9:8
    u8   crc;                    //   7:1
    /// This field is not from the CSD register.
    /// This is the actual block number.
    u32  blockNumber;
} MCD_CSD_T;


// =============================================================================
// MCD_ERR_T
// -----------------------------------------------------------------------------
/// Type used to describe the error status of the MMC driver.
// =============================================================================
typedef enum
{
    MCD_ERR_NO = 0,
    MCD_ERR_CARD_TIMEOUT = 1,
    MCD_ERR_DMA_BUSY = 3,
    MCD_ERR_CSD = 4,
    MCD_ERR_SPI_BUSY = 5,
    MCD_ERR_BLOCK_LEN = 6,
    MCD_ERR_CARD_NO_RESPONSE,
    MCD_ERR_CARD_RESPONSE_BAD_CRC,
    MCD_ERR_CMD,
    MCD_ERR_UNUSABLE_CARD,
    MCD_ERR_NO_CARD,
    MCD_ERR_NO_HOTPLUG,

    /// A general error value
    MCD_ERR,
} MCD_ERR_T;

// =============================================================================
// MCD_STATUS_T
// -----------------------------------------------------------------------------
/// Status of card
// =============================================================================
typedef enum
{
    // Card present and mcd is open
    MCD_STATUS_OPEN,
    // Card present and mcd is not open
    MCD_STATUS_NOTOPEN_PRESENT,
    // Card not present
    MCD_STATUS_NOTPRESENT,
    // Card removed, still open (please close !)
    MCD_STATUS_OPEN_NOTPRESENT
} MCD_STATUS_T ;

// =============================================================================
// MCD_CARD_SIZE_T
// -----------------------------------------------------------------------------
/// Card size
// =============================================================================
typedef struct
{
    u32 nbBlock;
    u32 blockLen;
} MCD_CARD_SIZE_T ;


// =============================================================================
// MCD_CARD_VER
// -----------------------------------------------------------------------------
/// Card version
// =============================================================================

typedef enum 
{
    MCD_CARD_V1,
    MCD_CARD_V2
}MCD_CARD_VER;


// =============================================================================
// MCD_CARD_ID
// -----------------------------------------------------------------------------
/// Card version
// =============================================================================

typedef enum 
{
    MCD_CARD_ID_0,
    MCD_CARD_ID_1,
    MCD_CARD_ID_NO,
}MCD_CARD_ID;


MCD_ERR_T mcd_Open(MCD_CSD_T* mcdCsd, MCD_CARD_VER mcdVer);
MCD_ERR_T mcd_Close(void);
MCD_ERR_T mcd_Write(u32 startAddr, u8* blockWr, u32 size);
MCD_ERR_T mcd_Read(u32 startAddr, u8* blockRd, u32 size);

#endif /* _SDMMC_H_ */

