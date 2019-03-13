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

#ifndef _WPA_H_
#define _WPA_H_

#include <ethernet.h>


/* Reason Codes */

/* 13 through 23 taken from IEEE Std 802.11i-2004 */
#define DOT11_RC_INVALID_WPA_IE		13	/* Invalid info. element */
#define DOT11_RC_MIC_FAILURE		14	/* Michael failure */
#define DOT11_RC_4WH_TIMEOUT		15	/* 4-way handshake timeout */
#define DOT11_RC_GTK_UPDATE_TIMEOUT	16	/* Group key update timeout */
#define DOT11_RC_WPA_IE_MISMATCH	17	/* WPA IE in 4-way handshake differs from (re-)assoc. request/probe response */
#define DOT11_RC_INVALID_MC_CIPHER	18	/* Invalid multicast cipher */
#define DOT11_RC_INVALID_UC_CIPHER	19	/* Invalid unicast cipher */
#define DOT11_RC_INVALID_AKMP		20	/* Invalid authenticated key management protocol */
#define DOT11_RC_BAD_WPA_VERSION	21	/* Unsupported WPA version */
#define DOT11_RC_INVALID_WPA_CAP	22	/* Invalid WPA IE capabilities */
#define DOT11_RC_8021X_AUTH_FAIL	23	/* 802.1X authentication failure */

#define WPA2_PMKID_LEN	            16

/* WPA IE fixed portion */
PRE_PACKED struct wpa_ie_fixed
{
	u8 tag;	    /* TAG */
	u8 length;	/* TAG length */
	u8 oui[3];	/* IE OUI */
	u8 oui_type;	/* OUI type */
	PRE_PACKED struct 
	{
		u8 low;
		u8 high;
	} POST_PACKED version;	/* IE version */
} POST_PACKED;

#define WPA_IE_OUITYPE_LEN	        4
#define WPA_IE_FIXED_LEN	        8
#define WPA_IE_TAG_FIXED_LEN	    6

PRE_PACKED struct wpa_rsn_ie_fixed
{
	u8 tag;	    /* TAG */
	u8 length;	/* TAG length */
	PRE_PACKED struct 
	{
		u8 low;
		u8 high;
	} POST_PACKED version;	/* IE version */
} POST_PACKED;

#define WPA_RSN_IE_FIXED_LEN	    4
#define WPA_RSN_IE_TAG_FIXED_LEN	2

typedef u8 wpa_pmkid_t[WPA2_PMKID_LEN];

/* WPA suite/multicast suite */
typedef PRE_PACKED struct
{
	u8 oui[3];
	u8 type;
} POST_PACKED wpa_suite_t, wpa_suite_mcast_t;

#define WPA_SUITE_LEN	4

/* WPA unicast suite list/key management suite list */
typedef PRE_PACKED struct
{
	PRE_PACKED struct {
		u8 low;
		u8 high;
	} POST_PACKED count;
	wpa_suite_t list[1];
} POST_PACKED wpa_suite_ucast_t, wpa_suite_auth_key_mgmt_t;

#define WPA_IE_SUITE_COUNT_LEN	2

typedef PRE_PACKED struct
{
	PRE_PACKED struct {
		u8 low;
		u8 high;
	} POST_PACKED count;
	wpa_pmkid_t list[1];
} POST_PACKED wpa_pmkid_list_t;

/* WPA cipher suites */
#define WPA_CIPHER_NONE		            0	/* None */
#define WPA_CIPHER_WEP_40	            1	/* WEP (40-bit) */
#define WPA_CIPHER_TKIP		            2	/* TKIP: default for WPA */
#define WPA_CIPHER_AES_OCB	            3	/* AES (OCB) */
#define WPA_CIPHER_AES_CCM	            4	/* AES (CCM) */
#define WPA_CIPHER_WEP_104	            5	/* WEP (104-bit) */
#define WPA_CIPHER_BIP		            6	/* WEP (104-bit) */
#define WPA_CIPHER_TPK		            7	/* Group addressed traffic not allowed */
#ifdef BCMWAPI_WPI
#define WAPI_CIPHER_NONE	            WPA_CIPHER_NONE
#define WAPI_CIPHER_SMS4	            11
#define WAPI_CSE_WPI_SMS4	            1
#endif /* BCMWAPI_WPI */


#define IS_WPA_CIPHER(cipher)	       ((cipher) == WPA_CIPHER_NONE || \
                				        (cipher) == WPA_CIPHER_WEP_40 || \
                				        (cipher) == WPA_CIPHER_WEP_104 || \
                				        (cipher) == WPA_CIPHER_TKIP || \
                				        (cipher) == WPA_CIPHER_AES_OCB || \
                				        (cipher) == WPA_CIPHER_AES_CCM || \
                				        (cipher) == WPA_CIPHER_TPK)

#ifdef BCMWAPI_WAI
#define IS_WAPI_CIPHER(cipher)	        ((cipher) == WAPI_CIPHER_NONE || (cipher) == WAPI_CSE_WPI_SMS4)

