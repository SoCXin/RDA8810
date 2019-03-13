
/*
 * Rda nftl core
 *
 * (C) 2010 10
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/mutex.h>

#include "rnftl.h"

int rda_nftl_badblock_handle(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t phy_blk_addr, addr_blk_t logic_blk_addr)
{
    struct rda_nftl_wl_t *rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
    struct phyblk_node_t *phy_blk_node, *phy_blk_node_new;
    addr_blk_t phy_blk_addr_new, src_page, dest_page;
    int status, i, retry_cnt_map = 0, retry_cnt_copy = 0;

RETRY:
    if ((rda_nftl_wl->free_root.count < (rda_nftl_info->fillfactor / 2)) && (!rda_nftl_wl->erased_root.count) && (rda_nftl_wl->wait_gc_block < 0))
    {
    	rda_nftl_wl->garbage_collect(rda_nftl_wl, 0);  
  	}
	
    status = rda_nftl_wl->get_best_free(rda_nftl_wl, &phy_blk_addr_new);
    if (status) {
        status = rda_nftl_wl->garbage_collect(rda_nftl_wl, DO_COPY_PAGE);
        if (status == 0) {
            rda_nftl_dbg("%s line:%d nftl couldn`t found free block: %d %d\n", __func__, __LINE__, rda_nftl_wl->free_root.count, rda_nftl_wl->wait_gc_block);
            return -ENOENT;
        }
        status = rda_nftl_wl->get_best_free(rda_nftl_wl, &phy_blk_addr_new);
        if (status)
        {
            rda_nftl_dbg("%s line:%d nftl couldn`t get best free block: %d %d\n", __func__, __LINE__, rda_nftl_wl->free_root.count, rda_nftl_wl->wait_gc_block);
            return -ENOENT;
        }
    }

    rda_nftl_add_node(rda_nftl_info, logic_blk_addr, phy_blk_addr_new);
    rda_nftl_wl->add_used(rda_nftl_wl, phy_blk_addr_new);
    phy_blk_node_new = &rda_nftl_info->phypmt[phy_blk_addr_new];
    status = rda_nftl_info->get_phy_sect_map(rda_nftl_info, phy_blk_addr_new);
    if(status)
    {
		rda_nftl_dbg("%s line:%d nftl couldn`t get block sectmap: block: %d, retry_cnt_map:%d\n", __func__, __LINE__, phy_blk_addr_new, retry_cnt_map);
		rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr_new);
		if(retry_cnt_map++ < 3)
			goto RETRY;
		else
			return -ENOENT;
    }

	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];
    for(i=0; i<rda_nftl_wl->pages_per_blk; i++)
    {
        src_page = phy_blk_node->phy_page_map[i];
        if (src_page < 0)
            continue;

        dest_page = phy_blk_node_new->last_write + 1;
        if(rda_nftl_info->copy_page(rda_nftl_info, phy_blk_addr_new, dest_page, phy_blk_addr, src_page) == 2)
        {
			rda_nftl_dbg("%s line:%d nftl copy block write failed block: %d, retry_cnt_copy:%d\n", __func__, __LINE__, phy_blk_addr_new, retry_cnt_copy);
			rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr_new);
			if(retry_cnt_copy++ < 3)
				goto RETRY;
			else
            	return -ENOENT;
        }
    }
    rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr);

	return phy_blk_addr_new;        
}

int rda_nftl_check_node(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr)
{
	struct rda_nftl_wl_t *rda_nftl_wl;
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_free;
	addr_blk_t phy_blk_num, vt_blk_num;
	//int16_t valid_page[MAX_BLK_NUM_PER_NODE+2];
	int16_t	*valid_page;
	int k, node_length, node_len_actual, node_len_max, status = 0;

	rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	vt_blk_num = blk_addr;
	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
	if (vt_blk_node == NULL)
		return -ENOMEM;

	if (vt_blk_num < NFTL_FAT_TABLE_NUM)
		node_len_max = (MAX_BLK_NUM_PER_NODE + 2);
	else
		node_len_max = (MAX_BLK_NUM_PER_NODE + 1);

	node_len_actual = rda_nftl_get_node_length(rda_nftl_info, vt_blk_node);
	node_len_actual = (node_len_actual > (rda_nftl_info->accessibleblocks-1)) ? (rda_nftl_info->accessibleblocks-1) : node_len_actual;

	if(node_len_actual >node_len_max) {	
		valid_page = rda_nftl_malloc(sizeof(int16_t) * (node_len_actual + 2));

		if(valid_page == NULL){
			rda_nftl_dbg("have no mem for valid_page, at blk_addr:%d,  node_len_actual:%d!!!!\n", blk_addr, node_len_actual);
			return -ENOMEM;
		}	
		memset((unsigned char *)valid_page, 0x0, sizeof(int16_t)*(node_len_actual+2));
	}
	else {
		valid_page = rda_nftl_malloc(sizeof(int16_t) * (node_len_max + 2));

		if(valid_page == NULL){
			rda_nftl_dbg("have no mem for valid_page, at blk_addr:%d,  node_len_actual:%d!!!!\n", blk_addr, node_len_max);
			return -ENOMEM;
		}	
		memset((unsigned char *)valid_page, 0x0, sizeof(int16_t)*(node_len_max+2));
	}

	
#if 0  //removed unused printk	
	if (node_len_actual > node_len_max) {
		rda_nftl_dbg("%s Line:%d blk_addr:%d  have node length over MAX, and node_len_actual:%d\n", __func__, __LINE__, blk_addr, node_len_actual);
#if 0		
		node_length = 0;
		while (vt_blk_node != NULL) {
			node_length++;
			if ((vt_blk_node != NULL) && (node_length > node_len_max)) {
				vt_blk_node_free = vt_blk_node->next;
				if (vt_blk_node_free != NULL) {
					rda_nftl_wl->add_free(rda_nftl_wl, vt_blk_node_free->phy_blk_addr);
					vt_blk_node->next = vt_blk_node_free->next;
					rda_nftl_free(vt_blk_node_free);
					continue;
				}
			}
			vt_blk_node = vt_blk_node->next;
		}
#endif		
	}
#endif

	
	for (k=0; k<rda_nftl_info->pages_per_blk; k++) {

		node_length = 0;
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		while (vt_blk_node != NULL) {

			phy_blk_num = vt_blk_node->phy_blk_addr;
			phy_blk_node = &rda_nftl_info->phypmt[phy_blk_num];
			if (!(phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8))))
				break;
			rda_nftl_info->get_phy_sect_map(rda_nftl_info, phy_blk_num);
			if ((phy_blk_node->phy_page_map[k] >= 0) && (phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8)))) {
				valid_page[node_length]++;
				break;
			}
			node_length++;
			vt_blk_node = vt_blk_node->next;
		}
	}

	//rda_nftl_dbg("NFTL check node logic blk: %d  %d %d %d %d %d\n", vt_blk_num, valid_page[0], valid_page[1], valid_page[2], valid_page[3], valid_page[4]);
	node_length = 0;
	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
	while (vt_blk_node->next != NULL) {
		node_length++;
		if (valid_page[node_length] == 0) {
			vt_blk_node_free = vt_blk_node->next;
			rda_nftl_wl->add_free(rda_nftl_wl, vt_blk_node_free->phy_blk_addr);
			vt_blk_node->next = vt_blk_node_free->next;
			rda_nftl_free(vt_blk_node_free);
			continue;
		}

		if (vt_blk_node->next != NULL)
			vt_blk_node = vt_blk_node->next;
	}

	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
	phy_blk_num = vt_blk_node->phy_blk_addr;
	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_num];
	node_length = rda_nftl_get_node_length(rda_nftl_info, vt_blk_node);
	if ((node_length == node_len_max) && (valid_page[node_len_max-1] == (rda_nftl_info->pages_per_blk - (phy_blk_node->last_write + 1)))) {
		status = RDA_NFTL_STRUCTURE_FULL;
	}

	rda_nftl_free(valid_page);

	return status;
}

static void rda_nftl_update_sectmap(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t phy_blk_addr, addr_page_t logic_page_addr, addr_page_t phy_page_addr)
{
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];

	phy_blk_node->valid_sects++;
	phy_blk_node->phy_page_map[logic_page_addr] = phy_page_addr;
	phy_blk_node->phy_page_delete[logic_page_addr>>3] |= (1 << (logic_page_addr % 8));
	phy_blk_node->last_write = phy_page_addr;
}

static int rda_nftl_write_page(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr,
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	int status;
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;	

	status = rda_nftl_ops->write_page(rda_nftl_info, blk_addr, page_addr, data_buf, nftl_oob_buf, oob_len);
	if (status)
		return status;

	rda_nftl_update_sectmap(rda_nftl_info, blk_addr, nftl_oob_info->sect, page_addr);
	return 0;
}

static int rda_nftl_read_page(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr,
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;

	return rda_nftl_ops->read_page(rda_nftl_info, blk_addr, page_addr, data_buf, nftl_oob_buf, oob_len);
}

static int rda_nftl_copy_page(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t dest_blk_addr, addr_page_t dest_page, 
				addr_blk_t src_blk_addr, addr_page_t src_page)
{
	int status = 0, oob_len;
	unsigned char *nftl_data_buf;
	unsigned char nftl_oob_buf[rda_nftl_info->oobsize];
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	struct phyblk_node_t *phy_blk_node = &rda_nftl_info->phypmt[dest_blk_addr];

	oob_len = min(rda_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC)));
	nftl_data_buf = rda_nftl_info->copy_page_buf;
	status = rda_nftl_info->read_page(rda_nftl_info, src_blk_addr, src_page, nftl_data_buf, nftl_oob_buf, oob_len);
	if (status) {
		rda_nftl_dbg("copy page read failed: %d %d status: %d\n", src_blk_addr, src_page, status);
		status = 1;
		goto exit;
	}

	if (rda_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC))) {
		if (memcmp((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), RDA_NFTL_MAGIC, strlen(RDA_NFTL_MAGIC))) {
			rda_nftl_dbg("nftl read invalid magic vtblk: %d %d \n", nftl_oob_info->vtblk, src_blk_addr);
		}
	}
	nftl_oob_info->ec = phy_blk_node->ec;
	nftl_oob_info->timestamp = phy_blk_node->timestamp;
	nftl_oob_info->status_page = 1;
	status = rda_nftl_info->write_page(rda_nftl_info, dest_blk_addr, dest_page, nftl_data_buf, nftl_oob_buf, oob_len);
	if (status) {
		rda_nftl_dbg("copy page write failed: %d status: %d\n", dest_blk_addr, status);
		status = 2;
		goto exit;
	}

exit:
	return status;
}

static int rda_nftl_get_page_info(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *nftl_oob_buf, int oob_len)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;

	return rda_nftl_ops->read_page_oob(rda_nftl_info, blk_addr, page_addr, nftl_oob_buf, oob_len);	
}

static int rda_nftl_blk_isbad(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;

	return rda_nftl_ops->blk_isbad(rda_nftl_info, blk_addr);
}

static int rda_nftl_blk_mark_bad(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t blk_addr)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;
	struct phyblk_node_t *phy_blk_node = &rda_nftl_info->phypmt[blk_addr];
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_prev;

	if (phy_blk_node->vtblk >= 0) {
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + phy_blk_node->vtblk));
		do {
			vt_blk_node_prev = vt_blk_node;
			vt_blk_node = vt_blk_node->next;

			if ((vt_blk_node != NULL) && (vt_blk_node->phy_blk_addr == blk_addr)) {
				vt_blk_node_prev->next = vt_blk_node->next;
				rda_nftl_free(vt_blk_node);
				vt_blk_node = NULL; //NULL for free				
			}
			else if ((vt_blk_node_prev != NULL) && (vt_blk_node_prev->phy_blk_addr == blk_addr)) {
				if (*(rda_nftl_info->vtpmt + phy_blk_node->vtblk) == vt_blk_node_prev)
					*(rda_nftl_info->vtpmt + phy_blk_node->vtblk) = vt_blk_node;

				rda_nftl_free(vt_blk_node_prev);
			}

		} while (vt_blk_node != NULL);
	}
	memset((unsigned char *)phy_blk_node, 0xff, sizeof(struct phyblk_node_t));
	phy_blk_node->status_page = STATUS_BAD_BLOCK;

	return rda_nftl_ops->blk_mark_bad(rda_nftl_info, blk_addr);	
}

static int rda_nftl_creat_sectmap(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t phy_blk_addr)
{
	int i, status;
	uint32_t page_per_blk;
	int16_t valid_sects = 0;
	struct phyblk_node_t *phy_blk_node;
	struct nftl_oobinfo_t *nftl_oob_info;
	//unsigned char nftl_oob_buf[sizeof(struct nftl_oobinfo_t)];
	unsigned char *nftl_oob_buf;
	nftl_oob_buf = rda_nftl_malloc(sizeof(struct nftl_oobinfo_t));
	
	if(nftl_oob_buf== NULL){
		printk("%s,%d malloc failed\n",__func__,__LINE__);
		return -ENOMEM;;
	}
	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;

	page_per_blk = rda_nftl_info->pages_per_blk;
	for (i=0; i<page_per_blk; i++) {

		if (i == 0) {
    		status = rda_nftl_info->get_page_info(rda_nftl_info, phy_blk_addr, i, nftl_oob_buf, sizeof(struct nftl_oobinfo_t));
    		if (status) {
    			rda_nftl_dbg("nftl creat sect map faile at: %d\n", phy_blk_addr);
				rda_nftl_free(nftl_oob_buf);
    			return RDA_NFTL_FAILURE;
    		}		    
			phy_blk_node->ec = nftl_oob_info->ec;
			phy_blk_node->vtblk = nftl_oob_info->vtblk;
			phy_blk_node->timestamp = nftl_oob_info->timestamp;
		}
		else{
            status = rda_nftl_info->get_page_info(rda_nftl_info, phy_blk_addr, i, nftl_oob_buf, sizeof(addr_sect_t));
    		if (status) {
    			rda_nftl_dbg("nftl creat sect map faile at: %d\n", phy_blk_addr);
				rda_nftl_free(nftl_oob_buf);
    			return RDA_NFTL_FAILURE;
    		}		    
		}
		    
		if (nftl_oob_info->sect >= 0) {
			phy_blk_node->phy_page_map[nftl_oob_info->sect] = i;
			phy_blk_node->last_write = i;
			valid_sects++;
		}
		else
			break;
	}
	phy_blk_node->valid_sects = valid_sects;
	rda_nftl_free(nftl_oob_buf);
	return 0;
}

static int rda_nftl_get_phy_sect_map(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr)
{
	int status;
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &rda_nftl_info->phypmt[blk_addr];

	if (phy_blk_node->valid_sects < 0) {
		status = rda_nftl_creat_sectmap(rda_nftl_info, blk_addr);
		if (status)
			return RDA_NFTL_FAILURE;
	}

	return 0;	
}

static void rda_nftl_erase_sect_map(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr)
{
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_prev;
	phy_blk_node = &rda_nftl_info->phypmt[blk_addr];

	if (phy_blk_node->vtblk >= 0) {
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + phy_blk_node->vtblk));
		do {
			vt_blk_node_prev = vt_blk_node;
			vt_blk_node = vt_blk_node->next;

			if ((vt_blk_node != NULL) && (vt_blk_node->phy_blk_addr == blk_addr)) {
				rda_nftl_dbg("free blk had mapped vt blk: %d phy blk: %d\n", phy_blk_node->vtblk, blk_addr);
				vt_blk_node_prev->next = vt_blk_node->next;
				rda_nftl_free(vt_blk_node);
				vt_blk_node = NULL; //NULL for free
			}
			else if ((vt_blk_node_prev != NULL) && (vt_blk_node_prev->phy_blk_addr == blk_addr)) {
				rda_nftl_dbg("free blk had mapped vt blk: %d phy blk: %d\n", phy_blk_node->vtblk, blk_addr);
				if (*(rda_nftl_info->vtpmt + phy_blk_node->vtblk) == vt_blk_node_prev)
					*(rda_nftl_info->vtpmt + phy_blk_node->vtblk) = vt_blk_node;

				rda_nftl_free(vt_blk_node_prev);
			}

		} while (vt_blk_node != NULL);
	}

	phy_blk_node->ec++;
	phy_blk_node->valid_sects = 0;
	phy_blk_node->vtblk = BLOCK_INIT_VALUE;
	phy_blk_node->last_write = BLOCK_INIT_VALUE;
	phy_blk_node->timestamp = MAX_TIMESTAMP_NUM;
	memset(phy_blk_node->phy_page_map, 0xff, (sizeof(addr_sect_t)*MAX_PAGES_IN_BLOCK));
	memset(phy_blk_node->phy_page_delete, 0xff, (MAX_PAGES_IN_BLOCK>>3));

	return;
}

static int rda_nftl_erase_block(struct rda_nftl_info_t * rda_nftl_info, addr_blk_t blk_addr)
{
	struct rda_nftl_ops_t *rda_nftl_ops = rda_nftl_info->rda_nftl_ops;
	int status;

	status = rda_nftl_ops->erase_block(rda_nftl_info, blk_addr);
	if (status)
		return RDA_NFTL_FAILURE;

	rda_nftl_erase_sect_map(rda_nftl_info, blk_addr);
	return 0;
}

int rda_nftl_add_node(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t logic_blk_addr, addr_blk_t phy_blk_addr)
{
	struct phyblk_node_t *phy_blk_node_curt, *phy_blk_node_add;
	struct vtblk_node_t  *vt_blk_node_curt, *vt_blk_node_add;
	struct rda_nftl_wl_t *rda_nftl_wl;
	addr_blk_t phy_blk_cur;
	int status = 0;

	rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	vt_blk_node_add = (struct vtblk_node_t *)rda_nftl_malloc(sizeof(struct vtblk_node_t));
	if (vt_blk_node_add == NULL)
		return RDA_NFTL_FAILURE;

	vt_blk_node_add->phy_blk_addr = phy_blk_addr;
	vt_blk_node_add->next = NULL;
	phy_blk_node_add = &rda_nftl_info->phypmt[phy_blk_addr];
	phy_blk_node_add->vtblk = logic_blk_addr;
	vt_blk_node_curt = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + logic_blk_addr));

	if (vt_blk_node_curt == NULL) {
		phy_blk_node_add->timestamp = 0;
	}
	else {
		phy_blk_cur = vt_blk_node_curt->phy_blk_addr;
		phy_blk_node_curt = &rda_nftl_info->phypmt[phy_blk_cur];
		if (phy_blk_node_curt->timestamp >= MAX_TIMESTAMP_NUM)
			phy_blk_node_add->timestamp = 0;
		else
			phy_blk_node_add->timestamp = phy_blk_node_curt->timestamp + 1;
		vt_blk_node_add->next = vt_blk_node_curt;

	}
	*(rda_nftl_info->vtpmt + logic_blk_addr) = vt_blk_node_add;

	return status;
}

static int rda_nftl_calculate_last_write(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t phy_blk_addr)
{
	int status;
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];

	if (phy_blk_node->valid_sects < 0) {
		status = rda_nftl_creat_sectmap(rda_nftl_info, phy_blk_addr);
		if (status)
			return RDA_NFTL_FAILURE;
	}

	return 0;
}

static int rda_nftl_get_valid_pos(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t logic_blk_addr, addr_blk_t *phy_blk_addr,
									 addr_page_t logic_page_addr, addr_page_t *phy_page_addr, uint8_t flag )
{
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node;
	int status;
	uint32_t page_per_blk;

	page_per_blk = rda_nftl_info->pages_per_blk;
	*phy_blk_addr = BLOCK_INIT_VALUE;
	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + logic_blk_addr));
	if (vt_blk_node == NULL)
		return RDA_NFTL_BLKNOTFOUND;

	*phy_blk_addr = vt_blk_node->phy_blk_addr;
	phy_blk_node = &rda_nftl_info->phypmt[*phy_blk_addr];
	status = rda_nftl_get_phy_sect_map(rda_nftl_info, *phy_blk_addr);
	if (status)
		return RDA_NFTL_FAILURE;

	if (flag == WRITE_OPERATION) {
		if (phy_blk_node->last_write < 0)
			rda_nftl_calculate_last_write(rda_nftl_info, *phy_blk_addr);

		*phy_page_addr = phy_blk_node->last_write + 1;
		if (*phy_page_addr >= page_per_blk)
			return RDA_NFTL_PAGENOTFOUND;

		return 0;
	}
	else if (flag == READ_OPERATION) {
		do {

			*phy_blk_addr = vt_blk_node->phy_blk_addr;
			phy_blk_node = &rda_nftl_info->phypmt[*phy_blk_addr];

			status = rda_nftl_get_phy_sect_map(rda_nftl_info, *phy_blk_addr);
			if (status)
				return RDA_NFTL_FAILURE;

			if (phy_blk_node->phy_page_map[logic_page_addr] >= 0) {
				*phy_page_addr = phy_blk_node->phy_page_map[logic_page_addr];
				return 0;
			}

			vt_blk_node = vt_blk_node->next;
		} while (vt_blk_node != NULL);

		return RDA_NFTL_PAGENOTFOUND;
	}

	return RDA_NFTL_FAILURE;
}

static int rda_nftl_read_sect(struct rda_nftl_info_t *rda_nftl_info, addr_page_t sect_addr, unsigned char *buf)
{
	uint32_t page_per_blk;
	addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
	int status = 0;

	page_per_blk = rda_nftl_info->pages_per_blk;
	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;

	status = rda_nftl_get_valid_pos(rda_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, READ_OPERATION);
	if ((status == RDA_NFTL_PAGENOTFOUND) || (status == RDA_NFTL_BLKNOTFOUND)) {
		memset(buf, 0xff, rda_nftl_info->writesize);
		return 0;
	}

	if (status == RDA_NFTL_FAILURE)
	{
		rda_nftl_badblock_handle(rda_nftl_info, phy_blk_addr, logic_blk_addr);
        return RDA_NFTL_FAILURE;
	}

	status = rda_nftl_info->read_page(rda_nftl_info, phy_blk_addr, phy_page_addr, buf, NULL, 0);
	if (status)
	{
		rda_nftl_badblock_handle(rda_nftl_info, phy_blk_addr, logic_blk_addr);
		return status;
	}

	return 0;
}

static void rda_nftl_delete_sect(struct rda_nftl_info_t *rda_nftl_info, addr_page_t sect_addr)
{
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	uint32_t page_per_blk;
	addr_page_t logic_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;

	page_per_blk = rda_nftl_info->pages_per_blk;
	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;
	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + logic_blk_addr));
	if (vt_blk_node == NULL)
		return;

	while (vt_blk_node != NULL) {

		phy_blk_addr = vt_blk_node->phy_blk_addr;
		phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];
		phy_blk_node->phy_page_delete[logic_page_addr>>3] &= (~(1 << (logic_page_addr % 8)));
		vt_blk_node = vt_blk_node->next;
	}
	return;
}

static int rda_nftl_write_sect(struct rda_nftl_info_t *rda_nftl_info, addr_page_t sect_addr, unsigned char *buf)
{
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	int status = 0, special_gc = 0, oob_len, retry_cnt = 0;
	struct rda_nftl_wl_t *rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	uint32_t page_per_blk;
	addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
	unsigned char nftl_oob_buf[rda_nftl_info->oobsize];
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;

	page_per_blk = rda_nftl_info->pages_per_blk;

	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;
	rda_nftl_info->current_write_block = logic_blk_addr;
	if (rda_nftl_wl->wait_gc_block >= 0) {
		rda_nftl_info->continue_writed_sects++;
		if (rda_nftl_wl->wait_gc_block == logic_blk_addr) {
			rda_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;
			rda_nftl_wl->garbage_collect(rda_nftl_wl, 0);
			rda_nftl_info->continue_writed_sects = 0;
			rda_nftl_wl->add_gc(rda_nftl_wl, logic_blk_addr);
		}
	}

	status = rda_nftl_get_valid_pos(rda_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, WRITE_OPERATION);
	if (status == RDA_NFTL_FAILURE)
	{
	    rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr);
	    rda_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status);
	    status = RDA_NFTL_BLKNOTFOUND;
            //return RDA_NFTL_FAILURE;
   	}

	if ((status == RDA_NFTL_PAGENOTFOUND) || (status == RDA_NFTL_BLKNOTFOUND)) {

		if ((rda_nftl_wl->free_root.count < (rda_nftl_info->fillfactor / 2)) && (!rda_nftl_wl->erased_root.count) && (rda_nftl_wl->wait_gc_block < 0))
			rda_nftl_wl->garbage_collect(rda_nftl_wl, 0);

		status = rda_nftl_wl->get_best_free(rda_nftl_wl, &phy_blk_addr);
		if (status) {
			status = rda_nftl_wl->garbage_collect(rda_nftl_wl, DO_COPY_PAGE);
			if (status == 0) {
				rda_nftl_dbg("nftl couldn`t found free block: %d %d\n", rda_nftl_wl->free_root.count, rda_nftl_wl->wait_gc_block);
				return -ENOENT;
			}
			status = rda_nftl_wl->get_best_free(rda_nftl_wl, &phy_blk_addr);
			if (status)
				return status;
		}

		rda_nftl_add_node(rda_nftl_info, logic_blk_addr, phy_blk_addr);
		rda_nftl_wl->add_used(rda_nftl_wl, phy_blk_addr);
		phy_page_addr = 0;
	}

WRITE_RETRY:
	phy_blk_node = &rda_nftl_info->phypmt[phy_blk_addr];
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	nftl_oob_info->ec = phy_blk_node->ec;
	nftl_oob_info->vtblk = logic_blk_addr;
	nftl_oob_info->timestamp = phy_blk_node->timestamp;
	nftl_oob_info->status_page = 1;
	nftl_oob_info->sect = logic_page_addr;
	oob_len = min(rda_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC)));
	if (rda_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC)))
		memcpy((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), RDA_NFTL_MAGIC, strlen(RDA_NFTL_MAGIC));

	status = rda_nftl_info->write_page(rda_nftl_info, phy_blk_addr, phy_page_addr, buf, nftl_oob_buf, oob_len);
	if (status && (retry_cnt++ < 3)) 
	{  //do not markbad when write failed, just get another block and write again till ok
	    rda_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d, retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status, retry_cnt);
#if 1
		phy_blk_addr = rda_nftl_badblock_handle(rda_nftl_info, phy_blk_addr, logic_blk_addr);
		if(phy_blk_addr >= 0) //sucess
			goto WRITE_RETRY;
		else
		{
			rda_nftl_dbg("%s, line %d, rda_nftl_badblock_handle failed blk: %d retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, retry_cnt);
			return -ENOENT;
		}
#else 
		rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr);
		rda_nftl_dbg("nftl write page faile blk: %d page: %d status: %d\n", phy_blk_addr, phy_page_addr, status);
		return status;
#endif		
	}
	else if(status)
	{
		rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_addr);
		rda_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d, retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status, retry_cnt);
		return status;
	}

	vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + logic_blk_addr));
	if ((rda_nftl_get_node_length(rda_nftl_info, vt_blk_node) > MAX_BLK_NUM_PER_NODE) || (phy_page_addr == (page_per_blk - 1))) {
		status = rda_nftl_check_node(rda_nftl_info, logic_blk_addr);
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + logic_blk_addr));
		if (status == RDA_NFTL_STRUCTURE_FULL) {
			//rda_nftl_dbg("rda nftl structure full at logic : %d phy blk: %d %d \n", logic_blk_addr, phy_blk_addr, phy_blk_node->last_write);
			status = rda_nftl_wl->garbage_one(rda_nftl_wl, logic_blk_addr, 0);
			if (status < 0)
				return status;
			special_gc = 1;
		}
		else if (rda_nftl_get_node_length(rda_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE) {
			rda_nftl_wl->add_gc(rda_nftl_wl, logic_blk_addr);
		}
	}

	if ((rda_nftl_wl->wait_gc_block >= 0) && (!special_gc)) {
		if (((rda_nftl_info->continue_writed_sects % 8) == 0) || (rda_nftl_wl->free_root.count < (rda_nftl_info->fillfactor / 4))) {
			rda_nftl_wl->garbage_collect(rda_nftl_wl, 0);
			if (rda_nftl_wl->wait_gc_block == BLOCK_INIT_VALUE)
				rda_nftl_info->continue_writed_sects = 0;
		}
	}

	return 0;
}

static void rda_nftl_add_block(struct rda_nftl_info_t *rda_nftl_info, addr_blk_t phy_blk, struct nftl_oobinfo_t *nftl_oob_info)
{
	struct phyblk_node_t *phy_blk_node_curt, *phy_blk_node_add;
	struct vtblk_node_t  *vt_blk_node_curt, *vt_blk_node_prev, *vt_blk_node_add;

	vt_blk_node_add = (struct vtblk_node_t *)rda_nftl_malloc(sizeof(struct vtblk_node_t));
	if (vt_blk_node_add == NULL)
		return;
	vt_blk_node_add->phy_blk_addr = phy_blk;
	vt_blk_node_add->next = NULL;
	phy_blk_node_add = &rda_nftl_info->phypmt[phy_blk];
	phy_blk_node_add->ec = nftl_oob_info->ec;
	phy_blk_node_add->vtblk = nftl_oob_info->vtblk;
	phy_blk_node_add->timestamp = nftl_oob_info->timestamp;
	vt_blk_node_curt = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk));

	if (vt_blk_node_curt == NULL) {
		vt_blk_node_curt = vt_blk_node_add;
		*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_curt;
		return;
	}

	vt_blk_node_prev = vt_blk_node_curt;
	while(vt_blk_node_curt != NULL) {

		phy_blk_node_curt = &rda_nftl_info->phypmt[vt_blk_node_curt->phy_blk_addr];
		if (((phy_blk_node_add->timestamp > phy_blk_node_curt->timestamp)
			 && ((phy_blk_node_add->timestamp - phy_blk_node_curt->timestamp) < (MAX_TIMESTAMP_NUM - rda_nftl_info->accessibleblocks)))
			|| ((phy_blk_node_add->timestamp < phy_blk_node_curt->timestamp)
			 && ((phy_blk_node_curt->timestamp - phy_blk_node_add->timestamp) >= (MAX_TIMESTAMP_NUM - rda_nftl_info->accessibleblocks)))) {

			vt_blk_node_add->next = vt_blk_node_curt;
			if (*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk) == vt_blk_node_curt)
				*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_add;
			else
				vt_blk_node_prev->next = vt_blk_node_add;
			break;
		}
		else if (phy_blk_node_add->timestamp == phy_blk_node_curt->timestamp) {
			rda_nftl_dbg("NFTL timestamp err logic blk: %d phy blk: %d %d\n", nftl_oob_info->vtblk, vt_blk_node_curt->phy_blk_addr, vt_blk_node_add->phy_blk_addr);
			if (phy_blk_node_add->ec < phy_blk_node_curt->ec) {

				vt_blk_node_prev->next = vt_blk_node_add;
				vt_blk_node_add->next = vt_blk_node_curt->next;
				vt_blk_node_add = vt_blk_node_curt;
			}
			vt_blk_node_curt = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk));
			vt_blk_node_add->next = vt_blk_node_curt;
			*(rda_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_add;
			break;
		}
		else {
			if (vt_blk_node_curt->next != NULL) {
				vt_blk_node_prev = vt_blk_node_curt;
				vt_blk_node_curt = vt_blk_node_curt->next;
			}
			else {
				vt_blk_node_curt->next = vt_blk_node_add;
				vt_blk_node_curt = vt_blk_node_curt->next;
				break;
			}
		}
	}

	return;
}

static void rda_nftl_check_conflict_node(struct rda_nftl_info_t *rda_nftl_info)
{
	struct vtblk_node_t  *vt_blk_node;
	struct rda_nftl_wl_t *rda_nftl_wl;
	addr_blk_t vt_blk_num;
	int node_length = 0, status = 0;
	struct timespec ts_check_start, ts_check_current;

	ktime_get_ts(&ts_check_start);
	rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	for (vt_blk_num=0; vt_blk_num<rda_nftl_info->accessibleblocks; vt_blk_num++) {

		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		if (vt_blk_node == NULL)
			continue;

		node_length = rda_nftl_get_node_length(rda_nftl_info, vt_blk_node);
		if (node_length < MAX_BLK_NUM_PER_NODE/* BASIC_BLK_NUM_PER_NODE */)
			continue;
			
        //rda_nftl_dbg("need check conflict node vt blk: %d and node_length:%d\n", vt_blk_num, node_length);
        
		status = rda_nftl_check_node(rda_nftl_info, vt_blk_num);
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		if (rda_nftl_get_node_length(rda_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE)
			rda_nftl_wl->add_gc(rda_nftl_wl, vt_blk_num);
		if (status == RDA_NFTL_STRUCTURE_FULL) {
			rda_nftl_dbg("found conflict node vt blk: %d \n", vt_blk_num);
			rda_nftl_wl->garbage_one(rda_nftl_wl, vt_blk_num, 0);
		}
		if (vt_blk_num >= (rda_nftl_info->accessibleblocks / 2)) {
			ktime_get_ts(&ts_check_current);
			if ((ts_check_current.tv_sec - ts_check_start.tv_sec) >= 5) {
				rda_nftl_dbg("check conflict node timeout: %d \n", vt_blk_num);
				break;
			}
		}
	}
