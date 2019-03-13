#include <common.h>
#include <part.h>
#include <fat.h>
#include <mmc.h> 

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/reg_mmc.h>
#include <asm/arch/reg_ifc.h>
#include <asm/arch/ifc.h>

#include "rda_mmc.h"

#ifdef DEBUG
#include <assert.h>
#else
#define assert(...)
#endif 

#define MCD_TRACE(...)

// =============================================================================
// HAL_SDMMC_TRANSFER_T
// -----------------------------------------------------------------------------
/// Describe a transfer between the SDmmc module and the SD card
// =============================================================================
typedef struct
{
    /// This address in the system memory
    u8* sysMemAddr;
    /// Address in the SD card
    u8* sdCardAddr;
    /// Quantity of data to transfer, in blocks
    u32 blockNum;
    /// Block size
    u32 blockSize;
    HAL_SDMMC_DIRECTION_T direction;
} HAL_SDMMC_TRANSFER_T;

// =============================================================================
//  Global variables
// =============================================================================
u8 g_halSdmmcWriteCh = HAL_UNKNOWN_CHANNEL;
u8 g_halSdmmcReadCh = HAL_UNKNOWN_CHANNEL;

/// SDMMC clock frequency.
static u32 g_halSdmmcFreq = 200000;

//=============================================================================
// hal_SdmmcUpdateDivider
//-----------------------------------------------------------------------------
/// Update the SDMMC module divider to keep the desired frequency, whatever
/// the system frequency is.
/// @param sysFreq System Frequency currently applied or that will be applied
/// if faster.
//=============================================================================
//static void hal_SdmmcUpdateDivider(HAL_SYS_FREQ_T sysFreq);

//=============================================================================
// hal_SdmmcOpen
//-----------------------------------------------------------------------------
/// Open the SDMMC module. Take a resource.
//=============================================================================
void hal_SdmmcOpen(u8 clk_adj)
{
    // Make sure the last clock is set
    //hal_SdmmcUpdateDivider(hal_SysGetFreq());

    // Take the module out of reset
    //hwp_sysCtrl->REG_DBG     = SYS_CTRL_PROTECT_UNLOCK;
    //hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_SDMMC;
    //hwp_sysCtrl->REG_DBG     = SYS_CTRL_PROTECT_LOCK;

    // We don't use interrupts.
    hwp_sdmmc->SDMMC_INT_MASK = 0x0;
    hwp_sdmmc->SDMMC_MCLK_ADJUST = clk_adj;

    // Take a resource (The idea is to be able to get a 25Mhz clock)
    //hal_SysRequestFreq(HAL_SYS_FREQ_SDMMC, HAL_SYS_FREQ_52M, hal_SdmmcUpdateDivider);
}

