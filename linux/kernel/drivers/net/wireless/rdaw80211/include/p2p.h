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

#ifndef _P2P_H_
#define _P2P_H_


#include <802.11.h>


/* WiFi P2P OUI values */
#define P2P_OUI			        WFA_OUI			/* WiFi P2P OUI */
#define P2P_VER			        WFA_OUI_TYPE_P2P	/* P2P version: 9=WiFi P2P v1.0 */

#define P2P_IE_ID		        0xdd			/* P2P IE element ID */

/* WiFi P2P IE */
PRE_PACKED struct wifi_p2p_ie {
	u8	id;		/* IE ID: 0xDD */
	u8	len;		/* IE length */
	u8	OUI[3];		/* WiFi P2P specific OUI: P2P_OUI */
	u8	oui_type;	/* Identifies P2P version: P2P_VER */
	u8	subelts[1];	/* variable length subelements */
} POST_PACKED;

#define P2P_IE_FIXED_LEN	    6

#define P2P_ATTR_ID_OFF		    0
#define P2P_ATTR_LEN_OFF	    1
#define P2P_ATTR_DATA_OFF	    3

#define P2P_ATTR_ID_LEN		    1	/* ID filed length */
#define P2P_ATTR_LEN_LEN	    2	/* length field length */
#define P2P_ATTR_HDR_LEN	    3 /* ID + 2-byte length field spec 1.02 */

/* P2P IE Subelement IDs from WiFi P2P Technical Spec 1.00 */
#define P2P_SEID_STATUS			0	/* Status */
#define P2P_SEID_MINOR_RC		1	/* Minor Reason Code */
#define P2P_SEID_P2P_INFO		2	/* P2P Capability (capabilities info) */
#define P2P_SEID_DEV_ID			3	/* P2P Device ID */
#define P2P_SEID_INTENT			4	/* Group Owner Intent */
#define P2P_SEID_CFG_TIMEOUT	5	/* Configuration Timeout */
#define P2P_SEID_CHANNEL		6	/* Channel */
#define P2P_SEID_GRP_BSSID		7	/* P2P Group BSSID */
#define P2P_SEID_XT_TIMING		8	/* Extended Listen Timing */
#define P2P_SEID_INTINTADDR		9	/* Intended P2P Interface Address */
#define P2P_SEID_P2P_MGBTY		10	/* P2P Manageability */
#define P2P_SEID_CHAN_LIST		11	/* Channel List */
#define P2P_SEID_ABSENCE		12	/* Notice of Absence */
#define P2P_SEID_DEV_INFO		13	/* Device Info */
#define P2P_SEID_GROUP_INFO		14	/* Group Info */
#define P2P_SEID_GROUP_ID		15	/* Group ID */
#define P2P_SEID_P2P_IF			16	/* P2P Interface */
#define P2P_SEID_OP_CHANNEL		17	/* Operating Channel */
#define P2P_SEID_INVITE_FLAGS	18	/* Invitation Flags */
#define P2P_SEID_VNDR			221	/* Vendor-specific subelement */

#define P2P_SE_VS_ID_SERVICES	0x1b /* BRCM proprietary subel: L2 Services */


/* WiFi P2P IE subelement: P2P Capability (capabilities info) */
PRE_PACKED struct wifi_p2p_info_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_P2P_INFO */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	dev;		/* Device Capability Bitmap */
	u8	group;		/* Group Capability Bitmap */
} POST_PACKED;

/* P2P Capability subelement's Device Capability Bitmap bit values */
#define P2P_CAPSE_DEV_SERVICE_DIS	0x1 /* Service Discovery */
#define P2P_CAPSE_DEV_CLIENT_DIS	0x2 /* Client Discoverability */
#define P2P_CAPSE_DEV_CONCURRENT	0x4 /* Concurrent Operation */
#define P2P_CAPSE_DEV_INFRA_MAN		0x8 /* P2P Infrastructure Managed */
#define P2P_CAPSE_DEV_LIMIT			0x10 /* P2P Device Limit */
#define P2P_CAPSE_INVITE_PROC		0x20 /* P2P Invitation Procedure */

