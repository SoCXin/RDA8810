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
#ifndef _WLAND_BT_AMP_H_
#define _WLAND_BT_AMP_H_

#define BT_SIG_SNAP_MPROT						"\xAA\xAA\x03\x00\x19\x58"

/* BT-AMP 802.11 PAL Protocols */
#define BTA_PROT_L2CAP								1
#define	BTA_PROT_ACTIVITY_REPORT					2
#define BTA_PROT_SECURITY							3
#define BTA_PROT_LINK_SUPERVISION_REQUEST			4
#define BTA_PROT_LINK_SUPERVISION_REPLY				5

/* BT-AMP 802.11 PAL AMP_ASSOC Type IDs */
#define BTA_TYPE_ID_MAC_ADDRESS						1
#define BTA_TYPE_ID_PREFERRED_CHANNELS				2
#define BTA_TYPE_ID_CONNECTED_CHANNELS				3
#define BTA_TYPE_ID_CAPABILITIES					4
#define BTA_TYPE_ID_VERSION							5


/* AMP HCI CMD packet format */
typedef PRE_PACKED struct amp_hci_cmd {
	u16 opcode;
	u8 plen;
	u8 parms[1];
} POST_PACKED amp_hci_cmd_t;

#define HCI_CMD_PREAMBLE_SIZE				OFFSETOF(amp_hci_cmd_t, parms)
#define HCI_CMD_DATA_SIZE					255

/* AMP HCI CMD opcode layout */
#define HCI_CMD_OPCODE(ogf, ocf)			((((ogf) & 0x3F) << 10) | ((ocf) & 0x03FF))
#define HCI_CMD_OGF(opcode)					((u8)(((opcode) >> 10) & 0x3F))
#define HCI_CMD_OCF(opcode)					((opcode) & 0x03FF)

/* AMP HCI command opcodes */
#define HCI_Read_Failed_Contact_Counter		HCI_CMD_OPCODE(0x05, 0x0001)
#define HCI_Reset_Failed_Contact_Counter	HCI_CMD_OPCODE(0x05, 0x0002)
#define HCI_Read_Link_Quality				HCI_CMD_OPCODE(0x05, 0x0003)
#define HCI_Read_Local_AMP_Info				HCI_CMD_OPCODE(0x05, 0x0009)
#define HCI_Read_Local_AMP_ASSOC			HCI_CMD_OPCODE(0x05, 0x000A)
#define HCI_Write_Remote_AMP_ASSOC			HCI_CMD_OPCODE(0x05, 0x000B)
#define HCI_Create_Physical_Link			HCI_CMD_OPCODE(0x01, 0x0035)
#define HCI_Accept_Physical_Link_Request	HCI_CMD_OPCODE(0x01, 0x0036)
#define HCI_Disconnect_Physical_Link		HCI_CMD_OPCODE(0x01, 0x0037)
#define HCI_Create_Logical_Link				HCI_CMD_OPCODE(0x01, 0x0038)
#define HCI_Accept_Logical_Link				HCI_CMD_OPCODE(0x01, 0x0039)
#define HCI_Disconnect_Logical_Link			HCI_CMD_OPCODE(0x01, 0x003A)
#define HCI_Logical_Link_Cancel				HCI_CMD_OPCODE(0x01, 0x003B)
#define HCI_Flow_Spec_Modify				HCI_CMD_OPCODE(0x01, 0x003C)
#define HCI_Write_Flow_Control_Mode			HCI_CMD_OPCODE(0x01, 0x0067)
#define HCI_Read_Best_Effort_Flush_Timeout	HCI_CMD_OPCODE(0x01, 0x0069)
#define HCI_Write_Best_Effort_Flush_Timeout	HCI_CMD_OPCODE(0x01, 0x006A)
#define HCI_Short_Range_Mode				HCI_CMD_OPCODE(0x01, 0x006B)
#define HCI_Reset							HCI_CMD_OPCODE(0x03, 0x0003)
#define HCI_Read_Connection_Accept_Timeout	HCI_CMD_OPCODE(0x03, 0x0015)
#define HCI_Write_Connection_Accept_Timeout	HCI_CMD_OPCODE(0x03, 0x0016)
#define HCI_Read_Link_Supervision_Timeout	HCI_CMD_OPCODE(0x03, 0x0036)
#define HCI_Write_Link_Supervision_Timeout	HCI_CMD_OPCODE(0x03, 0x0037)
#define HCI_Enhanced_Flush					HCI_CMD_OPCODE(0x03, 0x005F)
#define HCI_Read_Logical_Link_Accept_Timeout	HCI_CMD_OPCODE(0x03, 0x0061)
#define HCI_Write_Logical_Link_Accept_Timeout	HCI_CMD_OPCODE(0x03, 0x0062)
#define HCI_Set_Event_Mask_Page_2			HCI_CMD_OPCODE(0x03, 0x0063)
#define HCI_Read_Location_Data_Command		HCI_CMD_OPCODE(0x03, 0x0064)
#define HCI_Write_Location_Data_Command		HCI_CMD_OPCODE(0x03, 0x0065)
#define HCI_Read_Local_Version_Info			HCI_CMD_OPCODE(0x04, 0x0001)
#define HCI_Read_Local_Supported_Commands	HCI_CMD_OPCODE(0x04, 0x0002)
#define HCI_Read_Buffer_Size				HCI_CMD_OPCODE(0x04, 0x0005)
#define HCI_Read_Data_Block_Size			HCI_CMD_OPCODE(0x04, 0x000A)

