#include "bcf_block.h"
#include "bcf.h"
#include "bcomp_endian.h"
#include "crc.h"
#include <stdlib.h>
#include <string.h>

/**
 * VERSION 1.x COMMON HEADER BLOCK LOGIC
 *
 * 20 bytes long, same structure for every block type
 *
 * Layout:
 *   [0..3]   seq_num       (uint32 LE)
 *   [4]      block_type    (uint8)
 *   [5..7]   reserved      (3 bytes, zero-filled)
 *   [8..11]  payload_size  (uint32 LE)
 *   [12..15] payload_crc32 (uint32 LE) - CRC of the payload bytes
 *   [16..19] header_crc32  (uint32 LE) - CRC of bytes 0..15
 */
static int bk_hdr_v1_deserialize(bcf_block_t *out, const uint8_t *in_hdr_buf) {
    // header integrity check (first 16 bytes of the block)
    uint32_t hdr_crc_calc = crc32_calculate(in_hdr_buf, 16);
    uint32_t hdr_crc_saved = read_uint32_le(in_hdr_buf + 16); // read header_crc

    // if they don't match
    if (hdr_crc_saved != hdr_crc_calc) {
        return BCF_ERR_CRC_MISMATCH;
    }

    // save the data inside the struct
    out->seq_num = read_uint32_le(in_hdr_buf);
    out->type = in_hdr_buf[4];
    // 3 reserved bytes

    out->payload_size = read_uint32_le(in_hdr_buf + 8);

    // payload_crc is stored at [12..15] but is verified later,
    // after the full payload is read into memory

    return BCF_BK_HDR_V1_LEN; // length
}

static int bk_hdr_v1_serialize(const bcf_block_t *block, uint8_t *out_hdr_buf) {
    // [0..3] seq_num
    write_uint32_le(out_hdr_buf, block->seq_num);

    // [4] block_type
    out_hdr_buf[4] = block->type;

    // [5..7] reserved
    memset(out_hdr_buf + 5, 0, 3);

    // [8..11] payload_size
    write_uint32_le(out_hdr_buf + 8, block->payload_size);

    // [12..15] payload_crc: computed over the raw payload bytes
    uint32_t p_crc = crc32_calculate(block->payload, block->payload_size);
    write_uint32_le(out_hdr_buf + 12, p_crc);

    // [16..19] header_crc: computed over the first 16 bytes of this header
    uint32_t h_crc = crc32_calculate(out_hdr_buf, 16);
    write_uint32_le(out_hdr_buf + 16, h_crc);

    return BCF_BK_HDR_V1_LEN; // length
}

/**
 * BLOCK COMMON HEADER DISPATCH TABLE
 *
 * Dispatches only major versions
 */
typedef struct {
    uint8_t major;
    uint32_t header_size;
    int (*serialize)(const bcf_block_t *, uint8_t *);
    int (*deserialize)(bcf_block_t *, const uint8_t *);
} block_version_entry_t;

// dispatch table saved in memory
static const block_version_entry_t block_v_table[] = {
    {.major = 1, .header_size = BCF_BK_HDR_V1_LEN, .serialize = bk_hdr_v1_serialize, .deserialize = bk_hdr_v1_deserialize}};

// find a block_version_entry_t entry based on major version
static const block_version_entry_t *bk_find_entry(uint8_t major) {
    for (size_t i = 0; i < sizeof(block_v_table) / sizeof(block_v_table[0]); i++) {
        if (block_v_table[i].major == major)
            return &block_v_table[i];
    }
    return NULL;
}

/**
 * @brief Returns the size in bytes of the block header for a given major version.
 *
 * @param major BCF major version
 * @return Header size in bytes, or BCF_ERR_UNSUPPORTED_MAJOR
 */
static int bk_hdr_sizeof(uint8_t major) {
    const block_version_entry_t *entry = bk_find_entry(major);
    return entry ? (int)entry->header_size : BCF_ERR_UNSUPPORTED_MAJOR;
}

/**
 * @brief Returns the total size in bytes of a block (header + payload).
 *
 * @param block Pointer to the logical block
 * @param major BCF major version
 * @return Total size in bytes, or negative error code
 */
static int bk_sizeof(const bcf_block_t *block, uint8_t major) {
    const block_version_entry_t *entry = bk_find_entry(major);
    if (!entry)
        return BCF_ERR_UNSUPPORTED_MAJOR;

    return (int)entry->header_size + (int)block->payload_size;
}

