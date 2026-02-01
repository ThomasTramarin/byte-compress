#include "bcf.h"
#include "core_helpers.h"
#include "crc.h"
#include <string.h>

/**
 * Serializes and writes the BCF global header to a file stream.
 *
 * - automatically writes the magic
 * - reads from the struct version, algorithm and flags
 * - computes and writes the crc internally
 */
void write_bcf_header(bcf_header_t *h, FILE *f) {
    uint8_t buf[12];

    memcpy(h->magic, "BCF", 4);
    memcpy(buf, h->magic, 4);

    buf[4] = BCF_CURRENT_VERSION;
    buf[5] = h->algorithm;

    to_be16(&buf[6], h->flags);

    // Calculate CRC of the first 8 bytes
    crc32_t crc = crc32_init();
    crc = crc32_update_buf(crc, buf, 8);
    h->crc32 = crc32_finalize(crc);

    to_be32(&buf[8], h->crc32);

    fwrite(buf, 1, sizeof(buf), f);
}

/**
 * Serializes and writes the BCF frame header to a file stream.
 * - reads from the struct last_byte_bits, flags, uncompressed_size, compressed_size
 * - computes and writes the crc internally
 */
void write_bcf_frame_header(bcf_frame_header_t *h, const uint8_t *payload, FILE *f) {
    uint8_t buf[16];

    memcpy(buf, "FH", 2);

    buf[2] = h->flags;
    buf[3] = h->last_byte_bits;

    to_be32(&buf[4], h->uncompressed_size);
    to_be32(&buf[8], h->compressed_size);

    // Calculate the CRC of the first 12 bytes (the header without the crc field) and the compressed payload
    crc32_t crc = crc32_init();
    crc = crc32_update_buf(crc, buf, 12);
    crc = crc32_update_buf(crc, payload, h->compressed_size);
    h->crc32 = crc32_finalize(crc);

    to_be32(&buf[12], h->crc32);

    fwrite(buf, 1, 16, f);
}

void write_bcf_trailer(bcf_trailer_t *h, FILE *f) {
    uint8_t buf[8];

    memcpy(buf, "BEND", 4);

    to_be32(&buf[4], h->crc32);

    fwrite(buf, 1, 8, f);
}