//=============================================================================
// hal_SdmmcClose
//-----------------------------------------------------------------------------
/// Close the SDMMC module. Take a resource.
//=============================================================================
void hal_SdmmcClose(void)
{
    // Free a resource
    //hal_SysRequestFreq(HAL_SYS_FREQ_SDMMC, HAL_SYS_FREQ_32K, NULL);

    // Put the module in reset, as its clock is free running.
    //hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    //hwp_sysCtrl->Sys_Rst_Set = SYS_CTRL_SET_RST_SDMMC;
    //hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

//=============================================================================
// hal_SdmmcWakeUp
//-----------------------------------------------------------------------------
/// This function requests a resource of #HAL_SYS_FREQ_52M. 
/// hal_SdmmcSleep() must be called before any other
//=============================================================================
void hal_SdmmcWakeUp(void)
{
    // Take a resource (The idea is to be able to get a 25Mhz clock)
    //hal_SysRequestFreq(HAL_SYS_FREQ_SDMMC, HAL_SYS_FREQ_52M, hal_SdmmcUpdateDivider);
}

//=============================================================================
// hal_SdmmcSleep
//-----------------------------------------------------------------------------
/// This function release the resource to #HAL_SYS_FREQ_32K.
//=============================================================================
void hal_SdmmcSleep(void)
{
    // We just release the resource, because the clock gating in sdmmc controller
    // will disable the clock. We should wait for the clock to be actually disabled
    // but the module does not seam to have a status for that...

    // Free a resource
    //hal_SysRequestFreq(HAL_SYS_FREQ_SDMMC, HAL_SYS_FREQ_32K, NULL);
}

// =============================================================================
// hal_SdmmcSendCmd
// -----------------------------------------------------------------------------
/// Send a command to a SD card plugged to the sdmmc port.
/// @param cmd Command
/// @param arg Argument of the command
/// @param suspend Feature not implemented yet.
// =============================================================================
void hal_SdmmcSendCmd(HAL_SDMMC_CMD_T cmd, u32 arg, BOOL suspend)
{
    u32 configReg = 0;
    //hwp_sdmmc->SDMMC_CONFIG = SDMMC_AUTO_FLAG_EN;
    hwp_sdmmc->SDMMC_CONFIG = 0x00000000;
    
    switch (cmd)
    {
        case HAL_SDMMC_CMD_GO_IDLE_STATE:
            configReg = SDMMC_SDMMC_SENDCMD;
            break;

            
        case HAL_SDMMC_CMD_ALL_SEND_CID:
            configReg = SDMMC_RSP_SEL_R2    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x51
            break;

        case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR:
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x11
            break;

        case HAL_SDMMC_CMD_SEND_IF_COND:
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x11
            break;

        case HAL_SDMMC_CMD_SET_DSR:
            configReg = SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_SELECT_CARD:
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_SEND_CSD                :
            configReg = SDMMC_RSP_SEL_R2    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_STOP_TRANSMISSION        :
            configReg = 0; // TODO
            break;

        case HAL_SDMMC_CMD_SEND_STATUS            :
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_SET_BLOCKLEN            :
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_READ_SINGLE_BLOCK        :
            configReg =  SDMMC_RD_WT_SEL_READ
                       | SDMMC_RD_WT_EN
                       | SDMMC_RSP_SEL_OTHER
                       | SDMMC_RSP_EN
                       | SDMMC_SDMMC_SENDCMD; // 0x111
            break;

        case HAL_SDMMC_CMD_READ_MULT_BLOCK        :
            configReg =  SDMMC_S_M_SEL_MULTIPLE
                       | SDMMC_RD_WT_SEL_READ
                       | SDMMC_RD_WT_EN
                       | SDMMC_RSP_SEL_OTHER
                       | SDMMC_RSP_EN
                       | SDMMC_SDMMC_SENDCMD; // 0x511;
            break;

        case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK    :
            configReg = SDMMC_RD_WT_SEL_WRITE
                       | SDMMC_RD_WT_EN
                       | SDMMC_RSP_SEL_OTHER
                       | SDMMC_RSP_EN
                       | SDMMC_SDMMC_SENDCMD; // 0x311
            break;

        case HAL_SDMMC_CMD_WRITE_MULT_BLOCK        :
            configReg =  SDMMC_S_M_SEL_MULTIPLE
                       | SDMMC_RD_WT_SEL_WRITE
                       | SDMMC_RD_WT_EN
                       | SDMMC_RSP_SEL_OTHER
                       | SDMMC_RSP_EN
                       | SDMMC_SDMMC_SENDCMD; // 0x711
            break;

        case HAL_SDMMC_CMD_APP_CMD                :
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x11
            break;

        case HAL_SDMMC_CMD_SET_BUS_WIDTH            :
        case HAL_SDMMC_CMD_SWITCH:
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x11
            break;

        case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS    :
            configReg =  SDMMC_RD_WT_SEL_READ
                       | SDMMC_RD_WT_EN
                       | SDMMC_RSP_SEL_OTHER
                       | SDMMC_RSP_EN
                       | SDMMC_SDMMC_SENDCMD; // 0x111
            break;

        case HAL_SDMMC_CMD_SET_WR_BLK_COUNT        :
            configReg =  SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x11
            break;

        case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
        case HAL_SDMMC_CMD_SEND_OP_COND:
            configReg =  SDMMC_RSP_SEL_R3    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD; // 0x31
            break;

        default:
            break;
    }

    // TODO Add suspend management
    hwp_sdmmc->SDMMC_CMD_INDEX = SDMMC_COMMAND(cmd);
    hwp_sdmmc->SDMMC_CMD_ARG   = SDMMC_ARGUMENT(arg);
    //hwp_sdmmc->SDMMC_CONFIG    = configReg |SDMMC_AUTO_FLAG_EN;
    hwp_sdmmc->SDMMC_CONFIG    = configReg ;
}

// =============================================================================
// hal_SdmmcNeedResponse
// -----------------------------------------------------------------------------
/// Tells if the given command waits for a reponse.
/// @return \c TRUE if the command needs an answer, \c FALSE otherwise.
// =============================================================================
BOOL hal_SdmmcNeedResponse(HAL_SDMMC_CMD_T cmd)
{
    switch (cmd)
    {
        case HAL_SDMMC_CMD_GO_IDLE_STATE:
        case HAL_SDMMC_CMD_SET_DSR:
        case HAL_SDMMC_CMD_STOP_TRANSMISSION:
            return FALSE;
            break;

            
        case HAL_SDMMC_CMD_ALL_SEND_CID:
        case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR:
        case HAL_SDMMC_CMD_SEND_IF_COND:

        case HAL_SDMMC_CMD_SELECT_CARD:
        case HAL_SDMMC_CMD_SEND_CSD:
        case HAL_SDMMC_CMD_SEND_STATUS:
        case HAL_SDMMC_CMD_SET_BLOCKLEN:
        case HAL_SDMMC_CMD_READ_SINGLE_BLOCK:
        case HAL_SDMMC_CMD_READ_MULT_BLOCK:
        case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK:
        case HAL_SDMMC_CMD_WRITE_MULT_BLOCK:
        case HAL_SDMMC_CMD_APP_CMD:
        case HAL_SDMMC_CMD_SET_BUS_WIDTH:
        case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS:
        case HAL_SDMMC_CMD_SET_WR_BLK_COUNT:
        case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
        case HAL_SDMMC_CMD_SEND_OP_COND:
        case HAL_SDMMC_CMD_SWITCH:
            return TRUE;
            break;
        
        default:
            //assert(FALSE, "Unsupported SDMMC command:%d", cmd);
            // Dummy return, for the compiler to be pleased.
            return FALSE;
            break;
    }
}

// =============================================================================
// hal_SdmmcCmdDone
// -----------------------------------------------------------------------------
/// @return \c TRUE of there is not command pending or being processed.
// =============================================================================
BOOL hal_SdmmcCmdDone(void)
{
    return (!(hwp_sdmmc->SDMMC_STATUS & SDMMC_NOT_SDMMC_OVER));
}

// =============================================================================
// hal_SdmmcGetCardDetectPinLevel
// -----------------------------------------------------------------------------
/// @return \c TRUE if card detect (DAT3) line is high,
///         \c FALSE if the line is low.
/// User must check with SD spec and board pull-up/down resistor to 
/// interpret this value.
// =============================================================================
BOOL hal_SdmmcGetCardDetectPinLevel(void)
{
#if 0
#if (CHIP_ASIC_ID == CHIP_ASIC_ID_GREENSTONE)
    u32               value;

    hal_GpioSetIn(HAL_GPIO_26);
    //IOMUX control is useless, the input pins are not filtered but goes to both
    //SDMMC and GPIO module
    //hwp_extApb->GPIO_Mode &= ~REGS_MODE_PIN_SDMMC_DATA3;
    value = hal_GpioGet(HAL_GPIO_26);
    //hwp_extApb->GPIO_Mode |=  REGS_MODE_PIN_SDMMC_DATA3;

    //HAL_TRACE(TSTDOUT, 0, "----- Value %i\n", value);

    return value != 0;
#else
    assert(FALSE, "TODO: hal_SdmmcGetCardDetectPinLevel not implemented for your chip!");
    return FALSE;
#endif
#endif
    return TRUE;
}

//=============================================================================
// hal_SdmmcUpdateDivider
//-----------------------------------------------------------------------------
/// Update the SDMMC module divider to keep the desired frequency, whatever
/// the system frequency is.
/// @param sysFreq System Frequency currently applied or that will be applied
/// if faster.
//=============================================================================
static void hal_SdmmcUpdateDivider(void)
{
    //if (g_halSdmmcFreq != 0)
    //{
    //    u32 divider = (sysFreq-1)/(2*g_halSdmmcFreq) + 1;
    //    if (divider>1)
    //    {
    //        divider = divider-1;
    //    }

    //    if (divider > 0xFF)
    //    {
    //        divider = 0xFF;
    //    }

    // wngl, use 25M/8 as clock
    hwp_sdmmc->SDMMC_TRANS_SPEED = SDMMC_SDMMC_TRANS_SPEED(8);
    //}
}

// =============================================================================
// hal_SdmmcSetClk
// -----------------------------------------------------------------------------
/// Set the SDMMC clock frequency to something inferior but close to that,
/// taking into account system frequency.
// =============================================================================
void hal_SdmmcSetClk(u32 clock)
{
    // TODO Add assert to stay on supported values ?
    g_halSdmmcFreq = clock;
    
    // Update the divider takes care of the registers configuration
    hal_SdmmcUpdateDivider();
}

// =============================================================================
// hal_SdmmcGetOpStatus
// -----------------------------------------------------------------------------
/// @return The operational status of the SDMMC module.
// =============================================================================
HAL_SDMMC_OP_STATUS_T hal_SdmmcGetOpStatus(void)
{
    return ((HAL_SDMMC_OP_STATUS_T)(u32)hwp_sdmmc->SDMMC_STATUS);
}


// =============================================================================
// hal_SdmmcGetResp
// -----------------------------------------------------------------------------
/// This function is to be used to get the argument of the response of a 
/// command. It is needed to provide the command index and its application 
/// specific status, in order to determine the type of answer expected.
///
/// @param cmd Command to send
/// @param arg Pointer to a four 32 bit word array, where the argument will be 
/// stored. Only the first case is set in case of a response of type R1, R3 or 
/// R6, 4 if it is a R2 response.
/// @param suspend Unsupported
// =============================================================================
void hal_SdmmcGetResp(HAL_SDMMC_CMD_T cmd, u32* arg, BOOL suspend)
{
    // TODO Check in the spec for all the commands response types
    switch (cmd)
    {
        // If they require a response, it is cargoed 
        // with a 32 bit argument.
        case HAL_SDMMC_CMD_GO_IDLE_STATE       : 
        case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR  : 
        case HAL_SDMMC_CMD_SEND_IF_COND        : 
        case HAL_SDMMC_CMD_SET_DSR               : 
        case HAL_SDMMC_CMD_SELECT_CARD           : 
        case HAL_SDMMC_CMD_STOP_TRANSMISSION   : 
        case HAL_SDMMC_CMD_SEND_STATUS           : 
        case HAL_SDMMC_CMD_SET_BLOCKLEN           : 
        case HAL_SDMMC_CMD_READ_SINGLE_BLOCK   : 
        case HAL_SDMMC_CMD_READ_MULT_BLOCK       : 
        case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK  : 
        case HAL_SDMMC_CMD_WRITE_MULT_BLOCK       : 
        case HAL_SDMMC_CMD_APP_CMD               : 
        case HAL_SDMMC_CMD_SET_BUS_WIDTH       : 
        case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS  : 
        case HAL_SDMMC_CMD_SET_WR_BLK_COUNT       :
        case HAL_SDMMC_CMD_MMC_SEND_OP_COND    :
        case HAL_SDMMC_CMD_SEND_OP_COND           :
        case HAL_SDMMC_CMD_SWITCH:
            arg[0] = hwp_sdmmc->SDMMC_RESP_ARG3;
            arg[1] = 0;
            arg[2] = 0;
            arg[3] = 0;
            break;

        // Those response arguments are 128 bits
        case HAL_SDMMC_CMD_ALL_SEND_CID           : 
        case HAL_SDMMC_CMD_SEND_CSD               :
            arg[0] = hwp_sdmmc->SDMMC_RESP_ARG0;
            arg[1] = hwp_sdmmc->SDMMC_RESP_ARG1;
            arg[2] = hwp_sdmmc->SDMMC_RESP_ARG2;
            arg[3] = hwp_sdmmc->SDMMC_RESP_ARG3;
            break;

        default:
            //assert(FALSE, "hal_SdmmcGetResp called with "
            //                  "an unsupported command: %d", cmd);
            break;
    }
}

// =============================================================================
// hal_SdmmcGetResp32BitsArgument
// -----------------------------------------------------------------------------
/// This function is to be used to get the argument of the response of a 
/// command triggerring a response with a 32 bits argument (typically, 
/// R1, R3 or R6).
/// @return 32 bits of arguments of a 48 bits response token
// =============================================================================
u32 hal_SdmmcGetResp32BitsArgument(void)
{
    return hwp_sdmmc->SDMMC_RESP_ARG3;
}

// =============================================================================
// hal_SdmmcGetResp128BitsArgument
// -----------------------------------------------------------------------------
/// This function is to be used to get the argument of the response of a 
/// command triggerring a response with a 128 bits argument (typically, 
/// R2).
/// @param Pointer to a 4 case arrays of 32 bits where the argument of the 
/// function will be stored.
// =============================================================================
void hal_SdmmcGetResp128BitsArgument(u32* buf)
{
    buf[0] = hwp_sdmmc->SDMMC_RESP_ARG0;
    buf[1] = hwp_sdmmc->SDMMC_RESP_ARG1;
    buf[2] = hwp_sdmmc->SDMMC_RESP_ARG2;
    buf[3] = hwp_sdmmc->SDMMC_RESP_ARG3;
}

// =============================================================================
// hal_SdmmcEnterDataTransferMode
// -----------------------------------------------------------------------------
/// Configure the SDMMC module to support data transfers
/// FIXME Find out why we need that out of the transfer functions...
// =============================================================================
void hal_SdmmcEnterDataTransferMode(void)
{
    hwp_sdmmc->SDMMC_CONFIG |= SDMMC_RD_WT_EN;
}

// =============================================================================
// hal_SdmmcSetDataWidth
// -----------------------------------------------------------------------------
/// Set the data bus width
/// @param width Number of line of the SD data bus used.
// =============================================================================
void hal_SdmmcSetDataWidth(HAL_SDMMC_DATA_BUS_WIDTH_T width)
{
    switch (width)
    {
        case HAL_SDMMC_DATA_BUS_WIDTH_1:
            hwp_sdmmc->SDMMC_DATA_WIDTH = 1;
            break;

        case HAL_SDMMC_DATA_BUS_WIDTH_4:
            hwp_sdmmc->SDMMC_DATA_WIDTH = 4;
            break;

        default:
            //assert(FALSE, "hal_SdmmcSetDataWidth(%d) is an invalid width",
            //        width);
            break;
    }
}

// =============================================================================
// hal_SdmmcTransfer
// -----------------------------------------------------------------------------
/// Start the ifc transfer to feed the SDMMC controller fifo.
/// @param transfer Transfer to program.
/// @return HAL_ERR_NO or HAL_ERR_RESOURCE_BUSY.
// =============================================================================
int hal_SdmmcTransfer(HAL_SDMMC_TRANSFER_T* transfer)
{
    u8 channel = 0;
    HAL_IFC_REQUEST_ID_T ifcReq = HAL_IFC_NO_REQWEST;
    u32 length = 0;
    u32 lengthExp = 0;

    //assert((transfer->blockSize>=4) && (transfer->blockSize<=2048),
    //            "Block Length(%d) is invalid!\n", transfer->blockSize);

    length =  transfer->blockSize;
    
    // The block size register 
    while (length != 1)
    {
        length >>= 1;
        lengthExp++;
    }

    // Configure amount of data
    hwp_sdmmc->SDMMC_BLOCK_CNT  = SDMMC_SDMMC_BLOCK_CNT(transfer->blockNum);
    hwp_sdmmc->SDMMC_BLOCK_SIZE = SDMMC_SDMMC_BLOCK_SIZE(lengthExp);

    // Configure Bytes reordering
    hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);

    switch (transfer->direction)
    {
        case HAL_SDMMC_DIRECTION_READ:
            ifcReq = HAL_IFC_SDMMC_RX;
            break;

        case HAL_SDMMC_DIRECTION_WRITE:
            ifcReq = HAL_IFC_SDMMC_TX;
            break;

        default:
            assert(FALSE, "hal_SdmmcTransfer with erroneous %d direction",
                            transfer->direction);
            break;
    }

    channel = hal_IfcTransferStart(ifcReq, transfer->sysMemAddr, 
                                   transfer->blockNum*transfer->blockSize,
                                   HAL_IFC_SIZE_32_MODE_MANUAL);
    if (channel == HAL_UNKNOWN_CHANNEL)
    {
        printf("Chal_SdmmcTransfer error\n");
        return -1;
    }
    else
    {
        // Record Channel used.
        if (transfer->direction == HAL_SDMMC_DIRECTION_READ)
        {
            g_halSdmmcReadCh = channel;
        }
        else
        {
            g_halSdmmcWriteCh = channel;
        }
        return 0;
    }
}

