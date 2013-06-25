#ifndef _UADE_MD5_H_
#define _UADE_MD5_H_

#include <sys/types.h>
#include <stdint.h>

typedef struct uade_MD5Context {
        uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
} uade_MD5_CTX;

void uade_MD5Init(uade_MD5_CTX *context);
void uade_MD5Update(uade_MD5_CTX *context, unsigned char const *buf, unsigned len);
void uade_MD5Final(unsigned char digest[16], uade_MD5_CTX *context);

#endif /* !_UADE_MD5_H_ */
