#include <common.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <nand.h>
#include <malloc.h>
#include <image.h>
#include <usb/usbserial.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/rda_crypto.h>
#include <pdl.h>

//#define RDA_CRYPTO_DEBUG

#ifdef CONFIG_SIGNATURE_CHECK_IMAGE

struct pubkey_cert_t {
	struct {
		uint16_t se_cfg; // vendor_id and key_index
		uint8_t dummy[94];           // 96
		struct pubkey pubkey;        // +64
	} c;
	struct sig signature;                // +96
};

struct rda_cert_t {
	// R&D Certificate:
	// Create a signature with name[16] =
	//    "  R&D-CERT"(10)
	//    "F:0000"(6)
	// zeros[16] = 0...
	// Then Hash:
	//     sig.name, zeros, UNIQUE-ID
	struct sig rnd_cert;           // 96

	// free space without sign checking:
	uint8_t dummy[160];            // 160
};


struct rda_se_image {
	uint8_t               spl_code[48128+160];
	struct sig            image_signature;      // 96
	struct rda_cert_t     cert;                 // 256
	struct pubkey_cert_t  vendor_pubkey_cert;   // 256
};

#ifdef RDA_CRYPTO_DEBUG
#    define puts_deb(a) puts(a)
#else
#    define puts_deb(a)
#    define debug_dump_key_and_signature(h,pk,sig,d,l)
#endif

#ifdef RDA_CRYPTO_DEBUG
static void debug_dump_key_and_signature(
	const char *header, const struct pubkey *pk, const struct sig *sig,
	const void *data, unsigned data_len)
{
	if (sig) {
		printf("%s signature:\n", header);
		rda_dump_buf((char *)sig, 96);
	}
	if (pk) {
		printf("%s public key:\n", header);
		rda_dump_buf((char *)pk, 64);
	}
	if (data) {
		printf("%s data:\n", header);
		rda_dump_buf((char *)data, data_len);
	}
}
#endif


static int check_rnd_certificate(
	const struct rda_cert_t  *certs,
	const struct pubkey      *pubkey,
	struct spl_security_info *info)
{
	int ret;
	const struct sig *rnd_signature = &certs->rnd_cert;
	struct {
		uint8_t message[32];
		struct chip_unique_id id;
	} hashme;

	puts_deb("Checking R&D Certificate\n");

	// create message
	memset(&hashme, 0, sizeof(hashme));
	memcpy(hashme.message, rnd_signature->name, 16);
	hashme.id = info->chip_unique_id;

	// Dump the R&D hashme
	debug_dump_key_and_signature("R&D hashme", pubkey, rnd_signature,
		(char *)&hashme, sizeof(hashme));

	ret = signature_check(
		(const uint8_t*)&hashme, sizeof(hashme),
		rnd_signature,
		pubkey);

	return ret;
}


static void get_device_security_context(
	const struct rda_se_image *image,
	struct spl_security_info  *info)
{
	// Get ROM public key and context
	info->secure_mode = get_chip_security_context(
				&info->chip_security_context,
				&info->pubkey);

	int flags = info->chip_security_context.flags;

	// Fix the return code if security hasn't been enabled
	if ((flags & RDA_SE_CFG_SECURITY_ENABLE_BIT) == 0) {
		puts_deb("(Security disabled by efuse flags)\n");
		info->secure_mode = ROM_API_SECURITY_DISABLED;
		return;
	}

	// Check if we a using the customer public-key certificate
	if (flags & RDA_SE_CFG_INDIRECT_SIGN_BIT) {
		const struct pubkey_cert_t *vendor_cert = &image->vendor_pubkey_cert;
		puts_deb("(Using vendor pkcert as per efuse flags)\n");

		// The vendor pkey cert has already been verified
		// by the bootrom. For debugging check again here:
#ifdef RDA_CRYPTO_DEBUG
		// Dump the PKCERT data
		debug_dump_key_and_signature("PKCERT",
		        &info->pubkey, &vendor_cert->signature,
			&vendor_cert->c, sizeof(vendor_cert->c));

		int ret;
		ret = signature_check(
			(uint8_t*)&vendor_cert->c, sizeof(vendor_cert->c),
			&vendor_cert->signature,
			&info->pubkey);
		printf("PKCERT check return %d\n", ret);
#endif
		// copy public key certificate
		info->pubkey = vendor_cert->c.pubkey;
	}