// =============================================================================
// hal_SdmmcTransferDone
// -----------------------------------------------------------------------------
/// Check the end of transfer status.
/// Attention: This means the SDMMC module has finished to transfer data.
/// In case of a read operation, the Ifc will finish its transfer shortly 
/// after. Considering the way this function is used (after reading at least
/// 512 bytes, and flushing cache before releasing the data to the user), and
/// the fifo sizes, this is closely equivalent to the end of the transfer.
/// @return \c TRUE if a transfer is over, \c FALSE otherwise.
// =============================================================================
BOOL hal_SdmmcTransferDone(void)
{
    // The link is not full duplex. We check both the
    // direction, but only one can be in progress at a time.

    if (g_halSdmmcReadCh != HAL_UNKNOWN_CHANNEL)
    {
        // Operation in progress is a read.
        // The SDMMC module itself can know it has finished
        if ((hwp_sdmmc->SDMMC_INT_STATUS & SDMMC_DAT_OVER_INT)
         && (hwp_sysIfc->std_ch[g_halSdmmcReadCh].tc == 0))
        {
            // Transfer is over
            hwp_sdmmc->SDMMC_INT_CLEAR = SDMMC_DAT_OVER_CL;
            hal_IfcChannelRelease(HAL_IFC_SDMMC_RX, g_halSdmmcReadCh);

            // We finished a read
            g_halSdmmcReadCh = HAL_UNKNOWN_CHANNEL;
    
            //  Put the FIFO in reset state.
            hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);

            return TRUE;
        }
    }

    if (g_halSdmmcWriteCh != HAL_UNKNOWN_CHANNEL)
    {
        // Operation in progress is a write.
        // The SDMMC module itself can know it has finished
        if ((hwp_sdmmc->SDMMC_INT_STATUS & SDMMC_DAT_OVER_INT)
         && (hwp_sysIfc->std_ch[g_halSdmmcWriteCh].tc == 0))
        {
            // Transfer is over
            hwp_sdmmc->SDMMC_INT_CLEAR = SDMMC_DAT_OVER_CL;
            hal_IfcChannelRelease(HAL_IFC_SDMMC_TX, g_halSdmmcWriteCh);

            // We finished a write
            g_halSdmmcWriteCh = HAL_UNKNOWN_CHANNEL;
    
            //  Put the FIFO in reset state.
            hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);

            return TRUE;
        }
    }
    
    // there's still data running through a pipe (or no transfer in progress ...)
    return FALSE;
}

// =============================================================================
// hal_SdmmcStopTransfer
// -----------------------------------------------------------------------------
/// Stop the ifc transfer feeding the SDMMC controller fifo.
/// @param transfer Transfer to program.
/// @return #HAL_ERR_NO
// =============================================================================
void hal_SdmmcStopTransfer(HAL_SDMMC_TRANSFER_T* transfer)
{
    // Configure amount of data
    hwp_sdmmc->SDMMC_BLOCK_CNT  = SDMMC_SDMMC_BLOCK_CNT(0);
    hwp_sdmmc->SDMMC_BLOCK_SIZE = SDMMC_SDMMC_BLOCK_SIZE(0);
    
    //  Put the FIFO in reset state.
    hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);

    if (transfer->direction == HAL_SDMMC_DIRECTION_READ)
    {
        hal_IfcChannelFlush(HAL_IFC_SDMMC_RX, g_halSdmmcReadCh);
        while(!hal_IfcChannelIsFifoEmpty(HAL_IFC_SDMMC_RX, g_halSdmmcReadCh));
        hal_IfcChannelRelease(HAL_IFC_SDMMC_RX, g_halSdmmcReadCh);
        g_halSdmmcReadCh = HAL_UNKNOWN_CHANNEL;
    }
    else
    {
        hal_IfcChannelFlush(HAL_IFC_SDMMC_TX, g_halSdmmcWriteCh);
        while(!hal_IfcChannelIsFifoEmpty(HAL_IFC_SDMMC_TX, g_halSdmmcReadCh));
        hal_IfcChannelRelease(HAL_IFC_SDMMC_TX, g_halSdmmcWriteCh);
        g_halSdmmcWriteCh = HAL_UNKNOWN_CHANNEL;
    }
}

// =============================================================================
// hal_SdmmcIsBusy
// -----------------------------------------------------------------------------
/// Check if the SD/MMC is busy.
/// 
/// @return \c TRUE if the SD/MMC controller is busy.
///         \c FALSE otherwise.
// =============================================================================
BOOL hal_SdmmcIsBusy(void)
{
    //if (g_halSdmmcReadCh    != HAL_UNKNOWN_CHANNEL
    // || g_halSdmmcWriteCh   != HAL_UNKNOWN_CHANNEL
    // || ((hwp_sdmmc->SDMMC_STATUS & (SDMMC_NOT_SDMMC_OVER | SDMMC_BUSY | SDMMC_DL_BUSY)) != 0)
    //   )
    if ((hwp_sdmmc->SDMMC_STATUS & (SDMMC_NOT_SDMMC_OVER | SDMMC_BUSY | SDMMC_DL_BUSY)) != 0)
    {
        // SD/MMc is busy doing something.
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// =============================================================================
//  MACROS
// =============================================================================
#define BROAD_ADDR  0
#define NOT_SDMMC_OVER  (1<<0)

#define MCD_MAX_BLOCK_NUM       128

#define MCD_SDMMC_OCR_TIMEOUT    (1 SECOND)  // the card is supposed to answer within 1s
                                             //  - Max wait is 128 * 10ms=1,28s
                                             //  TODO: where those 10ms come from ??

// Command 8 things: cf spec Vers.2 section 4.3.13
#define MCD_CMD8_CHECK_PATTERN      0xaa
#define MCD_CMD8_VOLTAGE_SEL        (0x1<<8)
//#define MCD_CMD8_VOLTAGE_SEL        (0x2<<8)   // wngl, change to low voltage
#define MCD_OCR_HCS                 (1<<30)
#define MCD_OCR_CCS_MASK            (1<<30)

#define SECOND                    * CONFIG_SYS_HZ_CLOCK
// Timeouts for V1
#define MCD_CMD_TIMEOUT_V1        ( 1 SECOND / 1 )
#define MCD_RESP_TIMEOUT_V1       ( 1 SECOND / 1 )
#define MCD_TRAN_TIMEOUT_V1       ( 1 SECOND / 1 )
#define MCD_READ_TIMEOUT_V1       ( 5 SECOND )
#define MCD_WRITE_TIMEOUT_V1      ( 5 SECOND )

// Timeouts for V2
#define MCD_CMD_TIMEOUT_V2        ( 1 SECOND / 10 )
#define MCD_RESP_TIMEOUT_V2       ( 1 SECOND / 10 )
#define MCD_TRAN_TIMEOUT_V2       ( 1 SECOND / 10 )
#define MCD_READ_TIMEOUT_V2       ( 5 SECOND )
#define MCD_WRITE_TIMEOUT_V2      (5 SECOND )

// =============================================================================
// Global variables
// =============================================================================

// Spec Ver2 p96
#define MCD_SDMMC_OCR_VOLT_PROF_MASK  0x00ff8000

static u32 g_mcdOcrReg = MCD_SDMMC_OCR_VOLT_PROF_MASK;

/// Relative Card Address Register
/// Nota RCA is sixteen bit long, but is always used
/// as the 16 upper bits of a 32 bits word. Therefore
/// is variable is in fact (RCA<<16), to avoid operation
/// (shift, unshift), to always place the RCA value as the
/// upper bits of a data word.
static u32 g_mcdRcaReg = 0x00000000;

// Driver Stage Register p.118
// (To adjust bus capacitance)
// TODO Tune and put in calibration ?
static u32 g_mcdSdmmcDsr = 0x04040000;


static u32 g_mcdSdmmcFreq = 200000; 

static MCD_CID_FORMAT_T  g_mcdCidReg;
static u32            g_mcdBlockLen   = 0;
static u32            g_mcdNbBlock    = 0;
static BOOL              g_mcdCardIsSdhc = FALSE;

static MCD_STATUS_T      g_mcdStatus = MCD_STATUS_NOTOPEN_PRESENT;

//static CONST TGT_MCD_CONFIG_T*     g_mcdConfig=NULL;
//static MCD_CARD_DETECT_HANDLER_T   g_mcdCardDetectHandler=NULL;

/// Semaphore to ensure proper concurrency of the MCD accesses
/// among all tasks.
static u32           g_mcdSemaphore   = 0xFF;  

/// Current in-progress transfer, if any.
static HAL_SDMMC_TRANSFER_T g_mcdSdmmcTransfer = 
                        {
                            .sysMemAddr = 0,
                            .sdCardAddr = 0,
                            .blockNum   = 0,
                            .blockSize  = 0,
                            .direction  = HAL_SDMMC_DIRECTION_WRITE,
                        };

static MCD_CSD_T g_mcdLatestCsd;

static MCD_CARD_VER g_mcdVer = MCD_CARD_V2;

// =============================================================================
// Functions
// =============================================================================
u32 hal_TimGetUpTime(void)
{
    return (u32)get_ticks();
}
// =============================================================================
// Functions for the HAL Driver ?
// =============================================================================
#if 0
/// Macro to easily implement concurrency in the MCD driver.
/// Enter in the 'critical section'.
#define MCD_CS_ENTER \
    if (g_mcdSemaphore != 0xFF)             \
    {                                       \
        sxr_TakeSemaphore(g_mcdSemaphore);  \
    }                                       \
    else                                    \
    {                                       \
        return MCD_ERR;                     \
    }


/// Macro to easily implement concurrency in the MCD driver.
/// Exit in the 'critical section'.
#define MCD_CS_EXIT \
    {                                       \
    sxr_ReleaseSemaphore(g_mcdSemaphore);   \
    }
#endif

MCD_ERR_T mcd_GetCardSize(MCD_CARD_SIZE_T* size)
{
    //MCD_CS_ENTER;

    size->blockLen = g_mcdBlockLen;
    size->nbBlock  = g_mcdNbBlock;

    //MCD_CS_EXIT;
    return MCD_ERR_NO;
}

// Wait for a command to be done or timeouts
static MCD_ERR_T mcd_SdmmcWaitCmdOver(void)
{
    u32 startTime = hal_TimGetUpTime();
    u32 time_out;
    
    time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_CMD_TIMEOUT_V1:MCD_CMD_TIMEOUT_V2;
   
    while(hal_TimGetUpTime() - startTime < time_out && !hal_SdmmcCmdDone());    

    if (!hal_SdmmcCmdDone())
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "SDC Waiting is Time out");
        return MCD_ERR_CARD_TIMEOUT;
    }
    else
    {
        return MCD_ERR_NO;
    }
}

#if 0
/// Update g_mcdStatus
/// @return TRUE is card present (only exact when GPIO is used for card detect.)
static BOOL mcd_CardDetectUpdateStatus(void)
{
    if(NULL == g_mcdConfig)
    {
        g_mcdConfig = tgt_GetMcdConfig();
    }

    if(g_mcdConfig->cardDetectGpio != HAL_GPIO_NONE)
    {
        BOOL gpioState = !!hal_GpioGet(g_mcdConfig->cardDetectGpio);
        // CARD ?
        if(gpioState == g_mcdConfig->gpioCardDetectHigh)
        { // card detected
            switch(g_mcdStatus)
            {
                case MCD_STATUS_OPEN_NOTPRESENT: // wait for the close !
                case MCD_STATUS_OPEN:
                // No change
                break;
                default:
                g_mcdStatus = MCD_STATUS_NOTOPEN_PRESENT;
            }
            return TRUE;
        }
        else
        { // no card
            switch(g_mcdStatus)
            {
                case MCD_STATUS_OPEN_NOTPRESENT:
                case MCD_STATUS_OPEN:
                    g_mcdStatus = MCD_STATUS_OPEN_NOTPRESENT;
                    break;
                default:
                    g_mcdStatus = MCD_STATUS_NOTPRESENT;
            }
            return FALSE;
        }
    }
    // estimated
    switch(g_mcdStatus)
    {
        case MCD_STATUS_OPEN:
        case MCD_STATUS_NOTOPEN_PRESENT:
            return TRUE;
        default:
            return FALSE;
    }
}

