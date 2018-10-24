#ifndef DEMO_COMPRESSOR_HEADER
#define DEMO_COMPRESSOR_HEADER

#include <inttypes.h>

class Vec;

class Compressor
{
	private:
	void *lzo_wrkmem;

	public:
	void init();

	uintptr_t compress(uint8_t *input, uintptr_t insize, uint8_t *output, uintptr_t outsize);
};

#endif /* DEMO_COMPRESSOR_HEADER */
