#ifndef TRAILER_COMMON_H
#define TRAILER_COMMON_H

#include <stdint.h>

/**
 * Common trailer for compressed files
 * Placed at the end of the file, after the compressed data.
 */
typedef struct __attribute__((packed)) {
    uint32_t crc32; // CRC32 of original data (the original file)
} common_trailer_t;

#endif