static void mcd_CardDetectHandler(void)
{
    BOOL CardPresent = mcd_CardDetectUpdateStatus();

    g_mcdCardDetectHandler(CardPresent);
}

// =============================================================================
// mcd_SetCardDetectHandler
// -----------------------------------------------------------------------------
/// Register a handler for card detection
///
/// @param handler function called when insertion/removal is detected.
// =============================================================================
MCD_ERR_T mcd_SetCardDetectHandler(MCD_CARD_DETECT_HANDLER_T handler)
{
    if(NULL == g_mcdConfig)
    {
        g_mcdConfig = tgt_GetMcdConfig();
    }

    if(g_mcdConfig->cardDetectGpio == HAL_GPIO_NONE)
    {
        return MCD_ERR_NO_HOTPLUG;
    }

    if(NULL != handler)
    {
        HAL_GPIO_CFG_T cfg  = 
            {
            .direction      = HAL_GPIO_DIRECTION_INPUT,
            .irqMask        = 
                {
                .rising     = TRUE,
                .falling    = TRUE,
                .debounce   = TRUE,
                .level      = FALSE
                },
            .irqHandler     = mcd_CardDetectHandler
            };

        hal_GpioOpen(g_mcdConfig->cardDetectGpio, &cfg);
        g_mcdCardDetectHandler = handler;
    }
    else
    {
        hal_GpioClose(g_mcdConfig->cardDetectGpio);
        g_mcdCardDetectHandler = NULL;
    }
    
    return MCD_ERR_NO;
}

// =============================================================================
// mcd_CardStatus
// -----------------------------------------------------------------------------
/// Return the card status
///
/// @return Card status see #MCD_STATUS_T
// =============================================================================
MCD_STATUS_T mcd_CardStatus(void)
{
    mcd_CardDetectUpdateStatus();
    return g_mcdStatus;
}
#endif

// =============================================================================
// mcd_SdmmcWaitResp
// -----------------------------------------------------------------------------
/// Wait for a response for a time configured by MCD_RESP_TIMEOUT
/// @return MCD_ERR_NO if a response with a good crc was received,
///         MCD_ERR_CARD_NO_RESPONSE if no response was received within the 
/// driver configured timeout.
//          MCD_ERR_CARD_RESPONSE_BAD_CRC if the received response presented
//  a bad CRC.
// =============================================================================
static MCD_ERR_T mcd_SdmmcWaitResp(void)
{
    HAL_SDMMC_OP_STATUS_T status = hal_SdmmcGetOpStatus();
    u32 startTime = hal_TimGetUpTime(); 
    u32 rsp_time_out;
    
    rsp_time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_RESP_TIMEOUT_V1:MCD_RESP_TIMEOUT_V2;
    
    while(hal_TimGetUpTime() - startTime < rsp_time_out &&status.fields.noResponseReceived)
    {
        status = hal_SdmmcGetOpStatus();
    }

    if (status.fields.noResponseReceived)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Response is Time out");
        return MCD_ERR_CARD_NO_RESPONSE;
    }

    if(status.fields.responseCrcError)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Response CRC is Error");
        return MCD_ERR_CARD_RESPONSE_BAD_CRC;
    }

    return MCD_ERR_NO;
}

//=============================================================================
// mcd_SdmmcReadCheckCrc
//-----------------------------------------------------------------------------
/// Check the read state of the sdmmc card.
/// @return MCD_ERR_NO if the Crc of read data was correct, 
/// MCD_ERR_CARD_RESPONSE_BAD_CRC otherwise.
//=============================================================================
static MCD_ERR_T mcd_SdmmcReadCheckCrc(void)
{  
    HAL_SDMMC_OP_STATUS_T operationStatus;
    operationStatus = hal_SdmmcGetOpStatus();
    if (operationStatus.fields.dataError != 0) // 0 means no CRC error during transmission
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "sdC status:%0x", operationStatus.reg);
        return MCD_ERR_CARD_RESPONSE_BAD_CRC;
    }
    else
    {
        return MCD_ERR_NO;
    }
}

//=============================================================================
// mcd_SdmmcWriteCheckCrc
//-----------------------------------------------------------------------------
/// Check the crc state of the write operation of the sdmmc card.
/// @return MCD_ERR_NO if the Crc of read data was correct, 
/// MCD_ERR_CARD_RESPONSE_BAD_CRC otherwise.
//=============================================================================
static MCD_ERR_T mcd_SdmmcWriteCheckCrc(void)
{
    HAL_SDMMC_OP_STATUS_T operationStatus;
    operationStatus = hal_SdmmcGetOpStatus();

    if (operationStatus.fields.crcStatus != 2) // 0b010 = transmissionRight TODO a macro ?
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "sdC status:%0x", operationStatus.reg);
        return MCD_ERR_CARD_RESPONSE_BAD_CRC;
    }
    else
    {
        return MCD_ERR_NO;
    }
}

// =============================================================================
// mcd_SdmmcSendCmd
// -----------------------------------------------------------------------------
/// Send a command to the card, and fetch the response if one is expected.
/// @param cmd CMD to send
/// @param arg Argument of the ACMD.
/// @param resp Buffer to store card response.
/// @param suspend Not supported.
/// @return MCD_ERR_NO if a response with a good crc was received,
///         MCD_ERR_CARD_NO_RESPONSE if no reponse was received within the 
/// driver configured timeout.
///          MCD_ERR_CARD_RESPONSE_BAD_CRC if the received response presented
///  a bad CRC.
///         MCD_ERR_CARD_TIMEOUT if the card timedout during procedure.
// =============================================================================
static MCD_ERR_T mcd_SdmmcSendCmd(HAL_SDMMC_CMD_T cmd, u32 arg,
                                  u32* resp, BOOL suspend)
{
    MCD_ERR_T errStatus = MCD_ERR_NO;
    MCD_CARD_STATUS_T cardStatus = {0};
    u32  cmd55Response[4] = {0, 0, 0, 0};
    if (cmd & HAL_SDMMC_ACMD_SEL)
    {
        // This is an application specific command, 
        // we send the CMD55 first
        hal_SdmmcSendCmd(HAL_SDMMC_CMD_APP_CMD, g_mcdRcaReg, FALSE);
        
        // Wait for command over
        if (MCD_ERR_CARD_TIMEOUT == mcd_SdmmcWaitCmdOver())
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "Cmd55 Send is Time out");
            return MCD_ERR_CARD_TIMEOUT;
        }
        
        // Wait for response
        if (hal_SdmmcNeedResponse(HAL_SDMMC_CMD_APP_CMD))
        {
            errStatus = mcd_SdmmcWaitResp();
        }
        if (MCD_ERR_NO != errStatus)
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "cmd55 response error");
            return errStatus;
        }

        // Fetch response
        hal_SdmmcGetResp(HAL_SDMMC_CMD_APP_CMD, cmd55Response, FALSE);

        cardStatus.reg = cmd55Response[0];
        if(HAL_SDMMC_CMD_SEND_OP_COND == cmd) // for some special card
        {
           //if (!(cardStatus.fields.readyForData) || !(cardStatus.fields.appCmd))
           if (!(cardStatus.fields.appCmd))
            {
                MCD_TRACE(MCD_INFO_TRC, 0, "cmd55(acmd41) status=%0x", cardStatus.reg);
                return MCD_ERR;
            }
        }
        else
        {
            if (!(cardStatus.fields.readyForData) || !(cardStatus.fields.appCmd))
            {
                MCD_TRACE(MCD_INFO_TRC, 0, "cmd55 status=%0x", cardStatus.reg);
                return MCD_ERR;
            }

        }
    }

    // Send proper command. If it was an ACMD, the CMD55 have just been sent.
    hal_SdmmcSendCmd(cmd, arg, suspend);

    // Wait for command to be sent 
    errStatus = mcd_SdmmcWaitCmdOver();
    

    if (MCD_ERR_CARD_TIMEOUT == errStatus)
    {
        if (cmd & HAL_SDMMC_ACMD_SEL)
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "ACMD %d Sending Timed out", (cmd & HAL_SDMMC_CMD_MASK));
        }
        else
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "CMD %d Sending Timed out", (cmd & HAL_SDMMC_CMD_MASK));
        }
        return MCD_ERR_CARD_TIMEOUT;
    }

    // Wait for response and get its argument
    if (hal_SdmmcNeedResponse(cmd))
    {
        errStatus = mcd_SdmmcWaitResp();
    }

    if (MCD_ERR_NO != errStatus)
    {
        if (cmd & HAL_SDMMC_ACMD_SEL)
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "ACMD %d Response Error", (cmd & HAL_SDMMC_CMD_MASK));
        }
        else
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "CMD %d Response Error", (cmd & HAL_SDMMC_CMD_MASK));
            return errStatus;
        }
    }

    // Fetch response
    hal_SdmmcGetResp(cmd, resp, FALSE);

    //FOR DEBUG: MCD_TRACE(MCD_INFO_TRC, 0, "CMD %d Response is %#x", (cmd & HAL_SDMMC_CMD_MASK), resp[0]);

    return MCD_ERR_NO;
}

// =============================================================================
// mcd_SdmmcInitCid
// -----------------------------------------------------------------------------
/// Set the CID in the driver from the data read on the card.
/// @param cid 4 word array read from the card, holding the CID register value.
// =============================================================================
static MCD_ERR_T mcd_SdmmcInitCid(u32* cid)
{
    // Fill the structure with the register bitfields value.
    g_mcdCidReg.mid     = (u8)((cid[3]&(0xff<<24))>>24);
    g_mcdCidReg.oid     = (cid[3]&(0xffff<<8))>>8;
    g_mcdCidReg.pnm2    = (u8)(cid[3]&0xff);
    g_mcdCidReg.pnm1    = cid[2];
    g_mcdCidReg.prv     = (u8)((cid[1]&(0xff<<24))>>24);
    g_mcdCidReg.psn     = (cid[1]&0xffffff)<<8;
    g_mcdCidReg.psn     = g_mcdCidReg.psn|((cid[0]&(0xff<<23))>>23);
    g_mcdCidReg.mdt     = (cid[0]&(0xfff<<7))>>7;
    g_mcdCidReg.crc     = (u8)(cid[0]&0x7f);
    
    return MCD_ERR_NO;
}

