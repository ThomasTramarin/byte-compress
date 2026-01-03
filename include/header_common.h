#ifndef HEADER_COMMON_H
#define HEADER_COMMON_H

#include <stdint.h>
#define TYPE_RLE 0

/**
 * The common header is a byte that represents the compression algorithm used.
 * It is used to:
 *  - quickly detect if the user is trying to decompress the file with the wrong algorithm
 */
typedef struct __attribute__((packed)) {
    uint8_t type; // TYPE_*
} common_header_t;

#endif