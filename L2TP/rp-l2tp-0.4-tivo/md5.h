typedef unsigned int uint32;

struct MD5Context
{
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