#define MCD_CSD_VERSION_1       0
#define MCD_CSD_VERSION_2       1
// =============================================================================
// mcd_SdmmcInitCsd
// -----------------------------------------------------------------------------
/// Fill MCD_CSD_T structure from the array of data read from the card
/// 
/// @param csd Pointer to the structure
/// @param csdRaw Pointer to the raw data.
/// @return MCD_ERR_NO
// =============================================================================
static MCD_ERR_T mcd_SdmmcInitCsd(MCD_CSD_T* csd, u32* csdRaw)
{
    // CF SD spec version2, CSD version 1 ?
    csd->csdStructure       = (u8)((csdRaw[3]&(0x3<<30))>>30);
    
    // Byte 47 to 75 are different depending on the version 
    // of the CSD srtucture.
    csd->specVers           = (u8)((csdRaw[3]&(0xf<26))>>26);
    csd->taac               = (u8)((csdRaw[3]&(0xff<<16))>>16);
    csd->nsac               = (u8)((csdRaw[3]&(0xff<<8))>>8);
    csd->tranSpeed          = (u8)(csdRaw[3]&0xff);

    csd->ccc                = (csdRaw[2]&(0xfff<<20))>>20;
    csd->readBlLen          = (u8)((csdRaw[2]&(0xf<<16))>>16);
    csd->readBlPartial      = (u8)((csdRaw[2]&(0x1<<15))>>15);
    csd->writeBlkMisalign   = (u8)((csdRaw[2]&(0x1<<14))>>14);
    csd->readBlkMisalign    = (u8)((csdRaw[2]&(0x1<<13))>>13);
    csd->dsrImp             = (u8)((csdRaw[2]&(0x1<<12))>>12);

    if (csd->csdStructure == MCD_CSD_VERSION_1)
    {
        csd->cSize              = (csdRaw[2]&0x3ff)<<2;

        csd->cSize              = csd->cSize|((csdRaw[1]&(0x3<<30))>>30);
        csd->vddRCurrMin        = (u8)((csdRaw[1]&(0x7<<27))>>27);
        csd->vddRCurrMax        = (u8)((csdRaw[1]&(0x7<<24))>>24);
        csd->vddWCurrMin        = (u8)((csdRaw[1]&(0x7<<21))>>21);
        csd->vddWCurrMax        = (u8)((csdRaw[1]&(0x7<<18))>>18);
        csd->cSizeMult          = (u8)((csdRaw[1]&(0x7<<15))>>15);
        
        // Block number: cf Spec Version 2 page 103 (116).
        csd->blockNumber        = (csd->cSize + 1)<<(csd->cSizeMult + 2);
    }
    else
    {
        // csd->csdStructure == MCD_CSD_VERSION_2
        csd->cSize = ((csdRaw[2]&0x3f))|((csdRaw[1]&(0xffff<<16))>>16);
        
        // Other fields are undefined --> zeroed
        csd->vddRCurrMin = 0;
        csd->vddRCurrMax = 0;
        csd->vddWCurrMin = 0;
        csd->vddWCurrMax = 0;
        csd->cSizeMult   = 0;
        
        // Block number: cf Spec Version 2 page 109 (122).
        csd->blockNumber        = (csd->cSize + 1) * 1024;
        //should check incompatible size and return MCD_ERR_UNUSABLE_CARD;
    }

    csd->eraseBlkEnable     = (u8)((csdRaw[1]&(0x1<<14))>>14);
    csd->sectorSize         = (u8)((csdRaw[1]&(0x7f<<7))>>7);
    csd->wpGrpSize          = (u8)(csdRaw[1]&0x7f);

    csd->wpGrpEnable        = (u8)((csdRaw[0]&(0x1<<31))>>31);
    csd->r2wFactor          = (u8)((csdRaw[0]&(0x7<<26))>>26);
    csd->writeBlLen         = (u8)((csdRaw[0]&(0xf<<22))>>22);
    csd->writeBlPartial     = (u8)((csdRaw[0]&(0x1<<21))>>21);
    csd->fileFormatGrp      = (u8)((csdRaw[0]&(0x1<<15))>>15);
    csd->copy               = (u8)((csdRaw[0]&(0x1<<14))>>14);
    csd->permWriteProtect   = (u8)((csdRaw[0]&(0x1<<13))>>13);
    csd->tmpWriteProtect    = (u8)((csdRaw[0]&(0x1<<12))>>12);
    csd->fileFormat         = (u8)((csdRaw[0]&(0x3<<10))>>10);
    csd->crc                = (u8)((csdRaw[0]&(0x7f<<1))>>1);


    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:csdStructure = %d", csd->csdStructure        ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:specVers     = %d", csd->specVers           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:taac         = %d", csd->taac                ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:nsac         = %d", csd->nsac                ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:tranSpeed    = %d", csd->tranSpeed           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:ccc          = %d", csd->ccc             ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:readBlLen    = %d", csd->readBlLen           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:readBlPartial = %d", csd->readBlPartial      ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:writeBlkMisalign = %d", csd->writeBlkMisalign    ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:readBlkMisalign  = %d", csd->readBlkMisalign    ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:dsrImp       = %d", csd->dsrImp              ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:cSize        = %d", csd->cSize               ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:vddRCurrMin  = %d", csd->vddRCurrMin     ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:vddRCurrMax  = %d", csd->vddRCurrMax     ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:vddWCurrMin  = %d", csd->vddWCurrMin     ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:vddWCurrMax  = %d", csd->vddWCurrMax     ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:cSizeMult    = %d", csd->cSizeMult           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:eraseBlkEnable = %d", csd->eraseBlkEnable        ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:sectorSize   = %d", csd->sectorSize          ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:wpGrpSize    = %d", csd->wpGrpSize           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:wpGrpEnable  = %d", csd->wpGrpEnable     ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:r2wFactor    = %d", csd->r2wFactor           ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:writeBlLen   = %d", csd->writeBlLen          ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:writeBlPartial = %d", csd->writeBlPartial        ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:fileFormatGrp = %d", csd->fileFormatGrp       ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:copy  = %d", csd->copy                 );
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:permWriteProtect = %d", csd->permWriteProtect      );
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:tmpWriteProtect  = %d", csd->tmpWriteProtect ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:fileFormat       = %d", csd->fileFormat          ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:crc              = %d", csd->crc                ); 
    MCD_TRACE(MCD_INFO_TRC, 0, "CSD:block number     = %d", csd->blockNumber        ); 

    return MCD_ERR_NO;
}

// =============================================================================
//  FUNCTIONS (public)
// =============================================================================

// =============================================================================
// mcd_ReadCsd
// -----------------------------------------------------------------------------
/// @brief Read the MMC CSD register
/// @param mcdCsd Pointer to the structure where the MMC CSD register info
/// are going to be written. 
// =============================================================================
static MCD_ERR_T mcd_ReadCsd(MCD_CSD_T* mcdCsd)
{
    MCD_ERR_T errStatus = MCD_ERR_NO;
    u32 response[4];

    // Get card CSD
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SEND_CSD, g_mcdRcaReg, response, FALSE);
    if (errStatus == MCD_ERR_NO)
    {
        errStatus = mcd_SdmmcInitCsd(mcdCsd, response);
    }

    // Store it localy
    // FIXME: Is this real ? cf Physical Spec version 2
    // page 59 (72) about CMD16 : default block length
    // is 512 bytes. Isn't 512 bytes a good block
    // length ? And I quote : "In both cases, if block length is set larger
    // than 512Bytes, the card sets the BLOCK_LEN_ERROR bit."
    g_mcdBlockLen = (1 << mcdCsd->readBlLen);
    //if (g_mcdBlockLen > 512)
    //{
        g_mcdBlockLen = 512;
    //}
    //g_mcdNbBlock  = mcdCsd->blockNumber * ((1 << mcdCsd->readBlLen)/g_mcdBlockLen);
    g_mcdNbBlock  = mcdCsd->blockNumber * ((1 << mcdCsd->readBlLen)>>9);

    return errStatus;
}


// =============================================================================
// mcd_Open
// -----------------------------------------------------------------------------
/// @brief Open the SD-MMC chip
/// This function does the init process of the MMC chip, including reseting
/// the chip, sending a command of init to MMC, and reading the CSD
/// configurations.
///
/// @param mcdCsd Pointer to the structure where the MMC CSD register info
/// are going to be written.
///
/// @param mcdVer is t card version.
// =============================================================================
MCD_ERR_T mcd_Open(MCD_CSD_T* mcdCsd, MCD_CARD_VER mcdVer)
{
    MCD_ERR_T                  errStatus   = MCD_ERR_NO;
    u32                     response[4] = {0, 0, 0, 0};
    MCD_CARD_STATUS_T          cardStatus  = {0};
    BOOL                       isMmc       = FALSE;
    HAL_SDMMC_DATA_BUS_WIDTH_T dataBusWidth;
    u32 startTime = 0;
    
    //MCD_PROFILE_FUNCTION_ENTER(mcd_Open);
    MCD_TRACE(MCD_INFO_TRC, 0, "mcd_Open starts ...");

#if 0
    // Check concurrency. Only one mcd_Open.
    u32 cs = hal_SysEnterCriticalSection();
    if (g_mcdSemaphore == 0xFF)
    {
        // Create semaphore and go on with the driver.
        
        // NOTE: This semaphore is never deleted for now, as
        // 1. sema deletion while other task is waiting for it will cause an error;
        // 2. sema deletion will make trouble if already-open state is considered OK.
        g_mcdSemaphore = sxr_NewSemaphore(1);
    }
    hal_SysExitCriticalSection(cs);

    // Following operation should be kept protected
    MCD_CS_ENTER;

    if(NULL == g_mcdConfig)
    {
        g_mcdConfig = tgt_GetMcdConfig();
    }

    if(g_mcdConfig->cardDetectGpio != HAL_GPIO_NONE)
    {
        // Only if GPIO for detection exists, else we try to open anyway.
        if(!mcd_CardDetectUpdateStatus())
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "mcd_Open: GPIO detection - no card");
            MCD_CS_EXIT;
            MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
            return MCD_ERR_NO_CARD;
        }
    }

    if (g_mcdStatus == MCD_STATUS_OPEN)
    {
        // Already opened, just return OK
        MCD_TRACE(MCD_INFO_TRC, 0, "mcd_Open: Already opened");
        *mcdCsd = g_mcdLatestCsd;
        
        MCD_CS_EXIT;
        MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return MCD_ERR_NO;
    }