/**
 * @brief Serializes the block header into a buffer.
 *
 * The caller must ensure `out` is at least bk_hdr_sizeof(major) bytes.
 *
 * @param block Pointer to the logical block (payload must already be set)
 * @param major BCF major version
 * @param out   Destination buffer
 * @return Number of bytes written, or negative error code
 */
static int bk_hdr_serialize(const bcf_block_t *block, uint8_t major, uint8_t *out) {
    const block_version_entry_t *entry = bk_find_entry(major);
    if (!entry)
        return BCF_ERR_UNSUPPORTED_MAJOR;

    return entry->serialize(block, out);
}

/**
 * @brief Deserializes a block header from a buffer into a bcf_block_t.
 *
 * Only the header fields are populated. The caller is responsible for
 * reading the payload separately.
 *
 * @param out    Destination struct
 * @param major  BCF major version
 * @param in_buf Source buffer (must be at least bk_hdr_sizeof(major) bytes)
 * @return Number of bytes consumed, or negative error code
 */
int bk_hdr_deserialize(bcf_block_t *out, uint8_t major, const uint8_t *in_buf) {
    const block_version_entry_t *entry = bk_find_entry(major);
    if (!entry)
        return BCF_ERR_UNSUPPORTED_MAJOR;

    return entry->deserialize(out, in_buf);
}

/* ========================================================================= */

/**
 * TLV (Tag-Length-Value)
 *
 * Each TLV unit encodes:
 *   - Tag:     1 byte - context-specific identifier (scope: block_type)
 *   - Length:  1-5 bytes — varint (Unsigned LEB128) encoded size of Value
 *   - Value:   L bytes — raw data
 */

/**
 * @brief Serializes an unsigned 32-bit integer using Unsigned LEB128 (varint).
 *
 * Each byte carries 7 bits of data. The MSB is the continuation bit:
 *   1 = more bytes follow, 0 = last byte.
 * Values are stored in little-endian
 *
 * @param out Destination buffer (must be at least 5 bytes)
 * @param num Value to encode
 * @return Number of bytes written (1–5)
 */
static uint32_t bk_tlv_varint_serialize(uint8_t *out, uint32_t num) {
    uint32_t i = 0;

    while (num >= 0x80 && i < 4) {

        // write 7 bits and set the continuation bit (MSB = 1)
        out[i++] = (uint8_t)(num & 0x7F | 0x80);
        num >>= 7;
    }

    // write the last byte (MSB = 0)
    out[i++] = (uint8_t)(num & 0x7F);

    return i; // bytes written
}

/**
 * @brief Deserializes an Unsigned LEB128 varint from a buffer.
 *
 * Reads up to 5 bytes. If the encoded value exceeds 32 bits (malformed input),
 * the value is truncated to fit in a uint32_t.
 *
 * @param in         Source buffer
 * @param max_len    Number of bytes available to read
 * @param bytes_read Output: number of bytes consumed (0 on failure)
 * @return Decoded uint32_t value
 */
static uint32_t bk_tlv_varint_deserialize(const uint8_t *in, size_t max_len, size_t *bytes_read) {
    uint32_t num = 0;
    int i = 0;

    // max 5 bytes: 5 byte * 7 bit = 35 bit
    while (i < max_len && i < 5) {
        // place the current 7-bit group in the correct position
        num |= (uint32_t)(in[i] & 0x7F) << (7 * i);

        // stop if the MSB (continuation bit) is 0

        if (!(in[i++] & 0x80))
            break;
    }

    if (bytes_read)
        *bytes_read = i;
    // if the number is longer than 35 bits: truncate it to fit inside a 4 byte unsigned integer
    return num;
}

/**
 * @brief Returns the total serialized size in bytes of a TLV unit.
 *
 * Size = 1 (Tag) + varint_size(length) + length
 *
 * @param length Value size in bytes
 * @return Total TLV size in bytes
 */
static size_t bk_tlv_sizeof(uint32_t length) {
    size_t varint_size = 1;
    uint32_t temp = length;

    // calculate the size of
    while (temp >= 0x80) {
        varint_size++;
        temp >>= 7;
    }

    return varint_size + 1 + length;
}

