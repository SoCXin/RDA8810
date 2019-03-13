#ifndef __RDA_CRYPTO_H__

#include <image.h>
#include <asm/arch/spl_board_info.h>

int set_security_context(struct spl_security_info *info, const void *image);

int image_sign_verify_uimage(image_header_t *hdr);
int image_sign_verify(const uint8_t *buffer, uint32_t len);

#endif