#endif

    //printf("mcd_Open\n");

    if(MCD_CARD_V1 == mcdVer)
    {
        hal_SdmmcOpen(0x05);
    }
    else
    {
        hal_SdmmcOpen(0x0);
    }
    
    ///@todo: should use g_mcdConfig->dat3HasPullDown if true, we can handle 
    /// basic card detection as follows:
    /// call hal_SdmmcGetCardDetectPinLevel() to check if card as changed (back 1)
    /// send command ACMD42 to disable internal pull up (so pin goes to 0)
    /// meaning if pin is 1, there was a removal, file system should be told to
    /// remount the card as it might be a different one (or the content might have
    /// been changed externally).

    // RCA = 0x0000 for card identification phase.
    g_mcdRcaReg = 0;

    g_mcdSdmmcFreq = 200000;
    hal_SdmmcSetClk(g_mcdSdmmcFreq);

    // assume it's not an SDHC card
    g_mcdCardIsSdhc = FALSE;
    g_mcdOcrReg = MCD_SDMMC_OCR_VOLT_PROF_MASK;

    //printf("CMD0\n");

    // Send Power On command
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_GO_IDLE_STATE, BROAD_ADDR, response, FALSE);
    if (MCD_ERR_NO != errStatus)
    {
        printf("CMD0 Error\n");
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Power on, Initialize Failed");
        MCD_TRACE(MCD_INFO_TRC, 0, "Error Status = %d", errStatus);
        hal_SdmmcClose();

        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }
    else
    {
        //printf("CMD0 Done\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Card successfully in Idle state");
    }

    //printf("CMD8\n");

    // Check if the card is a spec vers.2 one
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SEND_IF_COND, (MCD_CMD8_VOLTAGE_SEL | MCD_CMD8_CHECK_PATTERN), response, FALSE);
    if (MCD_ERR_NO != errStatus)
    {
        printf("CMD8 Error\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "This card doesn't comply to the spec version 2.0");
    }
    else
    {
        // This card comply to the SD spec version 2. Is it compatible with the
        // proposed voltage, and is it an high capacity one ?
        if (response[0] != (MCD_CMD8_VOLTAGE_SEL | MCD_CMD8_CHECK_PATTERN))
        {
            printf("CMD8, Not Supported\n");
            // Bad pattern or unsupported voltage.
            MCD_TRACE(MCD_INFO_TRC, 0, "Bad pattern or unsupported voltage");
            hal_SdmmcClose();
            g_mcdStatus = MCD_STATUS_NOTPRESENT;
            //g_mcdVer = MCD_CARD_V1;  
            //MCD_CS_EXIT;
            //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
            return MCD_ERR_UNUSABLE_CARD;
        }
        else
        {
            //printf("CMD8, Supported\n");
            g_mcdOcrReg |= MCD_OCR_HCS;
        }
    }

    // TODO HCS mask bit to ACMD 41 if high capacity
    // Send OCR, as long as the card return busy
    startTime = hal_TimGetUpTime();
    
    while(1)
    {
        if(hal_TimGetUpTime() - startTime > MCD_SDMMC_OCR_TIMEOUT )
        {
            printf("ACMD41, Retry Timeout\n");
            MCD_TRACE(MCD_INFO_TRC, 0, "SD OCR timeout");
            hal_SdmmcClose();
            g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
            //MCD_CS_EXIT;
            //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
            return MCD_ERR;
        }

        //printf("ACMD41\n");
        errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SEND_OP_COND, g_mcdOcrReg, response, FALSE);
        if (errStatus == MCD_ERR_CARD_NO_RESPONSE)
        {
            printf("ACMD41, No Response, MMC Card?\n");
            MCD_TRACE(MCD_INFO_TRC, 0, "Inserted Card is a MMC");
            // CMD 55 was not recognised: this is not an SD card !
            isMmc = TRUE;
            break;
        }

        // Bit 31 is power up busy status bit(pdf spec p. 109)
        g_mcdOcrReg = (response[0] & 0x7fffffff);
        
        // Power up busy bit is set to 1, 
        // we can continue ...
        if (response[0] & 0x80000000)
        {
            //printf("ACMD41, PowerUp done\n");
            // Version 2?
            if((g_mcdOcrReg & MCD_OCR_HCS) == MCD_OCR_HCS)
            {
                // Card is V2: check for high capacity
                if (response[0] & MCD_OCR_CCS_MASK)
                {
                    g_mcdCardIsSdhc = TRUE;
                    printf("Card is SDHC\n");
                    MCD_TRACE(MCD_INFO_TRC, 0, "Card is SDHC");
                }
                else
                {
                    g_mcdCardIsSdhc = FALSE;
                    printf("Card is V2, but NOT SDHC\n");
                    MCD_TRACE(MCD_INFO_TRC, 0, "Card is standard capacity SD");
                }
            }
            else {
                printf("Card is NOT V2\n");
            }
            MCD_TRACE(MCD_INFO_TRC, 0, "Inserted Card is a SD card");
            break;
        }
    }

    if (isMmc)
    {
        printf("MMC Card\n");
        while(1)
        {
            if(hal_TimGetUpTime() - startTime > MCD_SDMMC_OCR_TIMEOUT )
            {
                MCD_TRACE(MCD_INFO_TRC, 0, "MMC OCR timeout");
                hal_SdmmcClose();
                g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
                //MCD_CS_EXIT;
                //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
                return MCD_ERR;
            }
            
            errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_MMC_SEND_OP_COND, g_mcdOcrReg, response, FALSE);
            if (errStatus == MCD_ERR_CARD_NO_RESPONSE)
            {
                MCD_TRACE(MCD_INFO_TRC, 0, "MMC OCR didn't answer");
                hal_SdmmcClose();
                g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
                //MCD_CS_EXIT;
                //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
                return MCD_ERR;
            }

            // Bit 31 is power up busy status bit(pdf spec p. 109)
            g_mcdOcrReg = (response[0] & 0x7fffffff);
            
            // Power up busy bit is set to 1, 
            // we can continue ...
            if (response[0] & 0x80000000)
            {
                break;
            }
        }    

    }

    //printf("CMD2\n");
    
    // Get the CID of the card.
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_ALL_SEND_CID, BROAD_ADDR, response, FALSE);
    if(MCD_ERR_NO != errStatus)
    {
        printf("CMD2 Fail\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Get CID, Initialize Failed");
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }
    mcd_SdmmcInitCid(response);

    //printf("CMD3\n");
    
    // Get card RCA
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SEND_RELATIVE_ADDR, BROAD_ADDR, response, FALSE);
    if (MCD_ERR_NO != errStatus)
    {
        printf("CMD3 Fail\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Get Relative Address, Initialize Failed");
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }
    
    // Spec Ver 2 pdf p. 81 - rca are the 16 upper bit of this 
    // R6 answer. (lower bits are stuff bits)
    g_mcdRcaReg = response[0] & 0xffff0000;

    //printf("CMD9\n");
    // Get card CSD
    errStatus = mcd_ReadCsd(mcdCsd);
    if (errStatus != MCD_ERR_NO)
    {
        printf("CMD9 Fail\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Get CSD, Initialize Failed");
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }

    // If possible, set the DSR
    if (mcdCsd->dsrImp)
    {
        //printf("CMD4\n");
        errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SET_DSR, g_mcdSdmmcDsr, response, FALSE);
        if (errStatus != MCD_ERR_NO)
        {
            printf("CMD4 Fail\n");
            MCD_TRACE(MCD_INFO_TRC, 0, "Because Set DSR, Initialize Failed");
            hal_SdmmcClose();
            g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
            //MCD_CS_EXIT;
            //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
            return errStatus;
        }
    }

    //printf("CMD7\n");
    // Select the card 
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SELECT_CARD, g_mcdRcaReg, response, FALSE);
    if (errStatus != MCD_ERR_NO)
    {
        printf("CMD7 Fail\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Select Card, Initialize Failed");
        MCD_TRACE(MCD_INFO_TRC, 0, "errStatus = %d", errStatus);
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }
    // That command returns the card status, we check we're in data mode.
    cardStatus.reg = response[0];

    if(!(cardStatus.fields.readyForData))
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "CMD7 status=%0x", cardStatus.reg);
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return MCD_ERR;
    }

    // Set the bus width (use 4 data lines for SD cards only)
    if (isMmc)
    {
        dataBusWidth = HAL_SDMMC_DATA_BUS_WIDTH_1;
    }
    else
    {
        
        dataBusWidth = MCD_CARD_V1 == mcdVer ? HAL_SDMMC_DATA_BUS_WIDTH_1 : HAL_SDMMC_DATA_BUS_WIDTH_4;
       
    }

    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SET_BUS_WIDTH, dataBusWidth,
                                  response, FALSE);
    if (errStatus != MCD_ERR_NO)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Set Bus, Initialize Failed");
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }

    // That command returns the card status, in tran state ?
    cardStatus.reg = response[0];
    if (   !(cardStatus.fields.appCmd)
        || !(cardStatus.fields.readyForData)
        || (cardStatus.fields.currentState != MCD_CARD_STATE_TRAN))
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "ACMD6 status=%0x", cardStatus.reg);
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return MCD_ERR;
    }
    
    // Configure the controller to use that many lines.
    hal_SdmmcSetDataWidth(dataBusWidth);

    // Configure the block lenght
    errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SET_BLOCKLEN, g_mcdBlockLen, response, FALSE);
    if (errStatus != MCD_ERR_NO)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Because Set Block Length, Initialize Failed");
        hal_SdmmcClose();
        g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
        //MCD_CS_EXIT;
        //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
        return errStatus;
    }

    // That command returns the card status, in tran state ?
    cardStatus.reg = response[0];
    {
        MCD_CARD_STATUS_T expectedCardStatus;
        expectedCardStatus.reg  = 0;
        expectedCardStatus.fields.readyForData = 1;
        expectedCardStatus.fields.currentState = MCD_CARD_STATE_TRAN;     
        
        if (cardStatus.reg != expectedCardStatus.reg)
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "CMD16 status=%0x", cardStatus.reg);
            hal_SdmmcClose();
            g_mcdStatus = MCD_STATUS_NOTPRESENT;
            
            //MCD_CS_EXIT;
            //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
            return MCD_ERR;
        }
    }
    
    hal_SdmmcEnterDataTransferMode();

    
    // Set the clock of the SD bus for the fastest possible rate.
 
    g_mcdSdmmcFreq = MCD_CARD_V1 == mcdVer ? 6000000 : 20000000;
     
    hal_SdmmcSetClk(g_mcdSdmmcFreq);

    g_mcdLatestCsd = *mcdCsd;
    g_mcdStatus = MCD_STATUS_OPEN;
    hal_SdmmcSleep();
    g_mcdVer = mcdVer;

    //printf("mcd_Open Done\n");

    //MCD_CS_EXIT;
    //MCD_PROFILE_FUNCTION_EXIT(mcd_Open);
    return MCD_ERR_NO;
}

