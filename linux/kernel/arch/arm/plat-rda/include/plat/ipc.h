
#ifndef RDA8850E_IPC_H_
#define RDA8850E_IPC_H_

#define RESERVED_LEN	96
#define SMD_BUFFER_LEN  1504

typedef struct {
    u32 next_offset;
    u8 cid;
    u8 simid;
    u16 len;
} ps_header_t;

typedef struct {
    ps_header_t hdr;
    u8 resver[RESERVED_LEN - sizeof(ps_header_t)];
    u8 ipdata[SMD_BUFFER_LEN];
} ps_buffer_t;

#define RDA_UL_IPDATA_BUF_CNT	512	/* size must be power of 2 */
#define RDA_UL_MASK				(RDA_UL_IPDATA_BUF_CNT - 1)
#define RDA_DL_IPDATA_BUF_CNT	512 /* size must be power of 2 */
#define RDA_DL_MASK				(RDA_DL_IPDATA_BUF_CNT - 1)
#define RDA_IPDATA_BUF_SIZE		1600

u32 smd_get_ul_free_buf_count(void);
u32 smd_get_dl_free_buf_count(void);

u32 smd_alloc_ul_ipdata_buf(void);
void smd_fill_ul_ipdata_hdr(ps_header_t *header, u8 simid, u8 cid, u16 len);
void smd_free_dl_ipdata_buf(u32 offset);

#endif
