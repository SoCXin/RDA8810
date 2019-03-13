
/*
 * Copyright (c) 2014 Rdamicro Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef WLAND_BT_3_0_SUPPORT
#include <wland_bta.h>

#ifdef SEND_HCI_CMD_VIA_IOCTL
#define BTA_HCI_CMD_MAX_LEN     HCI_CMD_PREAMBLE_SIZE + HCI_CMD_DATA_SIZE

/* Send HCI cmd via wl iovar HCI_cmd to the dongle. */
int dhd_bta_docmd(wland_private * pub, void *cmd_buf, uint cmd_len)
{
	amp_hci_cmd_t *cmd = (amp_hci_cmd_t *) cmd_buf;
	u8 buf[BTA_HCI_CMD_MAX_LEN + 16];
	uint len = sizeof(buf);
	struct wl_ioctl ioc;

	if (cmd_len < HCI_CMD_PREAMBLE_SIZE)
		return BCME_BADLEN;

	if ((uint) cmd->plen + HCI_CMD_PREAMBLE_SIZE > cmd_len)
		return BCME_BADLEN;

	len = bcm_mkiovar("HCI_cmd", (char *) cmd,
		(uint) cmd->plen + HCI_CMD_PREAMBLE_SIZE, (char *) buf, len);

	memset(&ioc, 0, sizeof(ioc));

	ioc.cmd = WLC_SET_VAR;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = true;

	return dhd_wl_ioctl(pub, &ioc, ioc.buf, ioc.len);
}
#else /* !SEND_HCI_CMD_VIA_IOCTL */

static void dhd_bta_flush_hcidata(wland_private * pub, u16 llh)
{
	int prec;
	uint count = 0;
	struct pktq *q = wland_bus_txq(pub->bus);

	if (q == NULL)
		return;

	DHD_BTA(("dhd: flushing HCI ACL data for logical link %u...\n", llh));

	dhd_os_sdlock_txq(pub);

	/*
	 * Walk through the txq and toss all HCI ACL data packets
	 */
	for (prec = q->num_prec - 1; prec >= 0; prec--) {
		void *head_pkt = NULL;

		while (pktq_ppeek(q, prec) != head_pkt) {
			void *pkt = pktq_pdeq(q, prec);
			int ifidx;

			PKTPULL(pub->osh, pkt, dhd_bus_hdrlen(pub->bus));
			wland_proto_hdrpull(pub, &ifidx, pkt);

			if (PKTLEN(pub->osh, pkt) >= RFC1042_HDR_LEN) {
				struct ether_header *eh =
					(struct ether_header *)
					PKTDATA(pub->osh, pkt);

				if (ntoh16(eh->ether_type) < ETHER_TYPE_MIN) {
					struct dot11_llc_snap_header *lsh =
						(struct dot11_llc_snap_header *)
						&eh[1];

					if (memcmp(lsh, BT_SIG_SNAP_MPROT,
							DOT11_LLC_SNAP_HDR_LEN -
							2) == 0
						&& ntoh16(lsh->type) ==
						BTA_PROT_L2CAP) {
						amp_hci_ACL_data_t *ACL_data =
							(amp_hci_ACL_data_t *) &
							lsh[1];
						u16 handle =
							ltoh16(ACL_data->
							handle);

						if (HCI_ACL_DATA_HANDLE(handle)
							== llh) {
							osl_pktfree(pub->osh,
								pkt, true);
							count++;
							continue;
						}
					}
				}
			}

			wland_proto_hdrpush(pub, ifidx, pkt);
			PKTPUSH(pub->osh, pkt, dhd_bus_hdrlen(pub->bus));

			if (head_pkt == NULL)
				head_pkt = pkt;
			pktq_penq(q, prec, pkt);
		}
	}

	dhd_os_sdunlock_txq(pub);

	WLAND_DBG(HCI, "dhd: flushed %u packet(s) for logical link %u...\n",
		count, llh);
}

/* Handle HCI cmd locally.
 * Return 0: continue to send the cmd across SDIO
 *        < 0: stop, fail
 *        > 0: stop, succuess
 */