// =============================================================================
// mcd_Close
// -----------------------------------------------------------------------------
/// Close MCD.
///
/// To be called at the end of the operations
/// @return MCD_ERR_NO if a response with a good crc was received,
///         MCD_ERR_CARD_NO_RESPONSE if no reponse was received within the 
/// driver configured timeout.
///          MCD_ERR_CARD_RESPONSE_BAD_CRC if the received response presented
///  a bad CRC.
///         MCD_ERR_CARD_TIMEOUT if the card timedout during procedure.
// =============================================================================
MCD_ERR_T mcd_Close(void)
{
    MCD_TRACE(MCD_INFO_TRC, 0, "mcd_Close");

    if (g_mcdSemaphore == 0xFF)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "mcd_Close: Never opened before");
        return MCD_ERR_NO;
    }

    //MCD_CS_ENTER;

    MCD_ERR_T errStatus = MCD_ERR_NO;
    
    //MCD_PROFILE_FUNCTION_ENTER(mcd_Close);

    // Don't close the MCD driver if a transfer is in progress,
    // and be definitive about it:
    if (hal_SdmmcIsBusy() == TRUE)
    {
        MCD_TRACE(MCD_WARN_TRC, 0, "MCD: Attempt to close MCD while a transfer is in progress");
    }

    hal_SdmmcWakeUp();
    
    // Brutal force stop current transfer, if any.
    hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);

    // Close the SDMMC module
    hal_SdmmcClose();

    g_mcdStatus = MCD_STATUS_NOTOPEN_PRESENT; // without GPIO
    //mcd_CardDetectUpdateStatus(); // Test GPIO for card detect

    //MCD_CS_EXIT;
    //MCD_PROFILE_FUNCTION_EXIT(mcd_Close);
    return errStatus;
}

//=============================================================================
// mcd_SdmmcTranState
//-----------------------------------------------------------------------------
/// Blocking function checking that the card is in the transfer state, 
/// acknowledging thus that, for example, end of transmission.
/// @param iter Number of attempt to be made.
/// @param duration Sleeping time before the next attempt (in sys ticks). 
//=============================================================================
static MCD_ERR_T mcd_SdmmcTranState(u32 iter)
{
    u32 cardResponse[4] = {0, 0, 0, 0};
    MCD_ERR_T errStatus = MCD_ERR_NO;
    MCD_CARD_STATUS_T cardStatusTranState;
    // Using reg to clear all bit of the bitfields that are not
    // explicitly set.
    cardStatusTranState.reg = 0;
    cardStatusTranState.fields.readyForData = 1; 
    cardStatusTranState.fields.currentState = MCD_CARD_STATE_TRAN;    
    u32 startTime = hal_TimGetUpTime();
    u32 time_out;

    while(1)
    {
        //printf("CMD13, Set Trans State\n");

        errStatus = mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SEND_STATUS, g_mcdRcaReg, cardResponse, FALSE);
        if (errStatus != MCD_ERR_NO)
        {
            printf("CMD13, Fail\n");
            // error while sending the command
            MCD_TRACE(MCD_INFO_TRC, 0, "Sd Tran Read Aft State error! err nb:%d", errStatus);
            return MCD_ERR;
        }
        else if (cardResponse[0] == cardStatusTranState.reg)
        {
            //printf("CMD13, Done\n");
            // the status is the expected one - success
            return MCD_ERR_NO;
        }
        else
        {
            // try again
            // check for timeout 
            time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_CMD_TIMEOUT_V1:MCD_CMD_TIMEOUT_V2;            
            if (hal_TimGetUpTime() - startTime >  time_out )
            {
                printf("CMD13, Timeout\n");
                MCD_TRACE(MCD_INFO_TRC, 0, "Sd Tran don't finished");
                MCD_TRACE(MCD_INFO_TRC, 0, "current state =%0x", cardResponse[0]);
                return MCD_ERR;
            }
        }
    }
}

//=============================================================================
// mcd_SdmmcMultBlockWrite
//-----------------------------------------------------------------------------
/// Write one or a bunch of data blocks.
/// @param blockAddr Address on the card where to write data.
/// @param pWrite Pointer towards the buffer of data to write.
/// @param blockNum Number of blocks to write.
//=============================================================================
static MCD_ERR_T mcd_SdmmcMultBlockWrite(u8* blockAddr, u8* pWrite, u32 blockNum)
{
    u32 cardResponse[4] = {0, 0, 0, 0};
    MCD_ERR_T errStatus = MCD_ERR_NO;
    MCD_CARD_STATUS_T cardStatusTranState;
    // Using reg to clear all bit of the bitfields that are not
    // explicitly set.
    cardStatusTranState.reg = 0;
    cardStatusTranState.fields.readyForData = 1; 
    cardStatusTranState.fields.currentState = MCD_CARD_STATE_TRAN;
    
    MCD_CARD_STATUS_T cardStatusPreErasedState;
    cardStatusPreErasedState.reg    = 0;
    cardStatusPreErasedState.fields.appCmd       = 1;
    cardStatusPreErasedState.fields.readyForData = 1;
    cardStatusPreErasedState.fields.currentState = MCD_CARD_STATE_TRAN;

    MCD_CARD_STATUS_T cardStatusResponse = {0,};

    u32 startTime = 0;

    HAL_SDMMC_CMD_T writeCmd;
    u32 tran_time_out;
    u32 write_time_out;

    g_mcdSdmmcTransfer.sysMemAddr = (u8*) pWrite;
    g_mcdSdmmcTransfer.sdCardAddr = blockAddr;
    g_mcdSdmmcTransfer.blockNum   = blockNum;
    g_mcdSdmmcTransfer.blockSize  = g_mcdBlockLen;
    g_mcdSdmmcTransfer.direction  = HAL_SDMMC_DIRECTION_WRITE;


// FIXME find what the heck is that !:
// program_right_num=0;

    // Check buffer.
    assert(pWrite != NULL, "SDMMC write: Buffer is NULL");
    assert(((u32)pWrite & 0x3) ==0,
               "SDMMC write: buffer is not aligned! addr=%08x", pWrite);
    assert(blockNum>=1 && blockNum<= MCD_MAX_BLOCK_NUM,
                "Block Num is overflow");
        
    // Check that the card is in tran (Transmission) state.
    tran_time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_TRAN_TIMEOUT_V1:MCD_TRAN_TIMEOUT_V2;
    if (MCD_ERR_NO != mcd_SdmmcTranState(tran_time_out))
    // 5200000, 0, initially, that is to say rougly 0.1 sec ?
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Write on Sdmmc while not in Tran state");
        return MCD_ERR_CARD_TIMEOUT;
    }

    // The command for single block or multiple blocks are differents
    if (blockNum == 1)
    {
        writeCmd = HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK;
    }
    else
    {
        writeCmd = HAL_SDMMC_CMD_WRITE_MULT_BLOCK;
    }

    // PreErasing, to accelerate the multi-block write operations
    if (blockNum >1)
    {
        //printf("SET_WR_BLK_COUNT\n");
        if(MCD_ERR == mcd_SdmmcSendCmd(HAL_SDMMC_CMD_SET_WR_BLK_COUNT, blockNum, cardResponse, FALSE))
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "Set Pre-erase Failed");
            return MCD_ERR;
        }

        // Advance compatibility,to support 1.0 t-flash.
        if (cardResponse[0] != cardStatusPreErasedState.reg)
        {
            MCD_TRACE(MCD_INFO_TRC, 0, "warning ACMD23 status=%0x", cardResponse[0]);
           // cardResponse[0] = cardStatusPreErasedState.reg;
            // return MCD_ERR;
        }
    }
    

    // Initiate data migration through Ifc.
    if (hal_SdmmcTransfer(&g_mcdSdmmcTransfer) != 0)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "write sd no ifc channel");
        return MCD_ERR_DMA_BUSY;
    }

    //printf("writeCmd\n");

    // Initiate data migration of multiple blocks through SD bus.
    errStatus = mcd_SdmmcSendCmd(writeCmd,
                                 (u32)blockAddr,
                                 cardResponse,
                                 FALSE);
    if (errStatus != MCD_ERR_NO)
    {
        printf("writeCmd Fail, err = %d\n", (int)errStatus);
        MCD_TRACE(MCD_INFO_TRC, 0, "Set sd write had error");
        hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
        return MCD_ERR_CMD;
    }

    cardStatusResponse.reg = cardResponse[0] ;
    // Check for error, by listing all valid states
    // TODO: FIXME, more states could be legal here
    // The sixteen uper bits are error bits: they all must be null
    // (Physical Spec V.2, p71)
    if ((cardResponse[0] != cardStatusTranState.reg)
     && !((cardStatusResponse.fields.readyForData == 1)   
            && (cardStatusResponse.fields.currentState == MCD_CARD_STATE_RCV)
            && ((cardStatusResponse.reg&0xFFFF0000) == 0)) 
     && !(cardStatusResponse.fields.currentState == MCD_CARD_STATE_RCV
            && ((cardStatusResponse.reg&0xFFFF0000) == 0))
     && !(cardStatusResponse.fields.currentState == MCD_CARD_STATE_PRG
            && ((cardStatusResponse.reg&0xFFFF0000) == 0))
     )
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Write block,Card Reponse: %x", cardResponse[0]);
        hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
        return MCD_ERR;
    }


    // Wait 
    startTime = hal_TimGetUpTime();
    write_time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_WRITE_TIMEOUT_V1:MCD_WRITE_TIMEOUT_V2;
    while(!hal_SdmmcTransferDone()) 
    {
        if (hal_TimGetUpTime() - startTime >  (write_time_out*blockNum))
        {
            printf("writeCmd Timeout\n");
            MCD_TRACE(MCD_INFO_TRC, 0, "Write on Sdmmc timeout");
            // Abort the transfert.
            hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
            return MCD_ERR_CARD_TIMEOUT;
        }
    }
    //printf("writeCmd Done\n");


    // Nota: CMD12 (stop transfer) is automatically 
    // sent by the SDMMC controller.

    if (mcd_SdmmcWriteCheckCrc() != MCD_ERR_NO)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "SDMMC Write error");
        return MCD_ERR;
    }
    //printf("CheckCRC Done\n");
    
    // Check that the card is in tran (Transmission) state.
    if (MCD_ERR_NO != mcd_SdmmcTranState(tran_time_out))
    // 5200000, 0, initially, that is to say rougly 0.1 sec ?
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "Write on Sdmmc while not in Tran state");
        return MCD_ERR_CARD_TIMEOUT;
    }
    //printf("Check Tran State Done\n");
    return MCD_ERR_NO;

}