/* P2P Capability subelement's Group Capability Bitmap bit values */
#define P2P_CAPSE_GRP_OWNER			0x1 /* P2P Group Owner */
#define P2P_CAPSE_PERSIST_GRP		0x2 /* Persistent P2P Group */
#define P2P_CAPSE_GRP_LIMIT			0x4 /* P2P Group Limit */
#define P2P_CAPSE_GRP_INTRA_BSS		0x8 /* Intra-BSS Distribution */
#define P2P_CAPSE_GRP_X_CONNECT		0x10 /* Cross Connection */
#define P2P_CAPSE_GRP_PERSISTENT	0x20 /* Persistent Reconnect */
#define P2P_CAPSE_GRP_FORMATION		0x40 /* Group Formation */

/* WiFi P2P IE subelement: Group Owner Intent */
PRE_PACKED struct wifi_p2p_intent_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_INTENT */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	intent;		/* Intent Value 0...15 (0=legacy 15=master only) */
} POST_PACKED;

/* WiFi P2P IE subelement: Configuration Timeout */
PRE_PACKED struct wifi_p2p_cfg_tmo_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_CFG_TIMEOUT */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	go_tmo;		/* GO config timeout in units of 10 ms */
	u8	client_tmo;	/* Client config timeout in units of 10 ms */
} POST_PACKED;

/* WiFi P2P IE subelement: Listen Channel */
PRE_PACKED struct wifi_p2p_listen_channel_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_CHANNEL */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	country[3];	/* Country String */
	u8	op_class;	/* Operating Class */
	u8	channel;	/* Channel */
} POST_PACKED;

/* WiFi P2P IE subelement: P2P Group BSSID */
PRE_PACKED struct wifi_p2p_grp_bssid_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_GRP_BSSID */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	mac[6];		/* P2P group bssid */
} POST_PACKED;

/* WiFi P2P IE subelement: P2P Group ID */
PRE_PACKED struct wifi_p2p_grp_id_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_GROUP_ID */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	mac[6];		/* P2P device address */
	u8	ssid[1];	/* ssid. device id. variable length */
} POST_PACKED;

/* WiFi P2P IE subelement: P2P Interface */
PRE_PACKED struct wifi_p2p_intf_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_P2P_IF */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	mac[6];		/* P2P device address */
	u8	ifaddrs;	/* P2P Interface Address count */
	u8	ifaddr[1][6];	/* P2P Interface Address list */
} POST_PACKED;

/* WiFi P2P IE subelement: Status */
PRE_PACKED struct wifi_p2p_status_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_STATUS */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	status;		/* Status Code: P2P_STATSE_* */
} POST_PACKED;

/* Status subelement Status Code definitions */
#define P2P_STATSE_SUCCESS			0
				/* Success */
#define P2P_STATSE_FAIL_INFO_CURR_UNAVAIL	1	/* Failed, information currently unavailable */
#define P2P_STATSE_PASSED_UP				P2P_STATSE_FAIL_INFO_CURR_UNAVAIL
				/* Old name for above in P2P spec 1.08 and older */
#define P2P_STATSE_FAIL_INCOMPAT_PARAMS		2	/* Failed, incompatible parameters */
#define P2P_STATSE_FAIL_LIMIT_REACHED		3	/* Failed, limit reached */
#define P2P_STATSE_FAIL_INVALID_PARAMS		4	/* Failed, invalid parameters */
#define P2P_STATSE_FAIL_UNABLE_TO_ACCOM		5	/* Failed, unable to accomodate request */
#define P2P_STATSE_FAIL_PROTO_ERROR			6	/* Failed, previous protocol error or disruptive behaviour */
#define P2P_STATSE_FAIL_NO_COMMON_CHAN		7	/* Failed, no common channels */
#define P2P_STATSE_FAIL_UNKNOWN_GROUP		8	/* Failed, unknown P2P Group */
#define P2P_STATSE_FAIL_INTENT				9	/* Failed, both peers indicated Intent 15 in GO Negotiation */
#define P2P_STATSE_FAIL_INCOMPAT_PROVIS		10	/* Failed, incompatible provisioning method */
#define P2P_STATSE_FAIL_USER_REJECT			11	/* Failed, rejected by user */

/* WiFi P2P IE attribute: Extended Listen Timing */
PRE_PACKED struct wifi_p2p_ext_se_s {
	u8	eltId;		/* ID: P2P_SEID_EXT_TIMING */
	u8	len[2];		/* length not including eltId, len fields */
	u8	avail[2];	/* availibility period */
	u8	interval[2];	/* availibility interval */
} POST_PACKED;

