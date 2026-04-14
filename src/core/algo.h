#ifndef ALGORITHMS_H
#define ALGORITHMS_H
#include "bcomp_core.h"
#include "errors.h"

run_err_t rle_compress(
    const uint8_t *in_buf, size_t in_size,
    uint8_t *out_buf, size_t out_cap, compress_result_t *res);

#endif
