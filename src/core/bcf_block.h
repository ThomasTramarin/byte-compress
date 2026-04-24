#ifndef BCF_BLOCK_H
#define BCF_BLOCK_H

#include "version.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BCF_BK_HDR_V1_LEN 20 // block_header is 20 bytes long

#define BCF_BK_TYPE_DATA 0x01

/* COMMON TAGS */
#define BCF_BK_TAG_NULL 0x00

/* DATA BLOCK */

#define BCF_BK_ALGO_RAW BCOMP_ALGO_RAW
#define BCF_BK_ALGO_RLE BCOMP_ALGO_RLE

#define BCF_BK_TAG_DATA_ALGO_ID 0x01
#define BCF_BK_TAG_DATA_PAYLOAD 0x02

/**
 *  Logical block
 *
 * Represents a single block in decoded form (after deserialization) or
 * in "ready to write" form (after bcf_bk_builder_commit).
 *
 * Ownership of `payload` depends on context:
 *   - encode: points into the builder's buffer (builder is owner)
 *   - decode: points into a caller-allocated read buffer (caller is owner)
 */
typedef struct {
    uint32_t seq_num; // starts at 0
    uint8_t type;     // block type
    uint32_t payload_size;
    uint8_t *payload; // pointer to TLV stream
} bcf_block_t;

/**
 * Encode side: staged flat-buffer builder
 *
 * Accumulates TLV entries into a single contiguous byte buffer.
 * The buffer grows automatically as entries are added.
 */
typedef struct {
    uint8_t *buf;  // byte buffer
    uint32_t cap;  // allocated capacity in bytes
    size_t offset; // bytes written so far
    uint8_t block_type;
    int error; // sticky error (BCF_SUCCESS = healthy)
} bcf_bk_builder_t;

/**
 * Decode side: one parsed TLV entry
 *
 * Populated by bcf_bk_tlv_deserialize().
 *
 * The `value` pointer points directly into the source buffer (zero-copy), do not free it.
 *
 * Tags are scoped to block_type: the same tag value may mean different
 * things in different block types. Unknown tags must be skipped.
 */
typedef struct {
    uint8_t tag;
    uint32_t length;
    const uint8_t *value;
} bcf_tlv_entry_t;

/* Builder Lifecycle */
int bcf_bk_builder_init(bcf_bk_builder_t *b, uint8_t block_type, uint32_t initial_cap);
void bcf_bk_builder_free(bcf_bk_builder_t *b);
void bcf_bk_builder_reset(bcf_bk_builder_t *b, uint8_t block_type);

/* Put helpers (encode) */
void bcf_bk_builder_put_uint8(bcf_bk_builder_t *b, uint8_t tag, uint8_t val);
void bcf_bk_builder_put_uint16(bcf_bk_builder_t *b, uint8_t tag, uint16_t val);
void bcf_bk_builder_put_uint32(bcf_bk_builder_t *b, uint8_t tag, uint32_t val);
void bcf_bk_builder_put_bytes(bcf_bk_builder_t *b, uint8_t tag, const uint8_t *data, uint32_t len);

/* Commit and write (encode) */
int bcf_bk_builder_commit(bcf_bk_builder_t *b, uint32_t seq_num, bcf_block_t *out);
int bcf_bk_builder_write(const bcf_block_t *block, uint8_t major, FILE *out);

/* TLV decode */
size_t bcf_bk_tlv_deserialize(const uint8_t *in, uint32_t in_size, bcf_tlv_entry_t *out);

#endif