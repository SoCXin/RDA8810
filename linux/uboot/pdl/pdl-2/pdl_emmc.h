#ifndef __PDL_EMMC_H_
#define __PDL_EMMC_H_

int emmc_data_start(const char *part_name, unsigned long file_size);
int emmc_data_midst(u8 *data, size_t size);
int emmc_data_end(uint32_t crc);
int emmc_read_partition(const char *part_name, unsigned char *data, size_t size,
			size_t *actual_len);
int emmc_write_partition(const char *part_name, unsigned char *data, size_t size);
int emmc_format_flash(void);
int emmc_erase_partition(const char *part_name);
int pdl_emmc_init(void);
#endif

