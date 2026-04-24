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

/**
 * Decompress a file compressed with RLE algorithm.
 *
 * Algorithm:
 *  1. Check that the first byte is TYPE_RLE
 *  2. Read the compressed data block by block:
 *      - If MSB=1: run block → write `count` copies of the next byte.
 *      - If MSB=0: literal block → write the next `count` bytes.
 *      - Update the CRC32 on the decompressed bytes.
 *  3. Stop at the start of the trailer (after compressed data).
 *  4. Read the CRC32 from the trailer and compare it with the computed CRC32.
 */
// int rle_drcompress(FILE *fi, FILE *fo) {
//     FILE *fi = fopen(input_path, "rb");
//     FILE *fo = fopen(output_path, "wb");
//     // Paths should already be validated, this is just a safety check
//     if (!fi || !fo)
//         return rle_cleanup_failed_output(fi, fo, output_path, "cannot open input or output file");

//     // Calculate the size of compressed data (without header and trailer)
//     fseek(fi, 0, SEEK_END);
//     long file_size = ftell(fi);
//     long compressed_data_size = file_size - sizeof(rle_header_t) - sizeof(rle_trailer_t);
//     fseek(fi, 0, SEEK_SET);

//     // Read the header
//     rle_header_t h;
//     fread(&h, sizeof(h), 1, fi);
//     if (h.common.type != TYPE_RLE)
//         return rle_cleanup_failed_output(fi, fo, output_path, "Input file is not TYPE_RLE");

//     uint32_t crc_moving = 0xFFFFFFFF;

//     uint8_t write_buf[128];

//     int bytes_read = 0;

//     while (bytes_read < compressed_data_size) {
//         int c = fgetc(fi);
//         if (c == EOF)
//             break;
//         bytes_read++;

//         uint8_t count_byte = (uint8_t)c;
//         uint8_t count_value = (count_byte & 0b01111111) + 1;

//         if (bytes_read + ((count_byte & 0b10000000) ? 1 : count_value) > compressed_data_size) {
//             return rle_cleanup_failed_output(fi, fo, output_path, "the input file is corrupted or invalid");
//         }

//         // RUN
//         if (count_byte & 0b10000000) {
//             int run_byte = fgetc(fi);
//             if (run_byte == EOF)
//                 return rle_cleanup_failed_output(fi, fo, output_path, "unexpected EOF while reading run byte");
//             bytes_read++;
//             memset(write_buf, run_byte, count_value);
//         } else { // LITERAL
//             if (fread(write_buf, 1, count_value, fi) != count_value)
//                 return rle_cleanup_failed_output(fi, fo, output_path, "unexpected EOF while reading literal bytes");
//             bytes_read += count_value;
//         }
//         fwrite(write_buf, 1, count_value, fo);
//         crc_moving = crc32_update_from_buf(crc_moving, write_buf, count_value);
//     }

//     // here, the pointer is at the start of the trailer
//     crc_moving ^= 0xFFFFFFFF;

//     // read the calculated crc in the trailer
//     rle_trailer_t t;
//     fread(&t, sizeof(rle_trailer_t), 1, fi);

//     // check that values are the same
//     if (crc_moving == t.common.crc32)
//         printf("Decompression completed successfully. You can check the output file: %s\n", output_path);
//     else
//         return rle_cleanup_failed_output(fi, fo, output_path,
//                                          "decompression failed: input file is corrupted or invalid");

//     fclose(fi);
//     fclose(fo);
//     return 0;
// }