/* AMP HCI command parameters */
typedef PRE_PACKED struct read_local_cmd_parms {
	u8 plh;
	u8 offset[2];			/* length so far */
	u8 max_remote[2];
} POST_PACKED read_local_cmd_parms_t;

typedef PRE_PACKED struct write_remote_cmd_parms {
	u8 plh;
	u8 offset[2];
	u8 len[2];
	u8 frag[1];
} POST_PACKED write_remote_cmd_parms_t;

typedef PRE_PACKED struct phy_link_cmd_parms {
	u8 plh;
	u8 key_length;
	u8 key_type;
	u8 key[1];
} POST_PACKED phy_link_cmd_parms_t;

typedef PRE_PACKED struct dis_phy_link_cmd_parms {
	u8 plh;
	u8 reason;
} POST_PACKED dis_phy_link_cmd_parms_t;

typedef PRE_PACKED struct log_link_cmd_parms {
	u8 plh;
	u8 txflow[16];
	u8 rxflow[16];
} POST_PACKED log_link_cmd_parms_t;

typedef PRE_PACKED struct ext_flow_spec {
	u8 id;
	u8 service_type;
	u8 max_sdu[2];
	u8 sdu_ia_time[4];
	u8 access_latency[4];
	u8 flush_timeout[4];
} POST_PACKED ext_flow_spec_t;

typedef PRE_PACKED struct log_link_cancel_cmd_parms {
	u8 plh;
	u8 tx_fs_ID;
} POST_PACKED log_link_cancel_cmd_parms_t;

typedef PRE_PACKED struct flow_spec_mod_cmd_parms {
	u8 llh[2];
	u8 txflow[16];
	u8 rxflow[16];
} POST_PACKED flow_spec_mod_cmd_parms_t;

typedef PRE_PACKED struct plh_pad {
	u8 plh;
	u8 pad;
} POST_PACKED plh_pad_t;

typedef PRE_PACKED union hci_handle {
	u16 bredr;
	plh_pad_t amp;
} POST_PACKED hci_handle_t;

typedef PRE_PACKED struct ls_to_cmd_parms {
	hci_handle_t handle;
	u8 timeout[2];
} POST_PACKED ls_to_cmd_parms_t;

typedef PRE_PACKED struct befto_cmd_parms {
	u8 llh[2];
	u8 befto[4];
} POST_PACKED befto_cmd_parms_t;

typedef PRE_PACKED struct srm_cmd_parms {
	u8 plh;
	u8 srm;
} POST_PACKED srm_cmd_parms_t;

typedef PRE_PACKED struct ld_cmd_parms {
	u8 ld_aware;
	u8 ld[2];
	u8 ld_opts;
	u8 l_opts;
} POST_PACKED ld_cmd_parms_t;

typedef PRE_PACKED struct eflush_cmd_parms {
	u8 llh[2];
	u8 packet_type;
} POST_PACKED eflush_cmd_parms_t;

/* Generic AMP extended flow spec service types */
#define EFS_SVCTYPE_NO_TRAFFIC		0
#define EFS_SVCTYPE_BEST_EFFORT		1
#define EFS_SVCTYPE_GUARANTEED		2

/* AMP HCI event packet format */
typedef PRE_PACKED struct amp_hci_event {
	u8 ecode;
	u8 plen;
	u8 parms[1];
} POST_PACKED amp_hci_event_t;

#define HCI_EVT_PREAMBLE_SIZE			OFFSETOF(amp_hci_event_t, parms)

/* AMP HCI event codes */
#define HCI_Command_Complete			0x0E
#define HCI_Command_Status			0x0F
#define HCI_Flush_Occurred			0x11
#define HCI_Enhanced_Flush_Complete		0x39
#define HCI_Physical_Link_Complete		0x40
#define HCI_Channel_Select			0x41
#define HCI_Disconnect_Physical_Link_Complete	0x42
#define HCI_Logical_Link_Complete		0x45
#define HCI_Disconnect_Logical_Link_Complete	0x46
#define HCI_Flow_Spec_Modify_Complete		0x47
#define HCI_Number_of_Completed_Data_Blocks	0x48
#define HCI_Short_Range_Mode_Change_Complete	0x4C
#define HCI_Status_Change_Event			0x4D
#define HCI_Vendor_Specific			0xFF