/* convert WAPI_CSE_WPI_XXX to WAPI_CIPHER_XXX */
#define WAPI_CSE_WPI_2_CIPHER(cse)      ((cse) == WAPI_CSE_WPI_SMS4 ? WAPI_CIPHER_SMS4 : WAPI_CIPHER_NONE)

#define WAPI_CIPHER_2_CSE_WPI(cipher)   ((cipher) == WAPI_CIPHER_SMS4 ? WAPI_CSE_WPI_SMS4 : WAPI_CIPHER_NONE)
#endif /* BCMWAPI_WAI */


/* WPA TKIP countermeasures parameters */
#define WPA_TKIP_CM_DETECT	            60	/* multiple MIC failure window (seconds) */
#define WPA_TKIP_CM_BLOCK	            60	/* countermeasures active window (seconds) */

/* RSN IE defines */
#define RSN_CAP_LEN		                2	/* Length of RSN capabilities field (2 octets) */

/* RSN Capabilities defined in 802.11i */
#define RSN_CAP_PREAUTH			        0x0001
#define RSN_CAP_NOPAIRWISE		        0x0002
#define RSN_CAP_PTK_REPLAY_CNTR_MASK	0x000C
#define RSN_CAP_PTK_REPLAY_CNTR_SHIFT	2
#define RSN_CAP_GTK_REPLAY_CNTR_MASK	0x0030
#define RSN_CAP_GTK_REPLAY_CNTR_SHIFT	4
#define RSN_CAP_1_REPLAY_CNTR		    0
#define RSN_CAP_2_REPLAY_CNTRS		    1
#define RSN_CAP_4_REPLAY_CNTRS		    2
#define RSN_CAP_16_REPLAY_CNTRS		    3
#ifdef MFP
#define RSN_CAP_MFPR			        0x0040
#define RSN_CAP_MFPC			        0x0080
#endif

/* WPA capabilities defined in 802.11i */
#define WPA_CAP_4_REPLAY_CNTRS		    RSN_CAP_4_REPLAY_CNTRS
#define WPA_CAP_16_REPLAY_CNTRS		    RSN_CAP_16_REPLAY_CNTRS
#define WPA_CAP_REPLAY_CNTR_SHIFT	    RSN_CAP_PTK_REPLAY_CNTR_SHIFT
#define WPA_CAP_REPLAY_CNTR_MASK	    RSN_CAP_PTK_REPLAY_CNTR_MASK

/* WPA capabilities defined in 802.11zD9.0 */
#define WPA_CAP_PEER_KEY_ENABLE		    (0x1 << 1)	/* bit 9 */

/* WPA Specific defines */
#define WPA_CAP_LEN	                    RSN_CAP_LEN	/* Length of RSN capabilities in RSN IE (2 octets) */
#define WPA_PMKID_CNT_LEN	            2 	/* Length of RSN PMKID count (2 octests) */

#define	WPA_CAP_WPA2_PREAUTH		    RSN_CAP_PREAUTH

#ifdef BCMWAPI_WAI
#define WAPI_CAP_PREAUTH		        RSN_CAP_PREAUTH

/* Other WAI definition */
#define WAPI_WAI_REQUEST		        0x00F1
#define WAPI_UNICAST_REKEY		        0x00F2
#define WAPI_STA_AGING			        0x00F3
#define WAPI_MUTIL_REKEY		        0x00F4
#define WAPI_STA_STATS			        0x00F5

#define WAPI_USK_REKEY_COUNT		    0x4000000 /* 0xA00000 */
#define WAPI_MSK_REKEY_COUNT		    0x4000000 /* 0xA00000 */
#endif /* BCMWAPI_WAI */

/* The following macros describe the bitfield map used by the firmware to determine its 11i mode */
#define NO_ENCRYPT			             0
#define ENCRYPT_ENABLED	                (1 << 0)
#define WEP					            (1 << 1)
#define WEP_EXTENDED		            (1 << 2)
#define WPA					            (1 << 3)
#define WPA2				            (1 << 4)
#define AES					            (1 << 5)
#define TKIP					        (1 << 6)

#define WPA2_PMKID_COUNT_LEN	        2

enum SECURITY_T{
	NO_SECURITY   = 0,
	WEP_40        = 0x3,
	WEP_104       = 0x7,
	WPA_AES       = 0x29,
	WPA_TKIP      = 0x49,
	WPA_AES_TKIP  = 0x69,		/* Aes or Tkip */
	WPA2_AES      = 0x31,
	WPA2_TKIP     = 0x51,
	WPA2_AES_TKIP = 0x71,	/* Aes or Tkip */
}; 

enum AUTHTYPE_T{
	OPEN_SYSTEM   = 1,
	SHARED_KEY    = 2,
	ANY           = 3,
    IEEE8021      = 5
};
#endif /* _WPA_H_ */