//	if (vt_blk_num >= rda_nftl_info->accessibleblocks)
//		rda_nftl_info->isinitialised = 1;
//	rda_nftl_info->cur_split_blk = vt_blk_num;

	return;
}

static void rda_nftl_creat_structure(struct rda_nftl_info_t *rda_nftl_info)
{
	struct vtblk_node_t  *vt_blk_node;
	struct rda_nftl_wl_t *rda_nftl_wl;
	int status = 0;
	addr_blk_t vt_blk_num;

	rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	for (vt_blk_num=rda_nftl_info->cur_split_blk; vt_blk_num<rda_nftl_info->accessibleblocks; vt_blk_num++) {

		if ((vt_blk_num - rda_nftl_info->cur_split_blk) >= DEFAULT_SPLIT_UNIT)
			break;

		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		if (vt_blk_node == NULL)
			continue;

		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		if (rda_nftl_get_node_length(rda_nftl_info, vt_blk_node) < BASIC_BLK_NUM_PER_NODE)
			continue;

		status = rda_nftl_check_node(rda_nftl_info, vt_blk_num);
		vt_blk_node = (struct vtblk_node_t *)(*(rda_nftl_info->vtpmt + vt_blk_num));
		if (rda_nftl_get_node_length(rda_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE)
			rda_nftl_wl->add_gc(rda_nftl_wl, vt_blk_num);
		if (status == RDA_NFTL_STRUCTURE_FULL) {
			rda_nftl_dbg("found conflict node vt blk: %d \n", vt_blk_num);
			rda_nftl_wl->garbage_one(rda_nftl_wl, vt_blk_num, 0);
		}
	}

	rda_nftl_info->cur_split_blk = vt_blk_num;
	if (rda_nftl_info->cur_split_blk >= rda_nftl_info->accessibleblocks) {
		if ((rda_nftl_wl->free_root.count <= DEFAULT_IDLE_FREE_BLK) && (!rda_nftl_wl->erased_root.count))
			rda_nftl_wl->gc_need_flag = 1;

		rda_nftl_info->isinitialised = 1;
		rda_nftl_wl->gc_start_block = rda_nftl_info->accessibleblocks - 1;
		rda_nftl_dbg("nftl creat stucture completely free blk: %d erased blk: %d\n", rda_nftl_wl->free_root.count, rda_nftl_wl->erased_root.count);
	}

	return;
}

static ssize_t show_address_map_table(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    struct rda_nftl_info_t *rda_nftl_info = container_of(class, struct rda_nftl_info_t, cls);
	unsigned int address;
    unsigned blks_per_sect, sect_addr;
    addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
    uint32_t page_per_blk;
    int ret;
    
	ret =  sscanf(buf, "%x", &address);
    blks_per_sect = rda_nftl_info->writesize / 512;
	sect_addr = address / blks_per_sect;

    page_per_blk = rda_nftl_info->pages_per_blk;
    logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;

    ret = rda_nftl_get_valid_pos(rda_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, READ_OPERATION);
	if (ret == RDA_NFTL_FAILURE){
        return RDA_NFTL_FAILURE;
	}

    if ((ret == RDA_NFTL_PAGENOTFOUND) || (ret == RDA_NFTL_BLKNOTFOUND)) {
		printk("the phy address not found\n");
		return 1;
	}

    printk("address %x map phy address:blk addr %x page addr %x\n", address, logic_blk_addr, logic_page_addr);

	return count;
}

static struct class_attribute nftl_class_attrs[] = {
    __ATTR(map_table,  S_IRUGO | S_IWUSR, NULL,    show_address_map_table),
    __ATTR_NULL
};

int rda_nftl_initialize(struct rda_nftl_blk_t *rda_nftl_blk)
{
	struct mtd_info *mtd = rda_nftl_blk->mbd.mtd;
	struct rda_nftl_info_t *rda_nftl_info;
	struct nftl_oobinfo_t *nftl_oob_info;
	struct rda_nftl_wl_t *rda_nftl_wl;
	struct phyblk_node_t *phy_blk_node;
	int error = 0, phy_blk_num, oob_len;
	uint32_t phy_page_addr, size_in_blk;
	uint32_t phys_erase_shift;
	unsigned char nftl_oob_buf[mtd->oobavail];

	if (mtd->oobavail < sizeof(struct nftl_oobinfo_t))
		return -EPERM;
	rda_nftl_info = rda_nftl_malloc(sizeof(struct rda_nftl_info_t));
	if (!rda_nftl_info)
		return -ENOMEM;

	rda_nftl_blk->rda_nftl_info = rda_nftl_info;
	rda_nftl_info->mtd = mtd;
	rda_nftl_info->writesize = mtd->writesize;
	rda_nftl_info->oobsize = mtd->oobavail;
	phys_erase_shift = ffs(mtd->erasesize) - 1;
	size_in_blk =  (mtd->size >> phys_erase_shift);
	if (size_in_blk <= RDA_LIMIT_FACTOR)
		return -EPERM;

	rda_nftl_info->pages_per_blk = mtd->erasesize / mtd->writesize;
	rda_nftl_info->fillfactor = (size_in_blk / 32);
	if (rda_nftl_info->fillfactor < RDA_LIMIT_FACTOR)
		rda_nftl_info->fillfactor = RDA_LIMIT_FACTOR;
	rda_nftl_info->fillfactor += NFTL_FAT_TABLE_NUM * 2* MAX_BLK_NUM_PER_NODE;
	rda_nftl_info->accessibleblocks = size_in_blk - rda_nftl_info->fillfactor;
	rda_nftl_info->current_write_block = BLOCK_INIT_VALUE;

	rda_nftl_info->copy_page_buf = rda_nftl_malloc(rda_nftl_info->writesize);
	if (!rda_nftl_info->copy_page_buf)
		return -ENOMEM;
	rda_nftl_info->phypmt = (struct phyblk_node_t *)rda_nftl_malloc((sizeof(struct phyblk_node_t) * size_in_blk));
	if (!rda_nftl_info->phypmt)
		return -ENOMEM;
	rda_nftl_info->vtpmt = (void **)rda_nftl_malloc((sizeof(void*) * rda_nftl_info->accessibleblocks));
	if (!rda_nftl_info->vtpmt)
		return -ENOMEM;
	memset((unsigned char *)rda_nftl_info->phypmt, 0xff, sizeof(struct phyblk_node_t)*size_in_blk);

	rda_nftl_ops_init(rda_nftl_info);

	rda_nftl_info->read_page = rda_nftl_read_page;
	rda_nftl_info->write_page = rda_nftl_write_page;
	rda_nftl_info->copy_page = rda_nftl_copy_page;
	rda_nftl_info->get_page_info = rda_nftl_get_page_info;
	rda_nftl_info->blk_mark_bad = rda_nftl_blk_mark_bad;
	rda_nftl_info->blk_isbad = rda_nftl_blk_isbad;
	rda_nftl_info->get_phy_sect_map = rda_nftl_get_phy_sect_map;
	rda_nftl_info->erase_block = rda_nftl_erase_block;

	rda_nftl_info->read_sect = rda_nftl_read_sect;
	rda_nftl_info->write_sect = rda_nftl_write_sect;
	rda_nftl_info->delete_sect = rda_nftl_delete_sect;
	rda_nftl_info->creat_structure = rda_nftl_creat_structure;

	error = rda_nftl_wl_init(rda_nftl_info);
	if (error)
		return error;

	rda_nftl_wl = rda_nftl_info->rda_nftl_wl;
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	oob_len = min(rda_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC)));
	for (phy_blk_num=0; phy_blk_num<size_in_blk; phy_blk_num++) {

		phy_page_addr = 0;
		phy_blk_node = &rda_nftl_info->phypmt[phy_blk_num];

		error = rda_nftl_info->blk_isbad(rda_nftl_info, phy_blk_num);
		if (error) {
			rda_nftl_info->accessibleblocks--;
			rda_nftl_dbg("nftl detect bad blk at : %d \n", phy_blk_num);
			continue;
		}

		error = rda_nftl_info->get_page_info(rda_nftl_info, phy_blk_num, phy_page_addr, nftl_oob_buf, oob_len);
		if (error) {
			rda_nftl_info->accessibleblocks--;
			phy_blk_node->status_page = STATUS_BAD_BLOCK;
			rda_nftl_dbg("get status error at blk: %d \n", phy_blk_num);
			continue;
		}

		if (nftl_oob_info->status_page == 0) {
			rda_nftl_info->accessibleblocks--;
			phy_blk_node->status_page = STATUS_BAD_BLOCK;
			rda_nftl_dbg("get status faile at blk: %d \n", phy_blk_num);
			rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_num);
			continue;
		}

		if (nftl_oob_info->vtblk == -1) {
			phy_blk_node->valid_sects = 0;
			phy_blk_node->ec = 0;
			rda_nftl_wl->add_erased(rda_nftl_wl, phy_blk_num);	
		}
		else if ((nftl_oob_info->vtblk < 0) || (nftl_oob_info->vtblk >= (size_in_blk - rda_nftl_info->fillfactor))) {
			rda_nftl_dbg("nftl invalid vtblk: %d \n", nftl_oob_info->vtblk);
			error = rda_nftl_info->erase_block(rda_nftl_info, phy_blk_num);
			if (error) {
				rda_nftl_info->accessibleblocks--;
				phy_blk_node->status_page = STATUS_BAD_BLOCK;
				rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_num);
			}
			else {
				phy_blk_node->valid_sects = 0;
				rda_nftl_wl->add_erased(rda_nftl_wl, phy_blk_num);
			}
		}
		else {
			if (rda_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(RDA_NFTL_MAGIC))) {
				if (memcmp((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), RDA_NFTL_MAGIC, strlen(RDA_NFTL_MAGIC))) {
					rda_nftl_dbg("nftl invalid magic vtblk: %d \n", nftl_oob_info->vtblk);
					error = rda_nftl_info->erase_block(rda_nftl_info, phy_blk_num);
					if (error) {
						rda_nftl_info->accessibleblocks--;
						phy_blk_node->status_page = STATUS_BAD_BLOCK;
						rda_nftl_info->blk_mark_bad(rda_nftl_info, phy_blk_num);
					}
					else {
						phy_blk_node->valid_sects = 0;
						phy_blk_node->ec = 0;
						rda_nftl_wl->add_erased(rda_nftl_wl, phy_blk_num);
					}
					continue;
				}
			}
			rda_nftl_add_block(rda_nftl_info, phy_blk_num, nftl_oob_info);
			rda_nftl_wl->add_used(rda_nftl_wl, phy_blk_num);
		}
	}

	rda_nftl_info->isinitialised = 0;
	rda_nftl_info->cur_split_blk = 0;
	rda_nftl_wl->gc_start_block = rda_nftl_info->accessibleblocks - 1;

	rda_nftl_check_conflict_node(rda_nftl_info);

	rda_nftl_blk->mbd.size = (rda_nftl_info->accessibleblocks * (mtd->erasesize  >> 9));
	rda_nftl_dbg("nftl initilize completely dev size: 0x%lx %d\n", rda_nftl_blk->mbd.size * 512, rda_nftl_wl->free_root.count);

    /*setup class*/
	rda_nftl_info->cls.name = kzalloc(strlen((const char*)RDA_NFTL_MAGIC)+1, GFP_KERNEL);

    strcpy(rda_nftl_info->cls.name, (char*)RDA_NFTL_MAGIC);
    rda_nftl_info->cls.class_attrs = nftl_class_attrs;
   	error = class_register(&rda_nftl_info->cls);
	if(error)
		printk(" class register nand_class fail!\n");

	return 0;
}

void rda_nftl_info_release(struct rda_nftl_info_t *rda_nftl_info)
{
	if (rda_nftl_info->vtpmt)
		rda_nftl_free(rda_nftl_info->vtpmt);
	if (rda_nftl_info->phypmt)
		rda_nftl_free(rda_nftl_info->phypmt);
	if (rda_nftl_info->rda_nftl_wl)
		rda_nftl_free(rda_nftl_info->rda_nftl_wl);
}