//=============================================================================
// mcd_SdmmcMultBlockRead
//-----------------------------------------------------------------------------
/// Read one or a bunch of data blocks.
/// @param blockAddr Address on the card where to read data.
/// @param pRead Pointer towards the buffer of data to read.
/// @param blockNum Number of blocks to read.
//=============================================================================
static MCD_ERR_T mcd_SdmmcMultBlockRead(u8* blockAddr, u8* pRead, u32 blockNum)
{
    u32                  cardResponse[4]     = {0, 0, 0, 0};
    MCD_ERR_T               errStatus           = MCD_ERR_NO;
    HAL_SDMMC_CMD_T         readCmd;
    MCD_CARD_STATUS_T cardStatusTranState;
    // Using reg to clear all bit of the bitfields that are not
    // explicitly set.
    cardStatusTranState.reg = 0;
    cardStatusTranState.fields.readyForData = 1; 
    cardStatusTranState.fields.currentState = MCD_CARD_STATE_TRAN;
    
    u32 startTime=0;
    u32 tran_time_out;
    u32 read_time_out;

    g_mcdSdmmcTransfer.sysMemAddr = (u8*) pRead;
    g_mcdSdmmcTransfer.sdCardAddr = blockAddr;
    g_mcdSdmmcTransfer.blockNum   = blockNum;
    g_mcdSdmmcTransfer.blockSize  = g_mcdBlockLen;
    g_mcdSdmmcTransfer.direction  = HAL_SDMMC_DIRECTION_READ;
    
    // Check buffer.
    assert(pRead != NULL, "SDMMC write: Buffer is NULL");
    assert(((u32)pRead & 0x3) ==0, "SDMMC write: buffer is not aligned");
    assert(blockNum>=1 && blockNum<= MCD_MAX_BLOCK_NUM,
                "Block Num is overflow");

    // Command are different for reading one or several blocks of data
    if (blockNum == 1)
    {
        readCmd = HAL_SDMMC_CMD_READ_SINGLE_BLOCK;
    }
    else
    {
        readCmd = HAL_SDMMC_CMD_READ_MULT_BLOCK;
    }

    // Initiate data migration through Ifc.
    if (hal_SdmmcTransfer(&g_mcdSdmmcTransfer) != 0)
    {
        MCD_TRACE(MCD_INFO_TRC, 0, "write sd no ifc channel");
        return MCD_ERR_DMA_BUSY;
    }

    // Initiate data migration of multiple blocks through SD bus.
    errStatus = mcd_SdmmcSendCmd(readCmd,
                                  (u32)blockAddr,
                                  cardResponse,
                                  FALSE);
    if (errStatus != MCD_ERR_NO)
    {
        printf("mcd_SdmmcMultBlockRead, send command error\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Set sd write had error");
        hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
        return MCD_ERR_CMD;
    }

    if (cardResponse[0] != cardStatusTranState.reg)
    {
        printf("mcd_SdmmcMultBlockRead, command response error\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "CMD%d status=%0x", cardResponse[0], readCmd);
        hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
        return MCD_ERR;
    }

    // Wait 
    startTime = hal_TimGetUpTime();
    read_time_out =  (MCD_CARD_V1 == g_mcdVer) ? MCD_READ_TIMEOUT_V1:MCD_READ_TIMEOUT_V2;
    while(!hal_SdmmcTransferDone()) 
    {
        if (hal_TimGetUpTime() - startTime > (read_time_out*blockNum))
        {
            printf("mcd_SdmmcMultBlockRead, timeout\n");
            MCD_TRACE(MCD_INFO_TRC, 0, "Read on Sdmmc timeout");
            // Abort the transfert.
            hal_SdmmcStopTransfer(&g_mcdSdmmcTransfer);
            return MCD_ERR_CARD_TIMEOUT;
        }
    }

    // Nota: CMD12 (stop transfer) is automatically 
    // sent by the SDMMC controller.
 
    if (mcd_SdmmcReadCheckCrc() != MCD_ERR_NO)
    {
        printf("mcd_SdmmcMultBlockRead, crc error\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "sdc read state error");
        return MCD_ERR;
    }

    tran_time_out = (MCD_CARD_V1 == g_mcdVer) ? MCD_TRAN_TIMEOUT_V1:MCD_TRAN_TIMEOUT_V2;
    // Check that the card is in tran (Transmission) state.
    if (MCD_ERR_NO != mcd_SdmmcTranState(tran_time_out))
    // 5200000, 0, initially, that is to say rougly 0.1 sec ?
    {
        printf("mcd_SdmmcMultBlockRead, trans state timeout\n");
        MCD_TRACE(MCD_INFO_TRC, 0, "Read on Sdmmc while not in Tran state");
        return MCD_ERR_CARD_TIMEOUT;
    } 
    
    // Flush Cache
    //hal_SysInvalidateCache(pRead, blockNum * g_mcdBlockLen);

    return MCD_ERR_NO;
}

// =============================================================================
// mcd_Write
// -----------------------------------------------------------------------------
/// @brief Write a block of data to MMC.
///
/// This function is used to write blocks of data on the MMC.
/// @param startAddr Start Adress  of the SD memory block where the
/// data will be written
/// @param blockWr Pointer to the block of data to write. Must be aligned 
/// on a 32 bits boundary.
/// @param size Number of bytes to write. Must be an interger multiple of the 
/// sector size of the card
// =============================================================================
MCD_ERR_T mcd_Write(u32 startAddr, u8* blockWr, u32 size)
{
    //MCD_CS_ENTER;
    u8*      tmpAdd  = NULL;
    MCD_ERR_T   value   = MCD_ERR_NO;
    u32 i = 0;
    
    //MCD_PROFILE_FUNCTION_ENTER(mcd_Write);
    assert(g_mcdBlockLen != 0, "mcd_Write called before a successful mcd_Open");
    assert(startAddr % g_mcdBlockLen == 0,
        "write card address is not aligned");
    assert((size % g_mcdBlockLen) == 0, "mcd_Write size (%d) is not a multiple of"
                                        "the block size (%d)",
                                        size, g_mcdBlockLen);

    //printf("mcd_Write\n");

    //if(!mcd_CardDetectUpdateStatus())
    //{
    //    MCD_PROFILE_FUNCTION_EXIT(mcd_Write);
    //    MCD_CS_EXIT;
    //    return MCD_ERR_NO_CARD;
    //}

    hal_SdmmcWakeUp();
    // Addresses are block number for high capacity card
    if (g_mcdCardIsSdhc)
    {
        tmpAdd = (u8*) (startAddr);
    }
    else
    {
        tmpAdd = (u8*) (startAddr * g_mcdBlockLen);
    }
    if(MCD_CARD_V1 == g_mcdVer)
    {
        //printf("mcd_Write, V1\n");

        //for(i = 0; i < size/g_mcdBlockLen; i++)
        for(i = 0; i < size>>9; i++)
        {
            value = mcd_SdmmcMultBlockWrite(tmpAdd + i*g_mcdBlockLen, blockWr + i*g_mcdBlockLen, 1);
            if(value != MCD_ERR_NO)
            {
                break;
            }
        }
    }
    else
    {
        //printf("mcd_Write, V2\n");
        //value = mcd_SdmmcMultBlockWrite(tmpAdd, blockWr, size/g_mcdBlockLen);
        value = mcd_SdmmcMultBlockWrite(tmpAdd, blockWr, size>>9);
    }

    hal_SdmmcSleep();
    //MCD_PROFILE_FUNCTION_EXIT(mcd_Write);
    //MCD_CS_EXIT;
    return value;
}

// =============================================================================
// mcd_Read
// -----------------------------------------------------------------------------
/// @brief Read using pattern mode.
/// @ param startAddr: of the SD memory block where the data
/// will be read
/// @param blockRd Pointer to the buffer where the data will be stored. Must be aligned 
/// on a 32 bits boundary.
/// @param size Number of bytes to read. Must be an interger multiple of the 
/// sector size of the card.
// =============================================================================
MCD_ERR_T mcd_Read(u32 startAddr, u8* blockRd, u32 size)
{
    //MCD_CS_ENTER;
    u8*      tmpAdd  = NULL;
    MCD_ERR_T   value   = MCD_ERR_NO;

    //MCD_PROFILE_FUNCTION_ENTER(mcd_Read);
    assert(g_mcdBlockLen != 0, "mcd_Read called before a successful mcd_Open_V1 or mcd_Open_V2");
    assert(startAddr % g_mcdBlockLen == 0,
                "read card address is not aligned");
    assert((size % g_mcdBlockLen) == 0, "mcd_Read size (%d) is not a multiple of"
                                        "the block size (%d)",
                                        size, g_mcdBlockLen);

    //printf("mcd_Read\n");

    //if(!mcd_CardDetectUpdateStatus())
    //{
    //    MCD_PROFILE_FUNCTION_EXIT(mcd_Read);
    //    MCD_CS_EXIT;
    //    return MCD_ERR_NO_CARD;
    //}

    hal_SdmmcWakeUp();
    // Addresses are block number for high capacity card
    if (g_mcdCardIsSdhc)
    {
        tmpAdd = (u8*) (startAddr);
    }
    else
    {
        tmpAdd = (u8*) (startAddr * g_mcdBlockLen);
    }
    
    //value = mcd_SdmmcMultBlockRead(tmpAdd, blockRd, size/g_mcdBlockLen);
    value = mcd_SdmmcMultBlockRead(tmpAdd, blockRd, size >> 9);

    hal_SdmmcSleep();

    //MCD_PROFILE_FUNCTION_EXIT(mcd_Read);
    //MCD_CS_EXIT;
    return value;

}

BOOL mcd_IsHighCapacityCard(void)
{
    if(g_mcdCardIsSdhc == TRUE)
    {
        return TRUE;
    }
    return FALSE;

}

static block_dev_desc_t mmc_blk_dev;
static int initialized = 0;

int mmc_set_dev(int dev_num)
{
	return 0;
}

block_dev_desc_t *mmc_get_dev(int dev)
{
	if (initialized)
		return (block_dev_desc_t *) &mmc_blk_dev;
	else
		return NULL;
}

static unsigned long mmc_bread(int dev_num, unsigned long blknr,
		lbaint_t blkcnt, void *dst)
{
    MCD_ERR_T mcd_ret;
    int block_size, i;

    block_size = 512;

    printf("\nmmc_bread, %ld blks from %ld, dst = 0x%08lx\n",
        (u32)blkcnt, blknr, (u32)dst);
    for (i=0;i<blkcnt;i++) {
        mcd_ret = mcd_Read(blknr + i, 
            (u8 *)(dst) + i*block_size, 
            block_size);
        if (mcd_ret != MCD_ERR_NO) {
            printf("Read SD card Failed\n");
            return i;
        }
    }
    printf("Done\n");

	return i;
}

int mmc_legacy_init(int dev)
{
    MCD_ERR_T mcd_ret;
    MCD_CSD_T mcd_csd;

    mcd_ret = mcd_Open(&mcd_csd, MCD_CARD_V2);
    if (mcd_ret != MCD_ERR_NO) {
        printf("Open SD card Failed\n");
        return 1;
    }

    printf("Open SD card Done\n");

	mmc_blk_dev.if_type = IF_TYPE_MMC;
	mmc_blk_dev.part_type = PART_TYPE_DOS;
	mmc_blk_dev.dev = 0;
	mmc_blk_dev.lun = 0;
	mmc_blk_dev.type = 0;

	/* FIXME fill in the correct size (is set to 32MByte) */
	mmc_blk_dev.blksz = 512;
	mmc_blk_dev.lba = 0x10000;
	mmc_blk_dev.removable = 0;
	mmc_blk_dev.block_read = mmc_bread;

#if 0
	if (fat_register_device(&mmc_blk_dev, 1))
		printf("Could not register MMC fat device\n");
#else
	init_part(&mmc_blk_dev);
#endif

	initialized = 1;
	return 0;
}

