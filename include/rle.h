#ifndef RLE_H
#define RLE_H

#include "errors.h"
#include "header_common.h"
#include "trailer_common.h"
#include <stdio.h>

// the header is the same as common_header_t
typedef struct {
    common_header_t common;
} rle_header_t;

// the trailer is the same as trailer_common_t
typedef struct {
    common_trailer_t common;
} rle_trailer_t;

run_err_t rle_compress(FILE *fi, FILE *fo);
run_err_t rle_decompress(FILE *fi, FILE *fo);

#endif