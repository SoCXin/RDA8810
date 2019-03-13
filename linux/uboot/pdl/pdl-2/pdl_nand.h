#ifndef __PDL_NAND_H_
#define __PDL_NAND_H_

int nand_data_start(const char *part_name, unsigned long file_size);
int nand_data_midst(u8 *data, size_t size);
int nand_data_end(uint32_t crc);
int nand_read_partition(const char *part_name, unsigned char *data,
		size_t size, size_t *actual_len);
int nand_write_partition(const char *part_name, unsigned char *data,
		size_t size);
int nand_format_flash(void);
int nand_erase_partition(const char *part_name);

int pdl_nand_init(void);

#ifdef MTDPARTS_UBI_DEF
int ubi_part_scan(const char *part_name);
int ubi_check_default_vols(const char *ubi_default_str);
int ubi_erase_vol(const char *vol_name);
int ubi_read_vol(const char *vol_name, loff_t offp, void *buf, size_t size);
int ubi_update_vol(const char *vol_name, void *buf, size_t size);
size_t ubi_sizeof_vol(const char *vol_name);

/* write a volume by multi parts. */
int ubi_prepare_write_vol(const char *volume, size_t size);
int ubi_do_write_vol(const char *volume, void *buf, size_t size);
int ubi_finish_write_vol(const char *volume);
#endif

#endif

