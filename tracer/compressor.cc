#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compressor.h"
#include "util.h"
#include "dmath.h"
#include "../lzo/minilzo.h"

void Compressor::init()
{
	this->lzo_wrkmem = malloc_or_die(LZO1X_1_MEM_COMPRESS);

    if(lzo_init() != LZO_E_OK) {
		Util::die("cannot init lzo\n");
	}
}

uintptr_t Compressor::compress(uint8_t *input, uintptr_t insize, uint8_t *output, uintptr_t outsize)
{
	lzo_uint out_len = outsize;
	int ret = lzo1x_1_compress(input, insize, output, &out_len, lzo_wrkmem);
	if(ret != LZO_E_OK) {
		Util::die("lzo compression failed\n");
	}

	return out_len;
}
