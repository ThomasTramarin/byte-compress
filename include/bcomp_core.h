#ifndef BCOMP_CORE_H
#define BCOMP_CORE_H

#include "bcff.h"
#include "errors.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t global_crc;
    /**< CRC of the entire stream (original data), will be written at the end of the stream (bcff_trailer) */
} compress_ctx_t;

typedef struct {
    size_t out_written;
    /**< Number of bytes written in out_buf */

    size_t in_consumed;
    /**< Number of bytes considered in this frame  */

    uint8_t last_byte_bits;
    /**< Number of bits of the last byte to consider */

} compress_result_t;

typedef run_err_t (*compress_algo_fn_t)(
    const uint8_t *in_buf, size_t in_size,
    uint8_t *out_buf, size_t out_cap, compress_result_t *res);

run_err_t compress_engine(FILE *in, FILE *out, uint8_t algo_id);

#endif