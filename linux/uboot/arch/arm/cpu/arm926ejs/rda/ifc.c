#include "common.h"
#include <errno.h>

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/reg_ifc.h>
#include <asm/arch/ifc.h>

#ifdef DEBUG
#include <assert.h>
#else
#define assert(...)
#endif 

#define HAL_TRACE(...)

HAL_IFC_REQUEST_ID_T g_halModuleIfcChannelOwner[SYS_IFC_STD_CHAN_NB];

void hal_IfcOpen(void)
{
    u8 channel;

    // Initialize the channel table with unknown requests.
    for (channel = 0; channel < SYS_IFC_STD_CHAN_NB; channel++)
    {
        g_halModuleIfcChannelOwner[channel] = HAL_IFC_NO_REQWEST;
    }
}

HAL_IFC_REQUEST_ID_T hal_IfcGetOwner(u8 channel)
{
    // Here, we consider the transfer as previously finished.
    if (channel == HAL_UNKNOWN_CHANNEL) return HAL_IFC_NO_REQWEST;

    // Channel number too big.
    assert(channel < SYS_IFC_STD_CHAN_NB, channel);

    return g_halModuleIfcChannelOwner[channel];
}

void hal_IfcChannelRelease(HAL_IFC_REQUEST_ID_T requestId, u8 channel)
{
    //u32 status;

    // Here, we consider the transfer as previously finished.
    if (channel == HAL_UNKNOWN_CHANNEL) return;

    // Channel number too big.
    assert(channel < SYS_IFC_STD_CHAN_NB, channel);

    //status = hal_SysEnterCriticalSection();
    if (g_halModuleIfcChannelOwner[channel] == requestId)
    {
        // disable this channel
        hwp_sysIfc->std_ch[channel].control = (SYS_IFC_REQ_SRC(requestId)
                                            | SYS_IFC_CH_RD_HW_EXCH
                                            | SYS_IFC_DISABLE);
        // read the status of this channel
        if (hwp_sysIfc->std_ch[channel].status & SYS_IFC_ENABLE)
        {
            HAL_TRACE(_HAL | TSTDOUT,0," Strange, the released channel not disabled yet");
        }
        // Write the TC to 0 for next time the channel is re-enabled
        hwp_sysIfc->std_ch[channel].tc =  0;
    }
    //hal_SysExitCriticalSection(status);
}

void hal_IfcChannelFlush(HAL_IFC_REQUEST_ID_T requestId, u8 channel)
{
    //u32 status;

    // Here, we consider the transfer as previously finished.
    if (channel == HAL_UNKNOWN_CHANNEL) return;

    // Channel number too big.
    assert(channel < SYS_IFC_STD_CHAN_NB, channel);

    // Check that the channel is really owned by the peripheral
    // which is doing the request, it could have been release
    // automatically or by an IRQ handler.
    //status = hal_SysEnterCriticalSection();
    if (g_halModuleIfcChannelOwner[channel] == requestId)
    {
        // If fifo not empty, flush it.
        if ( !(hwp_sysIfc->std_ch[channel].status & SYS_IFC_FIFO_EMPTY) )
        {
            hwp_sysIfc->std_ch[channel].control = 
                hwp_sysIfc->std_ch[channel].control | SYS_IFC_FLUSH;
        }
    }
    //hal_SysExitCriticalSection(status);
}

BOOL hal_IfcChannelIsFifoEmpty(HAL_IFC_REQUEST_ID_T requestId, u8 channel)
{
    //u32 status;
    BOOL fifoIsEmpty = TRUE;
    
    // Here, we consider the transfer as previously finished.
    if (channel == HAL_UNKNOWN_CHANNEL) return fifoIsEmpty;

    // Channel number too big.
    assert(channel < SYS_IFC_STD_CHAN_NB, channel);

    // Check that the channel is really owned by the peripheral
    // which is doing the request, it could have been release
    // automatically or by an IRQ handler.
    //status = hal_SysEnterCriticalSection();
    if (g_halModuleIfcChannelOwner[channel] == requestId)
    {
        fifoIsEmpty =
            (FALSE != (hwp_sysIfc->std_ch[channel].status & SYS_IFC_FIFO_EMPTY));
    }
    //hal_SysExitCriticalSection(status);

    return fifoIsEmpty;
}

u8 hal_IfcTransferStart(HAL_IFC_REQUEST_ID_T requestId, u8* memStartAddr, u32 xferSize, HAL_IFC_MODE_T ifcMode)
{
    //u32 status = hal_SysEnterCriticalSection();
    u8 channel;
    u8 i;

    // Check buffer alignment depending on the mode
    if (ifcMode != HAL_IFC_SIZE_8_MODE_MANUAL && ifcMode != HAL_IFC_SIZE_8_MODE_AUTO)
    {
        // Then ifcMode == HAL_IFC_SIZE_32, check word alignment
        assert(((u32)memStartAddr%4) == 0,
            "HAL IFC: 32 bits transfer misaligned 0x@%08X", memStartAddr);
    }
    else
    {
        // ifcMode == HAL_IFC_SIZE_8, nothing to check
    }

    // Check the requested id is not currently already used.
    for (i = 0; i < SYS_IFC_STD_CHAN_NB ; i++)
    {
        if (GET_BITFIELD(hwp_sysIfc->std_ch[i].control, SYS_IFC_REQ_SRC) == requestId)
        {
            // This channel is or was used for the requestId request.
            // Check it is still in use.
            assert((hwp_sysIfc->std_ch[i].status & SYS_IFC_ENABLE) == 0,
                    "HAL: Attempt to use the IFC to deal with a %d"
                    " request still active on channel %d", requestId, i);
        }
    }

    channel = SYS_IFC_CH_TO_USE(hwp_sysIfc->get_ch) ;

    if (channel >= SYS_IFC_STD_CHAN_NB)
    {
        serial_puts("HAL_UNKNOWN_CHANNEL\n");
        //hal_SysExitCriticalSection(status);
        return HAL_UNKNOWN_CHANNEL;
    }

    g_halModuleIfcChannelOwner[channel]     = requestId;
    hwp_sysIfc->std_ch[channel].start_addr  =  (u32) memStartAddr;
    hwp_sysIfc->std_ch[channel].tc          =  xferSize;
    hwp_sysIfc->std_ch[channel].control     = (SYS_IFC_REQ_SRC(requestId) 
                                            | ifcMode
                                            | SYS_IFC_CH_RD_HW_EXCH
                                            | SYS_IFC_ENABLE);

    //hal_SysExitCriticalSection(status);
    return channel;
}

