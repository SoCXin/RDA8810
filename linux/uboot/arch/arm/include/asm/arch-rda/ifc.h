#ifndef  _IFC_H_
#define  _IFC_H_

#include <asm/arch/hardware.h>

#define HAL_UNKNOWN_CHANNEL      0xff

typedef enum {
    HAL_IFC_UART_TX,
    HAL_IFC_UART_RX,
    HAL_IFC_UART2_TX,
    HAL_IFC_UART2_RX,
    HAL_IFC_SPI_TX,
    HAL_IFC_SPI_RX,
    HAL_IFC_SPI2_TX,
    HAL_IFC_SPI2_RX,
    HAL_IFC_SPI3_TX,
    HAL_IFC_SPI3_RX,
    HAL_IFC_SDMMC_TX,
    HAL_IFC_SDMMC_RX,
    HAL_IFC_SDMMC2_TX,
    HAL_IFC_SDMMC2_RX,
    HAL_IFC_SDMMC3_TX,
    HAL_IFC_SDMMC3_RX,
    HAL_IFC_NFSC_TX,
    HAL_IFC_NFSC_RX,
    HAL_IFC_UART3_TX,
    HAL_IFC_UART3_RX,
    HAL_IFC_NO_REQWEST
} HAL_IFC_REQUEST_ID_T;

// =============================================================================
// HAL_IFC_MODE_T
// -----------------------------------------------------------------------------
/// Define the mode used to configure an IFC transfer. This enum describes
/// the width (8 or 32 bits) and if the transfer is autodisabled or manually
/// disabled.
// =============================================================================
typedef enum
{
    HAL_IFC_SIZE_8_MODE_MANUAL  = (0 | 0),
    HAL_IFC_SIZE_8_MODE_AUTO    = (0 | SYS_IFC_AUTODISABLE),
    HAL_IFC_SIZE_32_MODE_MANUAL = (SYS_IFC_SIZE_WORD | 0),
    HAL_IFC_SIZE_32_MODE_AUTO   = (SYS_IFC_SIZE_WORD | SYS_IFC_AUTODISABLE),
} HAL_IFC_MODE_T;

void hal_IfcOpen(void);
HAL_IFC_REQUEST_ID_T hal_IfcGetOwner(u8 channel);
void hal_IfcChannelRelease(HAL_IFC_REQUEST_ID_T requestId, u8 channel);
void hal_IfcChannelFlush(HAL_IFC_REQUEST_ID_T requestId, u8 channel);
BOOL hal_IfcChannelIsFifoEmpty(HAL_IFC_REQUEST_ID_T requestId, u8 channel);
u8 hal_IfcTransferStart(HAL_IFC_REQUEST_ID_T requestId, u8* memStartAddr, u32 xferSize, HAL_IFC_MODE_T ifcMode);

#endif //  _IFC_H 