static int _dhd_bta_docmd(wland_private * pub, amp_hci_cmd_t * cmd)
{
	int status = 0;

	switch (ltoh16_ua((u8 *) & cmd->opcode)) {
	case HCI_Enhanced_Flush:
		{
			eflush_cmd_parms_t *cmdparms =
				(eflush_cmd_parms_t *) cmd->parms;
			dhd_bta_flush_hcidata(pub, ltoh16_ua(cmdparms->llh));
			break;
		}
	default:
		break;
	}

	return status;
}

/* Send HCI cmd encapsulated in BT-SIG frame via data channel to the dongle. */
int dhd_bta_docmd(wland_private * pub, void *cmd_buf, uint cmd_len)
{
	amp_hci_cmd_t *cmd = (amp_hci_cmd_t *) cmd_buf;
	struct ether_header *eh;
	struct dot11_llc_snap_header *lsh;
	struct osl_info *osh = pub->osh;
	uint len;
	void *p;
	int status;

	if (cmd_len < HCI_CMD_PREAMBLE_SIZE) {
		WLAND_ERR("dhd_bta_docmd: short command, cmd_len %u\n",
			cmd_len);
		return BCME_BADLEN;
	}

	if ((len = (uint) cmd->plen + HCI_CMD_PREAMBLE_SIZE) > cmd_len) {
		WLAND_ERR
			("dhd_bta_docmd: malformed command, len %u cmd_len %u\n",
			len, cmd_len);
		/*
		 * return BCME_BADLEN;
		 */
	}

	p = osl_pktget(osh, pub->hdrlen + RFC1042_HDR_LEN + len);
	if (p == NULL) {
		WLAND_ERR(("dhd_bta_docmd: out of memory\n"));
		return BCME_NOMEM;
	}

	/*
	 * intercept and handle the HCI cmd locally
	 */
	if ((status = _dhd_bta_docmd(pub, cmd)) > 0)
		return 0;
	else if (status < 0)
		return status;

	/*
	 * copy in HCI cmd
	 */
	PKTPULL(osh, p, pub->hdrlen + RFC1042_HDR_LEN);

	memcpy(PKTDATA(osh, p), cmd, len);

	/*
	 * copy in partial Ethernet header with BT-SIG LLC/SNAP header
	 */
	PKTPUSH(osh, p, RFC1042_HDR_LEN);
	eh = (struct ether_header *) PKTDATA(osh, p);

	memset(eh->ether_dhost, '\0', ETH_ALEN);

	ETHER_SET_LOCALADDR(eh->ether_dhost);

	memcpy(eh->ether_shost, &pub->mac, ETH_ALEN);

	eh->ether_type = hton16(len + DOT11_LLC_SNAP_HDR_LEN);

	lsh = (struct dot11_llc_snap_header *) &eh[1];

	memcpy(lsh, BT_SIG_SNAP_MPROT, DOT11_LLC_SNAP_HDR_LEN - 2);
	lsh->type = 0;

	return dhd_sendpkt(pub, 0, p);
}
#endif /* !SEND_HCI_CMD_VIA_IOCTL */

/* Send HCI ACL data to dongle via data channel */
int dhd_bta_tx_hcidata(wland_private * pub, void *data_buf, uint data_len)
{
	amp_hci_ACL_data_t *data = (amp_hci_ACL_data_t *) data_buf;
	struct ether_header *eh;
	struct dot11_llc_snap_header *lsh;
	struct osl_info *osh = pub->osh;
	uint len;
	void *p;

	if (data_len < HCI_ACL_DATA_PREAMBLE_SIZE) {
		WLAND_ERR("dhd_bta_tx_hcidata: short data_buf, data_len %u\n",
			data_len);
		return BCME_BADLEN;
	}

	if ((len = (uint) ltoh16(data->dlen) + HCI_ACL_DATA_PREAMBLE_SIZE) >
		data_len) {
		WLAND_ERR
			("dhd_bta_tx_hcidata: malformed hci data, len %u data_len %u\n",
			len, data_len);
		/*
		 * return BCME_BADLEN;
		 */
	}

	p = osl_pktget(osh, pub->hdrlen + RFC1042_HDR_LEN + len);
	if (p == NULL) {
		WLAND_ERR("dhd_bta_tx_hcidata: out of memory\n");
		return BCME_NOMEM;
	}

	/*
	 * copy in HCI ACL data header and HCI ACL data
	 */
	PKTPULL(osh, p, pub->hdrlen + RFC1042_HDR_LEN);

	memcpy(PKTDATA(osh, p), data, len);

	/*
	 * copy in partial Ethernet header with BT-SIG LLC/SNAP header
	 */
	PKTPUSH(osh, p, RFC1042_HDR_LEN);
	eh = (struct ether_header *) PKTDATA(osh, p);

	memset(eh->ether_dhost, '\0', ETH_ALEN);
	memcpy(eh->ether_shost, &pub->mac, ETH_ALEN);

	eh->ether_type = hton16(len + DOT11_LLC_SNAP_HDR_LEN);
	lsh = (struct dot11_llc_snap_header *) &eh[1];

	memcpy(lsh, BT_SIG_SNAP_MPROT, DOT11_LLC_SNAP_HDR_LEN - 2);

	lsh->type = BTA_PROT_L2CAP;

	return wland_sendpkt(pub, p);
}