#define P2P_EXT_MIN	10	/* minimum 10ms */

/* WiFi P2P IE subelement: Intended P2P Interface Address */
PRE_PACKED struct wifi_p2p_intintad_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_INTINTADDR */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	mac[6];		/* intended P2P interface MAC address */
} POST_PACKED;

/* WiFi P2P IE subelement: Channel */
PRE_PACKED struct wifi_p2p_channel_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_STATUS */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	band;		/* Regulatory Class (band) */
	u8	channel;	/* Channel */
} POST_PACKED;

/* Channel Entry structure within the Channel List SE */
PRE_PACKED struct wifi_p2p_chanlist_entry_s {
	u8	band;						/* Regulatory Class (band) */
	u8	num_channels;				/* # of channels in the channel list */
	u8	channels[WL_NUMCHANNELS];	/* Channel List */
} POST_PACKED;

#define WIFI_P2P_CHANLIST_SE_MAX_ENTRIES 2

/* WiFi P2P IE subelement: Channel List */
PRE_PACKED struct wifi_p2p_chanlist_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_CHAN_LIST */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	country[3];	/* Country String */
	u8	num_entries;	/* # of channel entries */
	struct wifi_p2p_chanlist_entry_s	entries[WIFI_P2P_CHANLIST_SE_MAX_ENTRIES];
						/* Channel Entry List */
} POST_PACKED;

/* WiFi Primary Device Type structure */
PRE_PACKED struct wifi_p2p_pri_devtype_s {
	u16	cat_id;		/* Category ID */
	u8	OUI[3];		/* WFA OUI: 0x0050F2 */
	u8	oui_type;	/* WPS_OUI_TYPE */
	u16	sub_cat_id;	/* Sub Category ID */
} POST_PACKED;

/* WiFi P2P IE's Device Info subelement */
PRE_PACKED struct wifi_p2p_devinfo_se_s {
	u8	eltId;			/* SE ID: P2P_SEID_DEVINFO */
	u8	len[2];			/* SE length not including eltId, len fields */
	u8	mac[6];			/* P2P Device MAC address */
	u16	wps_cfg_meths;		/* Config Methods: reg_prototlv.h WPS_CONFMET_* */
	u8	pri_devtype[8];		/* Primary Device Type */
} POST_PACKED;

#define P2P_DEV_TYPE_LEN	8

/* WiFi P2P IE's Group Info subelement Client Info Descriptor */
PRE_PACKED struct wifi_p2p_cid_fixed_s {
	u8	len;
	u8	devaddr[ETHER_ADDR_LEN];	/* P2P Device Address */
	u8	ifaddr[ETHER_ADDR_LEN];		/* P2P Interface Address */
	u8	devcap;				/* Device Capability */
	u8	cfg_meths[2];			/* Config Methods: reg_prototlv.h WPS_CONFMET_* */
	u8	pridt[P2P_DEV_TYPE_LEN];	/* Primary Device Type */
	u8	secdts;				/* Number of Secondary Device Types */
} POST_PACKED;

/* WiFi P2P IE's Device ID subelement */
PRE_PACKED struct wifi_p2p_devid_se_s {
	u8	eltId;
	u8	len[2];
	struct ether_addr	addr;			/* P2P Device MAC address */
} POST_PACKED;

/* WiFi P2P IE subelement: P2P Manageability */
PRE_PACKED struct wifi_p2p_mgbt_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_P2P_MGBTY */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	mg_bitmap;	/* manageability bitmap */
} POST_PACKED;

/* mg_bitmap field bit values */
#define P2P_MGBTSE_P2PDEVMGMT_FLAG   0x1 /* AP supports Managed P2P Device */

/* WiFi P2P IE subelement: Group Info */
PRE_PACKED struct wifi_p2p_grpinfo_se_s {
	u8	eltId;			/* SE ID: P2P_SEID_GROUP_INFO */
	u8	len[2];			/* SE length not including eltId, len fields */
} POST_PACKED;

/* WiFi IE subelement: Operating Channel */
PRE_PACKED struct wifi_p2p_op_channel_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_OP_CHANNEL */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	country[3];	/* Country String */
	u8	op_class;	/* Operating Class */
	u8	channel;	/* Channel */
} POST_PACKED;

