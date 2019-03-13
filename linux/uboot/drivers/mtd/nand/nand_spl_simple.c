/*
 * (C) Copyright 2006-2008
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <nand.h>
#include <asm/io.h>
#include <linux/mtd/nand_ecc.h>
#include <mtd/nand/rda_nand.h>

static nand_info_t mtd;
static struct nand_chip nand_chip;

#ifndef CONFIG_NAND_RDA
static int nand_ecc_pos[] = CONFIG_SYS_NAND_ECCPOS;
#define ECCSTEPS	(CONFIG_SYS_NAND_PAGE_SIZE / \
					CONFIG_SYS_NAND_ECCSIZE)
#define ECCTOTAL	(ECCSTEPS * CONFIG_SYS_NAND_ECCBYTES)


#if (CONFIG_SYS_NAND_PAGE_SIZE <= 512)
/*
 * NAND command for small page NAND devices (512)
 */
static int nand_command(int block, int page, uint32_t offs,
	u8 cmd)
{
	struct nand_chip *this = mtd.priv;
	int page_addr = page + block * CONFIG_SYS_NAND_PAGE_COUNT;

	while (!this->dev_ready(&mtd))
		;

	/* Begin command latch cycle */
	this->cmd_ctrl(&mtd, cmd, NAND_CTRL_CLE | NAND_CTRL_CHANGE);
	/* Set ALE and clear CLE to start address cycle */
	/* Column address */
	this->cmd_ctrl(&mtd, offs, NAND_CTRL_ALE | NAND_CTRL_CHANGE);
	this->cmd_ctrl(&mtd, page_addr & 0xff, NAND_CTRL_ALE); /* A[16:9] */
	this->cmd_ctrl(&mtd, (page_addr >> 8) & 0xff,
		       NAND_CTRL_ALE); /* A[24:17] */
#ifdef CONFIG_SYS_NAND_4_ADDR_CYCLE
	/* One more address cycle for devices > 32MiB */
	this->cmd_ctrl(&mtd, (page_addr >> 16) & 0x0f,
		       NAND_CTRL_ALE); /* A[28:25] */
#endif
	/* Latch in address */
	this->cmd_ctrl(&mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Wait a while for the data to be ready
	 */
	while (!this->dev_ready(&mtd))
		;

	return 0;
}
#else
/*
 * NAND command for large page NAND devices (2k)
 */
static int nand_command(int block, int page, uint32_t offs,
	u8 cmd)
{
	struct nand_chip *this = mtd.priv;
	int page_addr = page + block * CONFIG_SYS_NAND_PAGE_COUNT;
	void (*hwctrl)(struct mtd_info *mtd, int cmd,
			unsigned int ctrl) = this->cmd_ctrl;

	while (!this->dev_ready(&mtd))
		;

	/* Emulate NAND_CMD_READOOB */
	if (cmd == NAND_CMD_READOOB) {
		offs += CONFIG_SYS_NAND_PAGE_SIZE;
		cmd = NAND_CMD_READ0;
	}

	/* Shift the offset from byte addressing to word addressing. */
	if (this->options & NAND_BUSWIDTH_16)
		offs >>= 1;

	/* Begin command latch cycle */
	hwctrl(&mtd, cmd, NAND_CTRL_CLE | NAND_CTRL_CHANGE);
	/* Set ALE and clear CLE to start address cycle */
	/* Column address */
	hwctrl(&mtd, offs & 0xff,
		       NAND_CTRL_ALE | NAND_CTRL_CHANGE); /* A[7:0] */
	hwctrl(&mtd, (offs >> 8) & 0xff, NAND_CTRL_ALE); /* A[11:9] */
	/* Row address */
	hwctrl(&mtd, (page_addr & 0xff), NAND_CTRL_ALE); /* A[19:12] */
	hwctrl(&mtd, ((page_addr >> 8) & 0xff),
		       NAND_CTRL_ALE); /* A[27:20] */
#ifdef CONFIG_SYS_NAND_5_ADDR_CYCLE
	/* One more address cycle for devices > 128MiB */
	hwctrl(&mtd, (page_addr >> 16) & 0x0f,
		       NAND_CTRL_ALE); /* A[31:28] */
#endif
	/* Latch in address */
	hwctrl(&mtd, NAND_CMD_READSTART,
		       NAND_CTRL_CLE | NAND_CTRL_CHANGE);
	hwctrl(&mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Wait a while for the data to be ready
	 */
	while (!this->dev_ready(&mtd))
		;

	return 0;
}
#endif

static int nand_is_bad_block(int block)
{
	struct nand_chip *this = mtd.priv;

	nand_command(block, 0, CONFIG_SYS_NAND_BAD_BLOCK_POS,
		NAND_CMD_READOOB);

	/*
	 * Read one byte (or two if it's a 16 bit chip).
	 */
	if (this->options & NAND_BUSWIDTH_16) {
		if (readw(this->IO_ADDR_R) != 0xffff)
			return 1;
	} else {
		if (readb(this->IO_ADDR_R) != 0xff)
			return 1;
	}

	return 0;
}

#if defined(CONFIG_SYS_NAND_HW_ECC_OOBFIRST)
static int nand_read_page(int block, int page, uchar *dst)
{
	struct nand_chip *this = mtd.priv;
	u_char ecc_calc[ECCTOTAL];
	u_char ecc_code[ECCTOTAL];
	u_char oob_data[CONFIG_SYS_NAND_OOBSIZE];
	int i;
	int eccsize = CONFIG_SYS_NAND_ECCSIZE;
	int eccbytes = CONFIG_SYS_NAND_ECCBYTES;
	int eccsteps = ECCSTEPS;
	uint8_t *p = dst;

	nand_command(block, page, 0, NAND_CMD_READOOB);
	this->read_buf(&mtd, oob_data, CONFIG_SYS_NAND_OOBSIZE);
	nand_command(block, page, 0, NAND_CMD_READ0);

	/* Pick the ECC bytes out of the oob data */
	for (i = 0; i < ECCTOTAL; i++)
		ecc_code[i] = oob_data[nand_ecc_pos[i]];


	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		this->ecc.hwctl(&mtd, NAND_ECC_READ);
		this->read_buf(&mtd, p, eccsize);
		this->ecc.calculate(&mtd, p, &ecc_calc[i]);
		this->ecc.correct(&mtd, p, &ecc_code[i], &ecc_calc[i]);
	}

	return 0;
}
#else
static int nand_read_page(int block, int page, void *dst)
{
	struct nand_chip *this = mtd.priv;
	u_char ecc_calc[ECCTOTAL];
	u_char ecc_code[ECCTOTAL];
	u_char oob_data[CONFIG_SYS_NAND_OOBSIZE];
	int i;
	int eccsize = CONFIG_SYS_NAND_ECCSIZE;
	int eccbytes = CONFIG_SYS_NAND_ECCBYTES;
	int eccsteps = ECCSTEPS;
	uint8_t *p = dst;

	nand_command(block, page, 0, NAND_CMD_READ0);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		if (this->ecc.mode != NAND_ECC_SOFT)
			this->ecc.hwctl(&mtd, NAND_ECC_READ);
		this->read_buf(&mtd, p, eccsize);
		this->ecc.calculate(&mtd, p, &ecc_calc[i]);
	}
	this->read_buf(&mtd, oob_data, CONFIG_SYS_NAND_OOBSIZE);

	/* Pick the ECC bytes out of the oob data */
	for (i = 0; i < ECCTOTAL; i++)
		ecc_code[i] = oob_data[nand_ecc_pos[i]];

	eccsteps = ECCSTEPS;
	p = dst;

	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		/* No chance to do something with the possible error message
		 * from correct_data(). We just hope that all possible errors
		 * are corrected by this routine.
		 */
		this->ecc.correct(&mtd, p, &ecc_code[i], &ecc_calc[i]);
	}

	return 0;
}
#endif

