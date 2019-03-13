/*
 * Warning:
 *
 * This file is a mess of debugging and ROM bug workarounds.
 */

/* Enable a workaround for a bug in the bootrom romapi. */
#define RDA8810_VENDOR_EFUSE_WORKAROUND

/* Enable debugging certificates in pdl1: */
//#define RDA_CRYPTO_DEBUG_FAKE_ROMCERT



#ifdef RDA_CRYPTO_DEBUG_FAKE_ROMCERT
/*
 * Debugging version og get_chip_security_context().
 * This version is enabled with RDA_CRYPTO_DEBUG_FAKE_ROMCERT
 *
TEST DATA
   Use this secret key (file: rdatest.sec, pw: pa$$w0rd):

"Comment: RDA PDL-Test-01 secret key
UkRBU0VkQksAAAAq0Ai4xjiIrmda3G8qI6q3B4gPbVQMPISmUkRBIFBETC1UZXN0LTAxACU+PXqYnPYI
gGYdDrpN5PljQAAht3pYTN4bNgtA6oRp/sEVIrSXZiEkZPRGsnqlGPGFRHn3MMsDZBz/FhZGdE1m2FnD
YYgQMg=="
 */
int get_chip_security_context(
	struct chip_security_context *context,
	struct pubkey *pubkey)
{
	const struct chip_security_context c = {
		3, 0x42,
		RDA_SE_CFG_SECURITY_ENABLE_BIT |
		RDA_SE_CFG_INDIRECT_SIGN_BIT |
		RDA_SE_CFG_UNLOCK_ALLOWED
	};
	const struct pubkey pk = {
		/* Public key: rdatest.pub */
		{'R','D','A','S'}, {'E','d'},{}, "RDA PDL-Test-01 ",
		{ 0x25,0x3e,0x3d,0x7a,0x98,0x9c,0xf6,0x08  },
		{ 0x7b,0x4a,0x5a,0xa8,0xa1,0xda,0x92,0xea,0x9e,0x90,0xa8,0x7e,
		  0xfa,0x76,0x37,0x52,0xe0,0xe0,0x40,0x63,0x09,0x02,0xd3,0x86,
		  0x8b,0x9d,0xe0,0xae,0xf3,0x57,0xd7,0x44  }
	};

	// Set the hardcoded debug public key and context
	*context = c;
	*pubkey = pk;

	return ROM_API_SECURITY_ENABLED;
}
#endif



#ifdef RDA8810_VENDOR_EFUSE_WORKAROUND

#include <common.h>
#include <asm/arch/rda_crypto.h>
#include <asm/arch/ispi.h>

#define RDA_EFUSE_INDEX_SECURITY        (11)

/*
  Fuse layout:
	VVVVVVVV.T.H.I.S.KKKK
  VVVVVVVV: Vendor ID - 6bit + 2bit armour
         T: Trace disable
         H: Host serial disable
         I: Indirectly signed image
         S: Security enable
      KKKK: RDA Public Key Index - 3bit + 1 bit armour
*/
/* define bit for rda se config */
#define RDA_SE_CFG_KEY_INDEX(n)         (((n)&0xF)<<0)
#define RDA_SE_CFG_GET_KEY_INDEX(r)     (((r)>>0)&0xF)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT  (1<<4)
#define RDA_SE_CFG_INDIRECT_SIGN_BIT    (1<<5)
#define RDA_SE_CFG_HOST_DISABLE_BIT     (1<<6)
#define RDA_SE_CFG_TRACE_DISABLE_BIT    (1<<7)
#define RDA_SE_CFG_VENDOR_ID(n)         (((n)&0xFF)<<8)
#define RDA_SE_CFG_GET_VENDOR_ID(r)     (((r)>>8)&0xFF)

// RDA Public Key index map
//   (armour 6 indexes in 4 bits)
static const uint8_t keyindex_map[16] = {
	15, 15, 15,  0, 15,  1,  2, 15,
	15,  3,  4, 15,  5, 15, 15, 15
};

// Vendor ID
//   (armour 50 vendor ID'd in 8 bits)
static int valid_vendor_id( unsigned vendor )
{
	unsigned c;
	unsigned i = vendor >> 2;
	c = ((i & 0xaa)>>1) + (i & 0x55);
	c = ((c & 0xcc)>>2) + (c & 0x33);
	c = ((c & 0xf0)>>4) + (c & 0x0f);
	return ((vendor & 3) + c) == 5;
}
#endif




#ifndef RDA_CRYPTO_DEBUG_FAKE_ROMCERT
int get_chip_security_context(
	struct chip_security_context *context,
	struct pubkey *pubkey)
{
	int ret = romapi->get_chip_security_context(context, pubkey);

#ifdef RDA8810_VENDOR_EFUSE_WORKAROUND
	if (ret == ROM_API_INVALID_VENDOR_ID && romapi->version == 100) {
		uint16_t sec = rda_read_efuse(RDA_EFUSE_INDEX_SECURITY);

		// check validity of key index
		int key = keyindex_map[RDA_SE_CFG_GET_KEY_INDEX(sec)];

		// check validity of vendor id
		int vendor = RDA_SE_CFG_GET_VENDOR_ID(sec);
		if (valid_vendor_id(vendor)) {
			context->rda_key_index = key;
			context->vendor_id = vendor;
			ret = 0;
		}
	}
#endif
	return ret;
}
#endif