/* WiFi IE subelement: INVITATION FLAGS */
PRE_PACKED struct wifi_p2p_invite_flags_se_s {
	u8	eltId;		/* SE ID: P2P_SEID_INVITE_FLAGS */
	u8	len[2];		/* SE length not including eltId, len fields */
	u8	flags;		/* Flags */
} POST_PACKED;

/* WiFi P2P Action Frame */
PRE_PACKED struct wifi_p2p_action_frame {
	u8	category;	/* P2P_AF_CATEGORY */
	u8	OUI[3];		/* OUI - P2P_OUI */
	u8	type;		/* OUI Type - P2P_VER */
	u8	subtype;	/* OUI Subtype - P2P_AF_* */
	u8	dialog_token;	/* nonzero, identifies req/resp tranaction */
	u8	elts[1];	/* Variable length information elements.  Max size =
				 * ACTION_FRAME_SIZE - sizeof(this structure) - 1
				 */
} POST_PACKED;

#define P2P_AF_CATEGORY		        0x7f
#define P2P_AF_FIXED_LEN	        7

/* WiFi P2P Action Frame OUI Subtypes */
#define P2P_AF_NOTICE_OF_ABSENCE	0	/* Notice of Absence */
#define P2P_AF_PRESENCE_REQ		    1	/* P2P Presence Request */
#define P2P_AF_PRESENCE_RSP		    2	/* P2P Presence Response */
#define P2P_AF_GO_DISC_REQ		    3	/* GO Discoverability Request */


/* WiFi P2P Public Action Frame */
PRE_PACKED struct wifi_p2p_pub_act_frame {
	u8	category;	/* P2P_PUB_AF_CATEGORY */
	u8	action;		/* P2P_PUB_AF_ACTION */
	u8	oui[3];		/* P2P_OUI */
	u8	oui_type;	/* OUI type - P2P_VER */
	u8	subtype;	/* OUI subtype - P2P_TYPE_* */
	u8	dialog_token;	/* nonzero, identifies req/rsp transaction */
	u8	elts[1];	/* Variable length information elements.  Max size =
        				 * ACTION_FRAME_SIZE - sizeof(this structure) - 1
        				 */
} POST_PACKED;

#define P2P_PUB_AF_FIXED_LEN	8
#define P2P_PUB_AF_CATEGORY	    0x04
#define P2P_PUB_AF_ACTION	    0x09

/* WiFi P2P Public Action Frame OUI Subtypes */
#define P2P_PAF_GON_REQ		    0	/* Group Owner Negotiation Req */
#define P2P_PAF_GON_RSP		    1	/* Group Owner Negotiation Rsp */
#define P2P_PAF_GON_CONF	    2	/* Group Owner Negotiation Confirm */
#define P2P_PAF_INVITE_REQ	    3	/* P2P Invitation Request */
#define P2P_PAF_INVITE_RSP	    4	/* P2P Invitation Response */
#define P2P_PAF_DEVDIS_REQ	    5	/* Device Discoverability Request */
#define P2P_PAF_DEVDIS_RSP	    6	/* Device Discoverability Response */
#define P2P_PAF_PROVDIS_REQ	    7	/* Provision Discovery Request */
#define P2P_PAF_PROVDIS_RSP	    8	/* Provision Discovery Response */
#define P2P_PAF_SUBTYPE_INVALID	255	/* Invalid Subtype */

/* TODO: Stop using these obsolete aliases for P2P_PAF_GON_* */
#define P2P_TYPE_MNREQ		P2P_PAF_GON_REQ
#define P2P_TYPE_MNRSP		P2P_PAF_GON_RSP
#define P2P_TYPE_MNCONF		P2P_PAF_GON_CONF

/* WiFi P2P IE subelement: Notice of Absence */
PRE_PACKED struct wifi_p2p_noa_desc {
	u8	cnt_type;	/* Count/Type */
	u32	duration;	/* Duration */
	u32	interval;	/* Interval */
	u32	start;		/* Start Time */
} POST_PACKED;

PRE_PACKED struct wifi_p2p_noa_se {
	u8	eltId;		/* Subelement ID */
	u8	len[2];		/* Length */
	u8	index;		/* Index */
	u8	ops_ctw_parms;	/* CTWindow and OppPS Parameters */
	struct wifi_p2p_noa_desc	desc[1];	/* Notice of Absence Descriptor(s) */
} POST_PACKED;