#else
#define MAX_OOBSIZE 	32
static int nand_is_bad_block(int block)
{
	struct nand_chip *this = mtd.priv;
	int page = block *  (mtd.erasesize / mtd.writesize);
	static u_char oob[MAX_OOBSIZE];

	this->cmdfunc(&mtd, NAND_CMD_READOOB, 0, page);
	this->read_buf(&mtd, oob, 32);

	/*
	 * Read one byte (or two if it's a 16 bit chip).
	 */
	if (this->options & NAND_BUSWIDTH_16) {
		unsigned short oob_w = (oob[1] << 8) | oob[0];
		if (oob_w != 0xffff)
			return 1;
	} else {
		if (oob[0] != 0xff)
			return 1;
	}

	return 0;
}

#ifdef CONFIG_RDA_FPGA
void nand_dirty_block_erase(int page)
{
	struct nand_chip *this = mtd.priv;
	/* Send commands to erase a block */
	this->cmdfunc(&mtd, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc(&mtd, NAND_CMD_ERASE2, -1, -1);
}
#endif

static int nand_read_page(int block, int page, uchar *dst)
{
	struct nand_chip *this = mtd.priv;
	unsigned int page_cnt = mtd.erasesize / mtd.writesize;
	int page_addr = page + block * page_cnt;

	this->cmdfunc(&mtd, NAND_CMD_READ0, 0, page_addr);
	this->read_buf(&mtd, dst, mtd.writesize);

	return 0;
}
#endif //CONFIG_NAND_RDA

int nand_spl_read_skip_bad(uint64_t offs, uint64_t size, u8 *dst,
					u32 *skip_blocks)
{
	unsigned int block, lastblock;
	unsigned int page;
	unsigned int page_count = 0, page_num;
	unsigned int  bad = 0;
	unsigned int page_offset = offs % mtd.writesize;
	unsigned int data_valid_len = mtd.writesize - page_offset;

	if (page_offset) {/* u-boot is not page aligned */
		block = offs / mtd.erasesize;
		page = (offs / mtd.writesize) % (mtd.erasesize / mtd.writesize);
		do {
			if (!nand_is_bad_block(block)) {
				nand_read_page(block, page, dst);
				memcpy(dst, dst + page_offset, data_valid_len);
				dst += data_valid_len;
				offs += data_valid_len;
				size -= data_valid_len;
			} else {
				bad++;
				block++;
			}
			page_offset = offs % mtd.writesize;
		} while(bad < 16 && page_offset);
	}

	/*
	 * offs has to be aligned to a page address!
	 */
	block = offs / mtd.erasesize;
	lastblock = (offs + size + mtd.erasesize - 1) / mtd.erasesize;
	page = (offs / mtd.writesize) % (mtd.erasesize / mtd.writesize);
	page_num = (size + mtd.writesize - 1) / mtd.writesize;

	while (block < lastblock) {
		if (!nand_is_bad_block(block)) {
			/*
			 * Skip bad blocks
			 */
			while (page < (mtd.erasesize / mtd.writesize)) {
				nand_read_page(block, page, dst);
				dst += mtd.writesize;
				page++;
				page_count++;
				if(page_count >= page_num)
					goto exit;
			}
			page = 0;
		} else {
			bad++;
			if (bad >= 16) {
				*skip_blocks = bad;
				printf("too many bad blocks\n");
				return -1;
			}
			lastblock++;
		}

		block++;
	}
exit:

	*skip_blocks = bad;

	if (0 == page_count){
		printf("Error:read 0 page from nand \n");
		return -1;
	}

	return 0;
}

int nand_spl_load_image(uint64_t offs, uint64_t size, void *dst)
{
	u32 bad_blocks = 0;
	struct rda_nand_info *info = nand_chip.priv;

	printf("nand_spl_load_image: offs = 0x%llx, size = 0x%llx \n", offs, size);

	offs = ((offs - CONFIG_MTD_PTBL_SIZE) * info->spl_adjust_ratio) / 2 + CONFIG_MTD_PTBL_SIZE;
	//size -= (CONFIG_SPL_MAX_SIZE * info->spl_adjust_ratio) / 2 - CONFIG_SPL_MAX_SIZE;

	printf("nand_spl_load_image,adjust: offs = 0x%llx, size = 0x%llx \n", offs, size);

	if(nand_spl_read_skip_bad(offs, size, dst, &bad_blocks)) {
		printf("fatal error, cannot load bootload....\n");
		return -1;
	}

	if (bad_blocks > 0) {
		printf("spl, there are %d bad block in bootloader partition",
			bad_blocks);
	}
	return 0;
}

void nand_spl_mtd_info(u32 *page_size, u32 *block_size)
{
	*page_size = mtd.writesize;
	*block_size = mtd.erasesize;
}

static int nand_sanity_check(void)
{
	int ret = 0;

#if defined(NAND_PAGE_SIZE) && defined(NAND_BLOCK_SIZE)
	if (mtd.writesize != NAND_PAGE_SIZE)
		ret = -1;
	if (mtd.erasesize != NAND_BLOCK_SIZE)
		ret = -1;
	if (ret) {
		printf("nand parameters error: real pagesize: %d, blocksize %d\n",
				mtd.writesize, mtd.erasesize);
		printf("but We expect pagesize: %d, blocksize %d\n",
				NAND_PAGE_SIZE, NAND_BLOCK_SIZE);
		printf("please check device/rda/.../customer.mk\n");
	}
#endif
	return ret;
}
/* nand_init() - initialize data to make nand usable by SPL */
void nand_init(void)
{
	/*
	 * Init board specific nand support
	 */
	mtd.priv = &nand_chip;
	nand_chip.IO_ADDR_R = nand_chip.IO_ADDR_W =
		(void  __iomem *)CONFIG_SYS_NAND_BASE;
	board_nand_init(&nand_chip);

	if (nand_chip.init_size)
		nand_chip.init_size(&mtd, &nand_chip, NULL);

	if (nand_chip.select_chip)
		nand_chip.select_chip(&mtd, 0);

	if(nand_sanity_check()) {
		for(;;);
	}
}

/* Unselect after operation */
void nand_deselect(void)
{
	if (nand_chip.select_chip)
		nand_chip.select_chip(&mtd, -1);
}
