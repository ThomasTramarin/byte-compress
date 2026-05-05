#include "algo.h"
#include <stdlib.h>
#include <string.h>

/**
 * Write a run block
 *
 * Format:
 * - 1 byte: MSB = 1 (flag run), lower 7 bits = count - 1 (range 1..128)
 * - 1 byte: the byte to repeat
 */
static bcomp_err_t rle_write_group_run(uint8_t byte, uint8_t count, uint8_t *out, size_t *out_idx, size_t out_cap) {
    // a RUN block is always 2 bytes long
    if (*out_idx + 2 > out_cap) {
        return (bcomp_err_t){.code = BCOMP_ERR_MEM, .msg = "output buffer too small"};
    }

    out[(*out_idx)++] = (count - 1) | 0x80; // set the MSB to 1
    out[(*out_idx)++] = byte;

    return (bcomp_err_t){.code = BCOMP_OK};
}

/**
 * Write a literal block
 *
 * Format:
 * - 1 byte: MSB = 0 (flag literal), lower 7 bits = count - 1 (range 1..128)
 * - N bytes: the literal bytes
 */
static bcomp_err_t rle_write_group_literal(uint8_t *lits, uint8_t count, uint8_t *out, size_t *out_idx, size_t out_cap) {
    // LITERAL block: 1 byte (header) + N byte
    if (*out_idx + 1 + count > out_cap) {
        return (bcomp_err_t){.code = BCOMP_ERR_MEM, .msg = "output buffer too small"};
    }

    // do nothing
    if (count <= 0) {
        return (bcomp_err_t){.code = BCOMP_OK};
    }

    out[(*out_idx)++] = (count - 1); // MSB = 0
    memcpy(&out[*out_idx], lits, count);
    *out_idx += count;

    return (bcomp_err_t){.code = BCOMP_OK};
}

/**
 * Compress a file using a RLE different implementation with MSB flag.
 *
 */
bcomp_err_t algo_rle_compress(bcf_bk_builder_t *b, const uint8_t *in_buf, uint32_t in_size) {
    if (in_size == 0)
        return (bcomp_err_t){.code = BCOMP_OK};

    // out tmp buffer
    uint32_t max_out = in_size + (in_size >> 7) + 16;
    uint8_t *tmp_out = malloc(max_out);
    if (!tmp_out)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_MEM, "RLE tmp buffer fail");

    size_t out_idx = 0;
    uint8_t lit_buf[128];
    uint32_t lit_len = 0;
    uint8_t run_byte = 0;
    uint32_t run_len = 0;
    bcomp_err_t err;

    for (uint32_t in_idx = 0; in_idx < in_size; in_idx++) {
        uint8_t byte = in_buf[in_idx];

        // RUN state
        if (run_len >= 3) {
            if (byte == run_byte && run_len < 128) {
                run_len++;
                continue;
            } else {
                // run finished
                err = rle_write_group_run(run_byte, (uint8_t)run_len, tmp_out, &out_idx, max_out);
                if (err.code != BCOMP_OK) {
                    free(tmp_out);
                    return err;
                }
                run_len = 0;
            }
        }

        // LIT state
        lit_buf[lit_len++] = byte;

        // if run starts
        if (lit_len >= 3 && lit_buf[lit_len - 1] == lit_buf[lit_len - 2] && lit_buf[lit_len - 2] == lit_buf[lit_len - 3]) {
            err = rle_write_group_literal(lit_buf, (uint8_t)(lit_len - 3), tmp_out, &out_idx, max_out);
            if (err.code != BCOMP_OK) {
                free(tmp_out);
                return err;
            }

            run_byte = byte;
            run_len = 3;
            lit_len = 0;

        } else if (lit_len == 128) { // if lit buf is full
            err = rle_write_group_literal(lit_buf, 128, tmp_out, &out_idx, max_out);
            if (err.code != BCOMP_OK) {
                free(tmp_out);
                return err;
            }
            lit_len = 0;
        }
    }

    // flush
    if (run_len >= 3) {
        err = rle_write_group_run(run_byte, (uint8_t)run_len, tmp_out, &out_idx, max_out);
        if (err.code != BCOMP_OK) {
            free(tmp_out);
            return err;
        }
    } else if (lit_len > 0) {
        err = rle_write_group_literal(lit_buf, (uint8_t)lit_len, tmp_out, &out_idx, max_out);
        if (err.code != BCOMP_OK) {
            free(tmp_out);
            return err;
        }
    }

    if (out_idx < in_size) {
        bcf_bk_builder_put_uint8(b, BCF_BK_TAG_DATA_ALGO_ID, BCF_BK_ALGO_RLE);
        bcf_bk_builder_put_bytes(b, BCF_BK_TAG_DATA_PAYLOAD, tmp_out, (uint32_t)out_idx);
    } else {
        bcf_bk_builder_put_uint8(b, BCF_BK_TAG_DATA_ALGO_ID, BCF_BK_ALGO_RAW);
        bcf_bk_builder_put_bytes(b, BCF_BK_TAG_DATA_PAYLOAD, in_buf, in_size);
    }

    free(tmp_out);
    return (bcomp_err_t){.code = BCOMP_OK};
}

bcomp_err_t algo_rle_decompress(const bcf_tlvs *tlvs, FILE *out) {
    bcf_tlv_entry_t *payload = bcf_tlvs_get(tlvs, BCF_BK_TAG_DATA_PAYLOAD);

    if (!payload) {
        return (bcomp_err_t){.code = BCOMP_ERR_INTERNAL};
    }

    const uint8_t *in = payload->value;
    uint32_t size = payload->length;

    uint32_t i = 0;

    while (i < size) {
        uint8_t count_byte = in[i++];

        uint8_t count = (count_byte & 0x7F) + 1;

        // RUN
        if (count_byte & 0x80) {
            if (i >= size)
                return (bcomp_err_t){.code = BCOMP_ERR_INTERNAL};

            uint8_t byte = in[i++];

            for (int j = 0; j < count; j++)
                fputc(byte, out);
        } else {
            // LITERAL

            if (i + count > size)
                return (bcomp_err_t){.code = BCOMP_ERR_INTERNAL};

            fwrite(&in[i], 1, count, out);
            i += count;
        }
    }

    return (bcomp_err_t){.code = BCOMP_OK};
}
