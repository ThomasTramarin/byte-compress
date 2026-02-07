#ifndef BCF_H
#define BCF_H

#include "bcomp.h"
#include "version.h"
#include <stdint.h>
#include <stdio.h>

/**
 * BCF (BComp Format)
 * ----------------------------------------------------------------------------
 * Data Layout: [Global Header] + [Block 0] + ... + [Block N] + [Global Trailer]
 *
 * * DESIGN RULES:
 * 1. Structures are 8-byte aligned (multiples of 8).
 * 2. Little-Endian byte order for multi-byte fields.
 * 3. Each block is self-contained
 * 4. The format allows decompression without seeking (streaming)
 */

/* --- Magic Signatures --- */
#define BCF_MAGIC "BCF!"
#define BCF_BLOCK_MAGIC "BLK"

/* --- Block Types ---*/
#define BCF_BLOCK_DATA 0x01
#define BCF_BLOCK_TRAILER 0xFF

/* --- Algorithm Identifiers --- */
#define BCF_ALGO_RAW BCOMP_ALGO_RAW
#define BCF_ALGO_RLE BCOMP_ALGO_RLE

/**
 * GLOBAL HEADER
 *
 * Total size: 16 bytes
 */
typedef struct {
    uint8_t magic[4];
    /**< ASCII string "BCF!", used to identify the file format */

    uint8_t ver_major; /**< From BCOMP_VER_FORMAT_MAJOR */
    uint8_t ver_minor; /**< From BCOMP_VER_FORMAT_MINOR */
    uint8_t ver_patch; /**< From BCOMP_VER_FORMAT_PATCH */

    uint8_t reserved[5];

    uint32_t header_crc;
    /**< CRC of the global header*/
    
} bcf_header_t;

/**
 * BLOCK HEADER
 *
 * Common header for every chunk of data.
 * Total size: 24 bytes
 */
typedef struct {
    uint8_t magic[3];
    /**< ASCII string "BLK" */

    uint8_t type;
    /**< BCF_BLOCK_* */

    uint32_t n_block;
    /**< Sequential block identifier */

    uint32_t payload_len;
    /**< Length of the payload FOLLOWING this header */

    uint8_t reserved[4];

    uint32_t payload_crc;
    /**< CRC of the entire payload (next payload_len bytes) */
    
    uint32_t header_crc;
    /**< Integrity check for the header itself. */
    
} bcf_block_header_t;

/**
 * DATA PAYLOAD HEADER
 *
 * Total size: 16 bytes
 */

typedef struct {
    uint8_t algo; // BCF_ALGO_*
    uint8_t reserved[3];
    uint32_t original_size;
    uint32_t original_crc; // CRC of the original payload
} bcf_data_block_header_t;

/**
 * RLE DATA BLOCK HEADER
 *
 * Size: 8 bytes
 */
typedef struct {
    uint8_t reserved;
} bcf_data_block_rle_header_t;

/**
 * TRAILER BLOCK
 *
 * Size: 8 bytes
 */
typedef struct {
    uint32_t total_blocks;
    uint8_t reserved[4];
    uint64_t total_size;
} bcf_trailer_payload_t;

#endif
