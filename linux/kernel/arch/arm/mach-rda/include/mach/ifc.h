#ifndef  __ASM_ARCH_IFC_H
#define  __ASM_ARCH_IFC_H

#include <linux/scatterlist.h>
#include <plat/reg_ifc.h>

#define HAL_UNKNOWN_CHANNEL 	 0xff

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

typedef enum
{
    HAL_IFC_SIZE_8_MODE_MANUAL  = 0,
    HAL_IFC_SIZE_8_MODE_AUTO    = (SYS_IFC_AUTODISABLE),
    HAL_IFC_SIZE_32_MODE_MANUAL = (SYS_IFC_SIZE_WORD),
    HAL_IFC_SIZE_32_MODE_AUTO   = (SYS_IFC_SIZE_WORD | SYS_IFC_AUTODISABLE),
} HAL_IFC_MODE_T;

typedef struct
{
	/* I/O scatter list */
	struct scatterlist *sg;
	/* size of scatter list */
	u32 sg_len;

	HAL_IFC_REQUEST_ID_T request;
	u32 channel;
} HAL_IFC_CFG_T;

u8 ifc_transfer_sg_start(HAL_IFC_CFG_T *ifc,
	u32 xfer_size,
	HAL_IFC_MODE_T ifc_mode);

u32 ifc_transfer_sg_get_tc(HAL_IFC_CFG_T *ifc);

void ifc_transfer_sg_stop(HAL_IFC_CFG_T *ifc);

u8 ifc_transfer_start(HAL_IFC_REQUEST_ID_T request_id,
	u8* mem_addr,
	u32 xfer_size,
	HAL_IFC_MODE_T ifc_mode);

u32 ifc_transfer_get_tc(HAL_IFC_REQUEST_ID_T request_id, u8 channel);

void ifc_transfer_flush(HAL_IFC_REQUEST_ID_T request_id, u8 channel);

void ifc_transfer_stop(HAL_IFC_REQUEST_ID_T request_id, u8 channel);

#endif //  __ASM_ARCH_IFC_H