/* AMP HCI event mask bit positions */
#define HCI_Physical_Link_Complete_Event_Mask			0x0001
#define HCI_Channel_Select_Event_Mask				0x0002
#define HCI_Disconnect_Physical_Link_Complete_Event_Mask	0x0004
#define HCI_Logical_Link_Complete_Event_Mask			0x0020
#define HCI_Disconnect_Logical_Link_Complete_Event_Mask		0x0040
#define HCI_Flow_Spec_Modify_Complete_Event_Mask		0x0080
#define HCI_Number_of_Completed_Data_Blocks_Event_Mask		0x0100
#define HCI_Short_Range_Mode_Change_Complete_Event_Mask		0x1000
#define HCI_Status_Change_Event_Mask				0x2000
#define HCI_All_Event_Mask					0x31e7
/* AMP HCI event parameters */
typedef PRE_PACKED struct cmd_status_parms {
	u8 status;
	u8 cmdpkts;
	u16 opcode;
} POST_PACKED cmd_status_parms_t;

typedef PRE_PACKED struct cmd_complete_parms {
	u8 cmdpkts;
	u16 opcode;
	u8 parms[1];
} POST_PACKED cmd_complete_parms_t;

typedef PRE_PACKED struct flush_occurred_evt_parms {
	u16 handle;
} POST_PACKED flush_occurred_evt_parms_t;

typedef PRE_PACKED struct write_remote_evt_parms {
	u8 status;
	u8 plh;
} POST_PACKED write_remote_evt_parms_t;

typedef PRE_PACKED struct read_local_evt_parms {
	u8 status;
	u8 plh;
	u16 len;
	u8 frag[1];
} POST_PACKED read_local_evt_parms_t;

typedef PRE_PACKED struct read_local_info_evt_parms {
	u8      status;
	u8      AMP_status;
	u32  	bandwidth;
	u32  	gbandwidth;
	u32  	latency;
	u32  	PDU_size;
	u8      ctrl_type;
	u16     PAL_cap;
	u16     AMP_ASSOC_len;
	u32  	max_flush_timeout;
	u32  	be_flush_timeout;
} POST_PACKED read_local_info_evt_parms_t;

typedef PRE_PACKED struct log_link_evt_parms {
	u8  status;
	u16 llh;
	u8  plh;
	u8  tx_fs_ID;
} POST_PACKED log_link_evt_parms_t;

typedef PRE_PACKED struct disc_log_link_evt_parms {
	u8 status;
	u16 llh;
	u8 reason;
} POST_PACKED disc_log_link_evt_parms_t;

typedef PRE_PACKED struct log_link_cancel_evt_parms {
	u8 status;
	u8 plh;
	u8 tx_fs_ID;
} POST_PACKED log_link_cancel_evt_parms_t;

typedef PRE_PACKED struct flow_spec_mod_evt_parms {
	u8 status;
	u16 llh;
} POST_PACKED flow_spec_mod_evt_parms_t;

typedef PRE_PACKED struct phy_link_evt_parms {
	u8 status;
	u8 plh;
} POST_PACKED phy_link_evt_parms_t;

typedef PRE_PACKED struct dis_phy_link_evt_parms {
	u8 status;
	u8 plh;
	u8 reason;
} POST_PACKED dis_phy_link_evt_parms_t;

typedef PRE_PACKED struct read_ls_to_evt_parms {
	u8 status;
	hci_handle_t handle;
	u16 timeout;
} POST_PACKED read_ls_to_evt_parms_t;

typedef PRE_PACKED struct read_lla_ca_to_evt_parms {
	u8  status;
	u16 timeout;
} POST_PACKED read_lla_ca_to_evt_parms_t;

typedef PRE_PACKED struct read_data_block_size_evt_parms {
	u8  status;
	u16 ACL_pkt_len;
	u16 data_block_len;
	u16 data_block_num;
} POST_PACKED read_data_block_size_evt_parms_t;

typedef PRE_PACKED struct data_blocks {
	u16 handle;
	u16 pkts;
	u16 blocks;
} POST_PACKED data_blocks_t;

typedef PRE_PACKED struct num_completed_data_blocks_evt_parms {
	u16 num_blocks;
	u8 num_handles;
	data_blocks_t completed[1];
} POST_PACKED num_completed_data_blocks_evt_parms_t;

typedef PRE_PACKED struct befto_evt_parms {
	u8 status;
	u32 befto;
} POST_PACKED befto_evt_parms_t;