	// Sanity check for PKEY
	if (memcmp(&info->pubkey, "RDASEd", 6) != 0) {
		puts_deb("(Public key for signature check invalid)\n");
		info->secure_mode = -1; // better: ROM_API_SECURITY_INVALID_PKEY
		return;
	}

	// Dump the keys and signature when debugging
	debug_dump_key_and_signature("PDL1 -",
	        &info->pubkey, &image->image_signature, NULL, 0);

	// Check if device is in R&D mode
	int rnd_status = check_rnd_certificate(&image->cert, &info->pubkey, info);
	if (rnd_status == 0) {
		puts("(R&D mode override) ");
		// ... and disable security...
		info->secure_mode = ROM_API_SECURITY_DISABLED;
	}
}


int set_security_context(struct spl_security_info *info, const void *_image)
{
	const struct rda_se_image *image = _image;

	memset(info, 0, sizeof(*info));

	if (memcmp(romapi->magic, "RDA API", 8) != 0) {
		puts("Board security: Not present\n");
		info->secure_mode = ROM_API_SECURITY_UNAVAILABLE;
	}
	else {
		puts("Board security: present ");

		info->version = romapi->version;
		get_chip_id(&info->chip_id);
		get_chip_true_random(info->random, 32);
		get_chip_unique(&info->chip_unique_id);

		get_device_security_context(image, info);

		switch (info->secure_mode) {
		case ROM_API_SECURITY_ENABLED:
			puts("and enabled.\n");
			break;
		case ROM_API_SECURITY_DISABLED:
			puts("but disabled.\n");
			break;
		case ROM_API_INVALID_KEYINDEX:
		case ROM_API_INVALID_VENDOR_ID:
			puts("but has invalid key-index or vendor-id!\n");
			break;
		default:
			puts("but has invalid configuration!\n");
			break;
		}
	}
	return info->secure_mode;
}

int image_sign_verify(const uint8_t *buffer, uint32_t len)
{
	spl_bd_t *spl_board_info = (spl_bd_t *)CONFIG_SPL_BOARD_INFO_ADDR;
	struct spl_security_info *info = &spl_board_info->spl_security_info;

	len -= sizeof(struct sig);
	const struct sig *signature = (const struct sig *)(buffer + len);

	puts("Verify image:\n");

	// Check security mode
	int secure_mode = info->secure_mode;
	switch (secure_mode) {
	case ROM_API_SECURITY_ENABLED:
		// Check the signature of the image
		debug_dump_key_and_signature("bootloader -",
		         &info->pubkey, signature, NULL, 0);
		return signature_check(buffer, len, signature, &info->pubkey);
	case ROM_API_SECURITY_DISABLED:
	case ROM_API_SECURITY_UNAVAILABLE:
		return 0;
	default:
		return secure_mode; // This is != 0 -> verify error
	}
}

int image_sign_verify_uimage(image_header_t *hdr)
{
	return image_sign_verify((const uint8_t *)hdr,
	                         image_get_image_size(hdr) + sizeof(struct sig));
}

#else
int set_security_context(struct spl_security_info *info, const void *_image)
{
	memset(info, 0, sizeof(*info));

	if (memcmp(romapi->magic, "RDA API", 8) == 0) {
		puts("Board security: present ");

		info->version = romapi->version;
		get_chip_unique(&info->chip_unique_id);
	}
	return ROM_API_SECURITY_DISABLED;
}

int image_sign_verify(const uint8_t *buffer, uint32_t len)
{
	return 0;
}
#endif