/**
 * @brief Serializes a TLV unit into a buffer.
 *
 * Writes: tag (1 byte) | length (varint) | value (length bytes).
 * Returns 0 if the output buffer is too small.
 *
 * @param out          Destination buffer
 * @param max_out_size Available bytes in out
 * @param tag          TLV tag
 * @param length       Length of value in bytes
 * @param value        Pointer to value bytes
 * @return Number of bytes written, or 0 on failure
 */
size_t bcf_bk_tlv_serialize(uint8_t *out, uint32_t max_out_size, uint8_t tag, uint32_t length, const uint8_t *value) {

    size_t total_required = bk_tlv_sizeof(length);

    if (total_required > max_out_size) {
        return 0;
    }

    size_t off = 0;
    out[off++] = tag;

    uint32_t varint_bytes = bk_tlv_varint_serialize(out + off, length);
    off += varint_bytes;

    memcpy(out + off, value, (size_t)length);

    off += (size_t)length;

    return off;
}

/**
 * @brief Deserializes one TLV unit from a buffer.
 *
 * The `out->value` pointer points directly into `in` (zero-copy).
 * The caller must not free or modify `in` while using the TLV entry.
 *
 * @param in      Source buffer (payload)
 * @param in_size Number of bytes available
 * @param out     Output TLV entry
 * @return Number of bytes consumed, or 0 on error (malformed input)
 */
size_t bcf_bk_tlv_deserialize(const uint8_t *in, uint32_t in_size, bcf_tlv_entry_t *out) {
    size_t off = 0;

    // read tag
    out->tag = in[off++];

    // read length
    size_t varint_bytes = 0;
    out->length = bk_tlv_varint_deserialize(in + off, in_size - off, &varint_bytes);

    if (varint_bytes == 0)
        return 0;

    off += varint_bytes;

    if (off + out->length > in_size) {
        return 0;
    }

    out->value = (uint8_t *)(in + off);

    off += out->length;

    return off;
}

/* ========================================================================= */

/**
 * BUILDER
 *
 * The builder accumulates TLV entries into a singe byte buffer.
 * All bcf_bk_builder_put_* functions serialize their value and delegate to bk_builder_append_tlv()
 *
 * Sticky error: once b->error is different than BCF_SUCCESS, all subsequent operations do nothing.
 * The error is checked at commit time.
 */

/**
 * @brief Initializes a builder
 *
 * The function allocates the internal payload buffer with the given initial capacity.
 *
 * @param b            Pointer to an uninitialized builder struct
 * @param block_type   BCF block type for the block being built
 * @param initial_cap  Initial capacity of the internal buffer in bytes (useful when the caller knows the size of the payload to reduce reallocations)
 * @return BCF_SUCCESS, or BCF_ERR_INVALID_ARG / BCF_ERR_MEM on failure
 */
int bcf_bk_builder_init(bcf_bk_builder_t *b, uint8_t block_type, uint32_t initial_cap) {
    if (!b || initial_cap == 0)
        return BCF_ERR_INVALID_ARG;

    b->buf = malloc(initial_cap);
    if (!b->buf)
        return BCF_ERR_MEM;

    b->block_type = block_type;
    b->cap = initial_cap;
    b->offset = 0;
    b->error = BCF_SUCCESS;

    return BCF_SUCCESS;
}

/**
 * @brief Frees the builder's internal buffer and marks it as dead.
 *
 * After this call, all put_* and reset operations do nothing.
 * The builder struct itself is not freed (it may be stack-allocated).
 *
 * b->error is set to BCF_ERR_BUILDER_DEAD
 *
 * @param b Pointer to the builder
 */
void bcf_bk_builder_free(bcf_bk_builder_t *b) {
    if (!b)
        return;

    free(b->buf);
    b->buf = NULL;
    b->cap = 0;
    b->offset = 0;
    b->block_type = 0;
    b->error = BCF_ERR_BUILDER_DEAD;
}

/**
 * @brief Resets the builder to start a new block, reusing the existing buffer.
 *
 * Does not reallocate, the buffer capacity is preserved.
 * Does nothing if the builder is dead (after bcf_bk_builder_free).
 *
 * @param b          Pointer to an initialized builder
 * @param block_type BCF block type for the next block
 */
void bcf_bk_builder_reset(bcf_bk_builder_t *b, uint8_t block_type) {
    if (!b)
        return;
    if (b->error == BCF_ERR_BUILDER_DEAD)
        return;

    b->offset = 0;
    b->block_type = block_type;
    b->error = BCF_SUCCESS;
}

