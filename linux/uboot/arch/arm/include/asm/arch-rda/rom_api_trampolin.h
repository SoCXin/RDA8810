
#ifndef __RDA_ROM_API_H__
#define __RDA_ROM_API_H__

struct chip_id {
	// ASIC information
	uint32_t  chip;     // CHIP(31:15) NA(15:15) BOND(14:12) METAL(11:0)
	uint32_t  res1;
	// Production information
	uint16_t  date;     // YYYY:MMMM:DDDDD
	uint16_t  wafer;
	uint16_t  xy;
	uint16_t  res2;
};

struct chip_security_context {
	// Security context
	uint8_t   rda_key_index; // 0-5 : 0 default key
	uint8_t   vendor_id;     // 0-50
	uint16_t  flags;
#define RDA_SE_CFG_UNLOCK_ALLOWED       (1<<0)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT  (1<<4)
#define RDA_SE_CFG_INDIRECT_SIGN_BIT    (1<<5)
#define RDA_SE_CFG_HOST_DISABLE_BIT     (1<<6)
#define RDA_SE_CFG_TRACE_DISABLE_BIT    (1<<7)
};

struct chip_unique_id {
	uint8_t	  id[32];
};

/* security */
#define RDASIGN     "RDAS"
#define PKALG       "Ed"
#define KDFALG      "BK"

#define SIGBYTES    64
#define PUBLICBYTES 32
#define SECRETBYTES 64
#define FPLEN        8

struct pubkey {
	uint8_t rdasign[4];//RDAS
	uint8_t pkalg[2];  //Ed
	uint8_t dummy[2];
	uint8_t name[16];
	uint8_t fingerprint[FPLEN];
	uint8_t pubkey[PUBLICBYTES];
};

// RDASEdPo
struct sig {
	uint8_t rdasign[4];//RDAS
	uint8_t pkalg[2];  //Ed
	uint8_t hashalg[2];//Po/B2/SH
	uint8_t name[16];
	uint8_t fingerprint[FPLEN];
	uint8_t sig[SIGBYTES];
};


/* ROM Cryto API */

struct ROM_crypto_api {
	char     magic[8];  // "RDA API"
	unsigned version;   // 100

	// signature
	int (*signature_open) (
		const uint8_t *message, unsigned length,
		const struct sig    *sig,
		const struct pubkey *pubkey);
/* Return values */
#define ROM_API_SIGNATURE_OK      0
#define ROM_API_SIGNATURE_FAIL   -1
// positive values is for invalid arguments

	// hash
	unsigned sz_hash_context;
	int (*hash_init) ( unsigned *S, uint8_t outlen );
	int (*hash_update) ( unsigned *S, const uint8_t *in, unsigned inlen );
	int (*hash_final) ( unsigned *S, uint8_t *out, uint8_t outlen );

	// info API
	void (*get_chip_id) (struct chip_id *id);
	void (*get_chip_unique) (struct chip_unique_id *out);
	int (*get_chip_security_context) (struct chip_security_context *context,
                                          struct pubkey *pubkey);
/* Return values */
#define ROM_API_SECURITY_ENABLED      0
#define ROM_API_SECURITY_DISABLED     1
#define ROM_API_INVALID_KEYINDEX      2
#define ROM_API_INVALID_VENDOR_ID     3
#define ROM_API_SECURITY_UNAVAILABLE  4

	// RND
	void (*get_chip_true_random) (uint8_t *out, uint8_t outlen);

	// NEW in v101:
	int (*signature_open_w_hash)(uint8_t message_hash[64],
                              const struct sig    *signature,
                              const struct pubkey *pubkey);
/* Return values */
// same as signature_open() plus ROM_API_UNAVAILABLE if method is unavailable.
#define ROM_API_UNAVAILABLE 100

	// Future extension
	unsigned dummy[19];
};

#ifndef BOOTROM

// The ROMAPI is allocated either at 0x3f00 or 0xff00.
// 8810  - 0xff00
// 8810H - 0x3f00
#ifndef ROMAPI_BASE
#  define ROMAPI_BASE 0xff00
#endif

static const struct ROM_crypto_api *romapi = (void*)ROMAPI_BASE;


// info API

static void inline get_chip_id(struct chip_id *id)
{
	romapi->get_chip_id(id);
}


static inline void get_chip_unique(struct chip_unique_id *id )
{
	romapi->get_chip_unique(id);
}

/* Replaced with a enhanced version in arch/arm/rda/rda_romapi.c */
int get_chip_security_context(struct chip_security_context *context, struct pubkey *pubkey);
/*
static inline int get_chip_security_context(struct chip_security_context *context, struct pubkey *pubkey)
{
	return romapi->get_chip_security_context(context, pubkey);
}*/


//
// RND
//
static inline void get_chip_true_random(uint8_t *out, uint8_t outlen)
{
	romapi->get_chip_true_random(out, outlen);
}

//
// SIGNATURE CHECK
//
static inline int signature_check(
		const uint8_t *message, unsigned length,
		const struct sig    *sig,
		const struct pubkey *pubkey)
{
	return romapi->signature_open(message, length, sig, pubkey);
}

// NEW in v101:
//
// SIGNATURE CHECK variant that can be used to check larger than RAM messages
//
static inline int signature_check_w_hash(
		uint8_t message_hash[64],
		const struct sig    *sig,
		const struct pubkey *pubkey)
{
	if (romapi->signature_open_w_hash)
		return romapi->signature_open_w_hash(message_hash, sig, pubkey);
	else
		return ROM_API_UNAVAILABLE;
}

#endif
#endif
