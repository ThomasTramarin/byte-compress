#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <stddef.h>
#include <stdint.h>

#include "bcf.h"
#include "bcomp.h"

typedef bcomp_err_t (*bcomp_compress_fn)(
    const uint8_t *in_buf,
    uint32_t in_size,
    bcf_bk_builder_t *b);

bcomp_err_t algo_rle_compress(bcf_bk_builder_t *b, const uint8_t *in_buf, uint32_t in_size);
bcomp_err_t algo_raw_decompress(const bcf_tlvs *tlvs, FILE *out);

bcomp_err_t algo_rle_decompress(const bcf_tlvs *tlvs, FILE *out);

#endif