typedef PRE_PACKED struct srm_evt_parms {
	u8 status;
	u8 plh;
	u8 srm;
} POST_PACKED srm_evt_parms_t;

typedef PRE_PACKED struct contact_counter_evt_parms {
	u8 status;
	u8 llh[2];
	u16 counter;
} POST_PACKED contact_counter_evt_parms_t;

typedef PRE_PACKED struct contact_counter_reset_evt_parms {
	u8 status;
	u8 llh[2];
} POST_PACKED contact_counter_reset_evt_parms_t;

typedef PRE_PACKED struct read_linkq_evt_parms {
	u8 status;
	hci_handle_t handle;
	u8 link_quality;
} POST_PACKED read_linkq_evt_parms_t;

typedef PRE_PACKED struct ld_evt_parms {
	u8 status;
	u8 ld_aware;
	u8 ld[2];
	u8 ld_opts;
	u8 l_opts;
} POST_PACKED ld_evt_parms_t;

typedef PRE_PACKED struct eflush_complete_evt_parms {
	u16 handle;
} POST_PACKED eflush_complete_evt_parms_t;

typedef PRE_PACKED struct vendor_specific_evt_parms {
	u8 len;
	u8 parms[1];
} POST_PACKED vendor_specific_evt_parms_t;

typedef PRE_PACKED struct local_version_info_evt_parms {
	u8 status;
	u8 hci_version;
	u16 hci_revision;
	u8 pal_version;
	u16 mfg_name;
	u16 pal_subversion;
} POST_PACKED local_version_info_evt_parms_t;

#define MAX_SUPPORTED_CMD_BYTE	64
typedef PRE_PACKED struct local_supported_cmd_evt_parms {
	u8 status;
	u8 cmd[MAX_SUPPORTED_CMD_BYTE];
} POST_PACKED local_supported_cmd_evt_parms_t;

typedef PRE_PACKED struct status_change_evt_parms {
	u8 status;
	u8 amp_status;
} POST_PACKED status_change_evt_parms_t;

/* AMP HCI error codes */
#define HCI_SUCCESS				            0x00
#define HCI_ERR_ILLEGAL_COMMAND			    0x01
#define HCI_ERR_NO_CONNECTION			    0x02
#define HCI_ERR_MEMORY_FULL			        0x07
#define HCI_ERR_CONNECTION_TIMEOUT		    0x08
#define HCI_ERR_MAX_NUM_OF_CONNECTIONS		0x09
#define HCI_ERR_CONNECTION_EXISTS		    0x0B
#define HCI_ERR_CONNECTION_DISALLOWED		0x0C
#define HCI_ERR_CONNECTION_ACCEPT_TIMEOUT	0x10
#define HCI_ERR_UNSUPPORTED_VALUE		    0x11
#define HCI_ERR_ILLEGAL_PARAMETER_FMT		0x12
#define HCI_ERR_CONN_TERM_BY_LOCAL_HOST		0x16
#define HCI_ERR_UNSPECIFIED			        0x1F
#define HCI_ERR_UNIT_KEY_USED			    0x26
#define HCI_ERR_QOS_REJECTED			    0x2D
#define HCI_ERR_PARAM_OUT_OF_RANGE		    0x30
#define HCI_ERR_NO_SUITABLE_CHANNEL		    0x39
#define HCI_ERR_CHANNEL_MOVE			    0xFF

/* AMP HCI ACL Data packet format */
typedef PRE_PACKED struct amp_hci_ACL_data {
	u16	handle;			/* 12-bit connection handle + 2-bit PB and 2-bit BC flags */
	u16	dlen;			/* data total length */
	u8  data[1];
} POST_PACKED amp_hci_ACL_data_t;

#define HCI_ACL_DATA_PREAMBLE_SIZE	OFFSETOF(amp_hci_ACL_data_t, data)

#define HCI_ACL_DATA_BC_FLAGS		(0x0 << 14)
#define HCI_ACL_DATA_PB_FLAGS		(0x3 << 12)

#define HCI_ACL_DATA_HANDLE(handle)	((handle) & 0x0fff)
#define HCI_ACL_DATA_FLAGS(handle)	((handle) >> 12)

/* AMP Activity Report packet formats */
typedef PRE_PACKED struct amp_hci_activity_report {
	u8	ScheduleKnown;
	u8	NumReports;
	u8	data[1];
} POST_PACKED amp_hci_activity_report_t;

typedef PRE_PACKED struct amp_hci_activity_report_triple {
	u32	StartTime;
	u32	Duration;
	u32	Periodicity;
} POST_PACKED amp_hci_activity_report_triple_t;

#define HCI_AR_SCHEDULE_KNOWN		0x01

#endif /* _WLAND_BT_AMP_H_ */