#define P2P_NOA_SE_FIXED_LEN	5

/* cnt_type field values */
#define P2P_NOA_DESC_CNT_RESERVED	0	/* reserved and should not be used */
#define P2P_NOA_DESC_CNT_REPEAT		255	/* continuous schedule */
#define P2P_NOA_DESC_TYPE_PREFERRED	1	/* preferred values */
#define P2P_NOA_DESC_TYPE_ACCEPTABLE	2	/* acceptable limits */

/* ctw_ops_parms field values */
#define P2P_NOA_CTW_MASK	0x7f
#define P2P_NOA_OPS_MASK	0x80
#define P2P_NOA_OPS_SHIFT	7

#define P2P_CTW_MIN	10	/* minimum 10TU */

/*
 * P2P Service Discovery related
 */
#define	P2PSD_ACTION_CATEGORY		0x04	/* Public action frame */
#define	P2PSD_ACTION_ID_GAS_IREQ	0x0a    /* Action value for GAS Initial Request AF */
#define	P2PSD_ACTION_ID_GAS_IRESP	0x0b	/* Action value for GAS Initial Response AF */
#define	P2PSD_ACTION_ID_GAS_CREQ	0x0c	/* Action value for GAS Comback Request AF */
#define	P2PSD_ACTION_ID_GAS_CRESP	0x0d	/* Action value for GAS Comback Response AF */
#define P2PSD_AD_EID				0x6c	/* Advertisement Protocol IE ID */
#define P2PSD_ADP_TUPLE_QLMT_PAMEBI	0x00	/* Query Response Length Limit 7 bits plus PAME-BI 1 bit */
#define P2PSD_ADP_PROTO_ID			0x00	/* Advertisement Protocol ID. Always 0 for P2P SD */
#define P2PSD_GAS_OUI				P2P_OUI	/* WFA OUI */
#define P2PSD_GAS_OUI_SUBTYPE		P2P_VER	/* OUI Subtype for GAS IE */
#define P2PSD_GAS_NQP_INFOID		0xDDDD	/* NQP Query Info ID: 56797 */
#define P2PSD_GAS_COMEBACKDEALY		0x00	/* Not used in the Native GAS protocol */

/* Service Protocol Type */
enum p2psd_svc_protype {
	SVC_RPOTYPE_ALL = 0,
	SVC_RPOTYPE_BONJOUR = 1,
	SVC_RPOTYPE_UPNP = 2,
	SVC_RPOTYPE_WSD = 3,
	SVC_RPOTYPE_VENDOR = 255
} ;

/* Service Discovery response status code */
enum p2psd_resp_status{
	P2PSD_RESP_STATUS_SUCCESS = 0,
	P2PSD_RESP_STATUS_PROTYPE_NA = 1,
	P2PSD_RESP_STATUS_DATA_NA = 2,
	P2PSD_RESP_STATUS_BAD_REQUEST = 3
} ;

/* Advertisement Protocol IE tuple field */
PRE_PACKED struct wifi_p2psd_adp_tpl {
	u8	llm_pamebi;	/* Query Response Length Limit bit 0-6, set to 0 plus
            				* Pre-Associated Message Exchange BSSID Independent bit 7, set to 0
            				*/
	u8	adp_id;		/* Advertisement Protocol ID: 0 for NQP Native Query Protocol */
} POST_PACKED;

/* Advertisement Protocol IE */
PRE_PACKED struct wifi_p2psd_adp_ie {
	u8	                    id;		/* IE ID: 0x6c - 108 */
	u8	                    len;	/* IE length */
	struct wifi_p2psd_adp_tpl   adp_tpl;/* Advertisement Protocol Tuple field. Only one
                    				* tuple is defined for P2P Service Discovery
                    				*/
} POST_PACKED;

/* NQP Vendor-specific Content */
PRE_PACKED struct wifi_p2psd_nqp_query_vsc {
	u8	oui_subtype;	/* OUI Subtype: 0x09 */
	u16	svc_updi;		/* Service Update Indicator */
	u8	svc_tlvs[1];	/* wifi_p2psd_qreq_tlv_t type for service request,
            				 * wifi_p2psd_qresp_tlv_t type for service response
            				 */
} POST_PACKED;