/* txcomplete callback */
void dhd_bta_tx_hcidata_complete(wland_private * dhdp, void *txp, bool success)
{
	u8 *pktdata = (u8 *) PKTDATA(dhdp->osh, txp);
	amp_hci_ACL_data_t *ACL_data =
		(amp_hci_ACL_data_t *) (pktdata + RFC1042_HDR_LEN);
	u16 handle = ltoh16(ACL_data->handle);
	u16 llh = HCI_ACL_DATA_HANDLE(handle);

	wl_event_msg_t event;
	u8 data[HCI_EVT_PREAMBLE_SIZE +
		sizeof(num_completed_data_blocks_evt_parms_t)];
	amp_hci_event_t *evt;
	num_completed_data_blocks_evt_parms_t *parms;

	u16 len =
		HCI_EVT_PREAMBLE_SIZE +
		sizeof(num_completed_data_blocks_evt_parms_t);

	/*
	 * update the event struct
	 */
	memset(&event, 0, sizeof(event));
	event.version = hton16(BCM_EVENT_MSG_VERSION);
	event.event_type = hton32(WLC_E_BTA_HCI_EVENT);
	event.status = 0;
	event.reason = 0;
	event.auth_type = 0;
	event.datalen = hton32(len);
	event.flags = 0;

	/*
	 * generate Number of Completed Blocks event
	 */
	evt = (amp_hci_event_t *) data;
	evt->ecode = HCI_Number_of_Completed_Data_Blocks;
	evt->plen = sizeof(num_completed_data_blocks_evt_parms_t);

	parms = (num_completed_data_blocks_evt_parms_t *) evt->parms;
	htol16_ua_store(dhdp->maxdatablks, (u8 *) & parms->num_blocks);
	parms->num_handles = 1;
	htol16_ua_store(llh, (u8 *) & parms->completed[0].handle);
	parms->completed[0].pkts = 1;
	parms->completed[0].blocks = 1;

	dhd_sendup_event_common(dhdp, &event, data);
}

/* event callback */
void dhd_bta_doevt(wland_private * dhdp, void *data_buf, uint data_len)
{
	amp_hci_event_t *evt = (amp_hci_event_t *) data_buf;

	switch (evt->ecode) {
	case HCI_Command_Complete:
		{
			cmd_complete_parms_t *parms =
				(cmd_complete_parms_t *) evt->parms;
			switch (ltoh16_ua((u8 *) & parms->opcode)) {
			case HCI_Read_Data_Block_Size:
				{
					read_data_block_size_evt_parms_t *parms2
						=
						(read_data_block_size_evt_parms_t
						*) parms->parms;
					dhdp->maxdatablks =
						ltoh16_ua((u8 *) &
						parms2->data_block_num);
					break;
				}
			}
			break;
		}

	case HCI_Flush_Occurred:
		{
			flush_occurred_evt_parms_t *evt_parms =
				(flush_occurred_evt_parms_t *) evt->parms;
			dhd_bta_flush_hcidata(dhdp,
				ltoh16_ua((u8 *) & evt_parms->handle));
			break;
		}
	default:
		break;
	}
}
#endif /* WLAND_BT_3_0_SUPPORT */
