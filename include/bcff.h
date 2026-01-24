#ifndef BCFF_H
#define BCFF_H
#include <stdint.h>
#include <stdio.h>

/**
 * BCFF (BComp File Format)
 *
 * This file defines the core structures used for compressed data storage and streaming.
 *
 * Rules:
 *  - All multi-byte integer fields are stored in big-endian
 *  - C structs must not be written directly to disk (use wrapper function instead)
 */

/**
 * Progressive format number versioning
 */
#define BCFF_VERSION_V1 0x01
#define BCFF_CURRENT_VERSION BCFF_VERSION_V1

/**
 * Compression algorithm identifiers.
 * The algorithm type is stored in the BCFF header.
 *
 * The type starts from 1.
 */
#define BCFF_ALGO_RLE 0x01

/**
 * BCFF_FLAG_STREAMING
 *
 * Indicates that the compressed data is produced from a streaming source (e.g. stdin, pipe, socket).
 *
 * This flag is not set when input is a regular file
 */
#define BCFF_FLAG_STREAMING 0x0001

/**
 * BCFF_FLAG_METADATA
 *
 *
 * Indicates that file metadata blocks are present in the stream.
 *
 * Metadata blocks may include information such as:
 *  - original file or directory name
 *  - timestamps
 *  - permissions
 *  - directory structure information
 *
 * When this flag is set, the compressed stream contains metadata
 * to reconstruct files and directories during decompression.
 *
 * When this flag is not set, the stream represents raw data only, with
 * no associated filesystem metadata.
 *
 * This flag is typically required when compressing multiple files or
 * directories, as metadata is needed to rebuild the original structure.
 *
 * @note Current version of bcomp do not store metadata blocks.
 *       This flag is reserved for future use.
 */

#define BCFF_FLAG_METADATA 0x0002

/**
 * The main header of a BCFF compressed file or stream.
 *
 * This header is always written at the beginning of the compressed stream
 * and contains information to interpret the data.
 */
typedef struct {
    uint8_t magic[4];
    /**< ASCII string "BCFF", used to identify the file format */

    uint8_t version;
    /**< BCFF format version */

    uint8_t algorithm;
    /**< Compression algorithm identifier (BCFF_ALGO_*) */

    uint16_t flags;
    /**< Global format flags (BCFF_FLAG_*) */

    uint32_t crc32;
    /**< CRC-32 of the global header fields execpt of this field */
} bcff_header_t;

/**
 * BCFF_FRAME_FLAG_LAST
 *
 * Indicates this is the last frame in the stream.
 */
#define BCFF_FRAME_FLAG_LAST 0x01

#define BCFF_FRAME_MAX_SIZE 64536

/**
 * BCFF Frame Header
 * A BCFF compressed stream contains a sequence of one or more frames.
 *
 * Each Frame is independent, meaning that each new frame resets the
 * compression context.
 */
typedef struct {
    uint8_t magic[2];
    /**< ASCII string "FH" */

    uint8_t flags;
    /**< Frame specific flags (BCFF_FRAME_FLAG_*) */

    uint8_t last_byte_bits;
    /**< Number of valid bits in the last byte of the compressed data. */

    uint32_t uncompressed_size;
    /**< The size of the data after decompression. */

    uint32_t compressed_size;
    /**< The size of the data stored in this frame (payload only).*/

    uint32_t crc32;
    /**< CRC-32 checksum calculated of header fields (except of crc32) and compressed payload. */
} bcff_frame_header_t;

/**
 * BCFF Trailer
 */
typedef struct {
    uint8_t magic[4];
    /**< ASCII string "BEND" (Bcomp END) */

    uint32_t crc32;
    /**< CRC-32 of the entire uncompressed data sequence */
} bcff_trailer_t;

// --- Functions ---
void write_bcff_header(bcff_header_t *h, FILE *f);
void write_bcff_frame_header(bcff_frame_header_t *h, const uint8_t *payload, FILE *f);
void write_bcff_trailer(bcff_trailer_t *h, FILE *f);

#endif
