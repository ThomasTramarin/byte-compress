#ifndef RLE_H
#define RLE_H

#include "header_common.h"

// the header is the same as common_header_t
typedef struct {
    common_header_t common;
} rle_header_t;

int rle_compress(const char *input_path, const char *output_path);
int rle_decompress(const char *input_path, const char *output_path);

#endif