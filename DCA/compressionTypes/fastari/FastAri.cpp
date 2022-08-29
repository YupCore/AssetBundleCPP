#include "FastAri.h"
int FastAri::fa_compress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen)
{
	unsigned int low = 0, mid, high = 0xFFFFFFFF;
	size_t ipos, opos = 0;
	int bp, bv;
	int ctx;
	/* Initialize model */
	unsigned short* mdl = (unsigned short*)malloc((mask + 1) * sizeof(unsigned short));
	if (!mdl) return 1;
	for (ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Walk through input */
	for (ipos = 0; ipos < ilen; ++ipos) {
		bv = ibuf[ipos];
		for (bp = 7; bp >= 0; --bp) {
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if (bv & 1) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				ctx = ((ctx << 1) | 1) & mask;
			}
			else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			bv >>= 1;
			while ((high ^ low) < 0x1000000) {
				obuf[opos] = high >> 24;
				++opos;
				high = (high << 8) | 0xFF;
				low <<= 8;
			}
		}
	}
	/* Flush coder */
	obuf[opos] = (high >> 24); ++opos;
	obuf[opos] = (high >> 16) & 0xFF; ++opos;
	obuf[opos] = (high >> 8) & 0xFF; ++opos;
	obuf[opos] = high & 0xFF; ++opos;
	/* Cleanup */
	free(mdl);
	*olen = opos;
	return 0;
}
int FastAri::fa_decompress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t olen)
{
	unsigned int low = 0, mid, high = 0xFFFFFFFF, cur = 0, c;
	size_t opos = 0;
	int bp, bv;
	int ctx;
	/* Initialize model */
	unsigned short* mdl = (unsigned short*)malloc((mask + 1) * sizeof(unsigned short));
	if (!mdl) return 1;
	for (ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Initialize coder */
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	/* Walk through input */
	while (olen) {
		bv = 0;
		for (bp = 0; bp < 8; ++bp) {
			/* Decode bit */
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if (cur <= mid) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				bv |= (1 << bp);
				ctx = ((ctx << 1) | 1) & mask;
			}
			else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			while ((high ^ low) < 0x1000000) {
				high = (high << 8) | 0xFF;
				low <<= 8;
				c = *ibuf; ++ibuf;
				cur = (cur << 8) | c;
			}
		}
		*obuf = bv;
		++obuf;
		--olen;
	}
	/* Cleanup */
	free(mdl);
	return 0;
}