/* Service Request TLV */
PRE_PACKED struct wifi_p2psd_qreq_tlv {
	u16	len;			/* Length: 5 plus size of Query Data */
	u8	svc_prot;		/* Service Protocol Type */
	u8	svc_tscid;		/* Service Transaction ID */
	u8	query_data[1];	/* Query Data, passed in from above Layer 2 */
} POST_PACKED;

/* Query Request Frame, defined in generic format, instead of NQP specific */
PRE_PACKED struct wifi_p2psd_qreq_frame {
	u16	info_id;	    /* Info ID: 0xDDDD */
	u16	len;		    /* Length of service request TLV, 5 plus the size of request data */
	u8	oui[3];		    /* WFA OUI: 0x0050F2 */
	u8	qreq_vsc[1];    /* Vendor-specific Content: wifi_p2psd_nqp_query_vsc_t type for NQP */
} POST_PACKED;

/* GAS Initial Request AF body, "elts" in wifi_p2p_pub_act_frame */
PRE_PACKED struct wifi_p2psd_gas_ireq_frame {
	struct wifi_p2psd_adp_ie	adp_ie;		/* Advertisement Protocol IE */
	u16					    qreq_len;	/* Query Request Length */
	u8	                    qreq_frm[1];	/* Query Request Frame wifi_p2psd_qreq_frame_t */
} POST_PACKED;

/* Service Response TLV */
PRE_PACKED struct wifi_p2psd_qresp_tlv {
	u16	len;			/* Length: 5 plus size of Query Data */
	u8	svc_prot;		/* Service Protocol Type */
	u8	svc_tscid;		/* Service Transaction ID */
	u8	status;			/* Value defined in Table 57 of P2P spec. */
	u8	query_data[1];	/* Response Data, passed in from above Layer 2 */
} POST_PACKED;

/* Query Response Frame, defined in generic format, instead of NQP specific */
PRE_PACKED struct wifi_p2psd_qresp_frame {
	u16	info_id;	    /* Info ID: 0xDDDD */
	u16	len;		    /* Lenth of service response TLV, 6 plus the size of resp data */
	u8	oui[3];		    /* WFA OUI: 0x0050F2 */
	u8	qresp_vsc[1];   /* Vendor-specific Content: wifi_p2psd_qresp_tlv_t type for NQP */

} POST_PACKED;

/* GAS Initial Response AF body, "elts" in wifi_p2p_pub_act_frame */
PRE_PACKED struct wifi_p2psd_gas_iresp_frame {
	u16	                    status;			/* Value defined in Table 7-23 of IEEE P802.11u */
	u16	                    cb_delay;		/* GAS Comeback Delay */
	struct wifi_p2psd_adp_ie	adp_ie;		/* Advertisement Protocol IE */
	u16		                qresp_len;	/* Query Response Length */
	u8	                    qresp_frm[1];	/* Query Response Frame wifi_p2psd_qresp_frame_t */
} POST_PACKED;

/* GAS Comeback Response AF body, "elts" in wifi_p2p_pub_act_frame */
PRE_PACKED struct wifi_p2psd_gas_cresp_frame {
	u16	                    status;			/* Value defined in Table 7-23 of IEEE P802.11u */
	u8	                    fragment_id;	/* Fragmentation ID */
	u16	                    cb_delay;		/* GAS Comeback Delay */
	struct wifi_p2psd_adp_ie	adp_ie;		/* Advertisement Protocol IE */
	u16	                    qresp_len;		/* Query Response Length */
	u8	                    qresp_frm[1];	/* Query Response Frame wifi_p2psd_qresp_frame_t */
} POST_PACKED;

/* Wi-Fi GAS Public Action Frame */
PRE_PACKED struct wifi_p2psd_gas_pub_act_frame {
	u8	category;		/* 0x04 Public Action Frame */
	u8	action;			/* 0x6c Advertisement Protocol */
	u8	dialog_token;	/* nonzero, identifies req/rsp transaction */
	u8	query_data[1];	/* Query Data. wifi_p2psd_gas_ireq_frame_t or wifi_p2psd_gas_iresp_frame_t format */
} POST_PACKED;

#endif /* _P2P_H_ */