/**
 * @brief Appends a TLV unit to the builder's internal buffer.
 *
 * This is the only function that writes to the buffer directly.
 *
 * Growth policy: grow exactly what's needed + 25% margin
 *
 * Sets b->error on failure (sticky). Subsequent calls become no-ops.
 *
 * @param b     Builder instance
 * @param tag   TLV tag
 * @param value Pointer to value bytes
 * @param len   Length of value in bytes
 */
static void bk_builder_append_tlv(bcf_bk_builder_t *b, uint8_t tag, const uint8_t *value, uint32_t len) {
    if (b->error)
        return;

    size_t needed = bk_tlv_sizeof(len);

    if (b->offset + needed > b->cap) {
        uint32_t new_cap = (uint32_t)((b->offset + needed) * 1.25f);

        // safety: never shrink, always at least needed
        if (new_cap < b->offset + needed)
            new_cap = (uint32_t)(b->offset + needed);

        uint8_t *tmp = realloc(b->buf, new_cap);
        if (!tmp) {
            b->error = BCF_ERR_MEM;
            return;
        }

        b->buf = tmp;
        b->cap = new_cap;
    }

    // serialize the tlv
    size_t written = bcf_bk_tlv_serialize(b->buf + b->offset, b->cap - b->offset, tag, len, value);

    if (written == 0) {
        b->error = BCF_ERR_INTERNAL;
        return;
    }

    b->offset += written;
}

void bcf_bk_builder_put_uint8(bcf_bk_builder_t *b, uint8_t tag, uint8_t val) {
    bk_builder_append_tlv(b, tag, &val, sizeof(val));
}

void bcf_bk_builder_put_uint16(bcf_bk_builder_t *b, uint8_t tag, uint16_t val) {
    uint8_t tmp[2];
    write_uint16_le(tmp, val);
    bk_builder_append_tlv(b, tag, tmp, sizeof(tmp));
}

void bcf_bk_builder_put_uint32(bcf_bk_builder_t *b, uint8_t tag, uint32_t val) {
    uint8_t tmp[4];
    write_uint32_le(tmp, val);
    bk_builder_append_tlv(b, tag, tmp, sizeof(tmp));
}

void bcf_bk_builder_put_bytes(bcf_bk_builder_t *b, uint8_t tag, const uint8_t *data, uint32_t len) {
    bk_builder_append_tlv(b, tag, data, len);
}

/**
 * @brief Finalizes the builder and populates a bcf_block_t.
 *
 * Zero-copy: out->payload points directly into the builder's buffer.
 *
 * The builder remains the owner of the buffer.
 *
 * Do not call bcf_bk_builder_free() or bcf_bk_builder_reset() until the block
 * has been written with bcf_bk_builder_write().
 *
 * @param b       Builder instance
 * @param seq_num Sequential block number (starts at 0)
 * @param out     Output logical block struct
 * @return BCF_SUCCESS, or the sticky error code if any put_* failed
 */
int bcf_bk_builder_commit(bcf_bk_builder_t *b, uint32_t seq_num, bcf_block_t *out) {
    if (!b || !out)
        return BCF_ERR_INVALID_ARG;
    if (b->error)
        return b->error;

    out->seq_num = seq_num;
    out->type = b->block_type;
    out->payload_size = (uint32_t)b->offset;
    out->payload = b->buf;

    return BCF_SUCCESS;
}

/**
 * @brief Serializes and writes a block (header + payload) to a FILE stream.
 *
 * The header is serialized to a stack buffer and written first,
 * followed by the raw payload bytes.
 *
 * @param block Pointer to the committed logical block
 * @param major BCF major version (determines header format)
 * @param out   Output FILE stream (must be open for writing)
 * @return BCF_SUCCESS, or negative error code on failure
 */
int bcf_bk_builder_write(const bcf_block_t *block, uint8_t major, FILE *out) {
    if (!block || !out)
        return BCF_ERR_INVALID_ARG;

    int hdr_size = bk_hdr_sizeof(major);
    if (hdr_size < 0)
        return hdr_size;

    uint8_t hdr[hdr_size];
    int r = bk_hdr_serialize(block, major, hdr);
    if (r < 0)
        return r;

    if (fwrite(hdr, 1, hdr_size, out) != (size_t)hdr_size) {
        return BCF_ERR_IO;
    }

    if (fwrite(block->payload, 1, block->payload_size, out) != (size_t)block->payload_size) {
        return BCF_ERR_IO;
    }

    return BCF_SUCCESS;
}