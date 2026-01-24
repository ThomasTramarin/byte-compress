#include "bcomp_core.h"
#include "crc.h"
#include "errors.h"
#include <string.h>

/**
 * Write a run block
 *
 * Format:
 * - 1 byte: MSB = 1 (flag run), lower 7 bits = count - 1 (range 1..128)
 * - 1 byte: the byte to repeat
 */
static run_err_t rle_write_group_run(uint8_t byte, uint8_t count, uint8_t *out, size_t *out_idx, size_t out_cap) {
    // a RUN block is always 2 bytes long
    if (*out_idx + 2 > out_cap) {
        return (run_err_t){.code = RUN_ERR_BUF_OVERFLOW, .msg = "output buffer too small"};
    }

    out[(*out_idx)++] = (count - 1) | 0x80; // set the MSB to 1
    out[(*out_idx)++] = byte;

    return (run_err_t){.code = RUN_OK};
}

/**
 * Write a literal block
 *
 * Format:
 * - 1 byte: MSB = 0 (flag literal), lower 7 bits = count - 1 (range 1..128)
 * - N bytes: the literal bytes
 */
static run_err_t rle_write_group_literal(uint8_t *lits, uint8_t count, uint8_t *out, size_t *out_idx, size_t out_cap) {
    // LITERAL block: 1 byte (header) + N byte
    if (*out_idx + 1 + count > out_cap) {
        return (run_err_t){.code = RUN_ERR_BUF_OVERFLOW, .msg = "output buffer too small"};
    }

    out[(*out_idx)++] = (count - 1); // MSB = 0
    memcpy(&out[*out_idx], lits, count);
    *out_idx += count;

    return (run_err_t){.code = RUN_OK};
}

/**
 * Compress a file using a RLE different implementation with MSB flag.
 *
 * @warning The FILE pointers `fi` and `fo` are not closed by this function.
 *          The caller must close them after the operation.
 *
 * Algorithm:
 *  1. Accumulate literal bytes until 3 identical bytes are found (start RUN)
 *  2. Flush literal bytes before the run
 *  3. Write the run until the byte changes or maximum length is reached (128)
 *  4. Repeat until EOF
 *  5. Update CRC32 during writing and append a trailer at the end.
//  */
run_err_t rle_compress(
    const uint8_t *in_buf, size_t in_size,
    uint8_t *out_buf, size_t out_cap, compress_result_t *res) {

    size_t in_idx = 0;
    size_t out_idx = 0;
    uint8_t lit_buf[128];
    size_t lit_len = 0;
    uint8_t run_byte = 0;
    size_t run_len = 0;
    run_err_t err;

    // iterate over each byte of the input
    for (in_idx = 0; in_idx < in_size; in_idx++) {
        uint8_t b = in_buf[in_idx];

        // RUN state
        if (run_len >= 3) {

            if (b == run_byte && run_len < 128) {
                run_len++;
                continue;
            } else {
                // finish run (or it is full)
                err = rle_write_group_run(run_byte, run_len, out_buf, &out_idx, out_cap);
                if (err.code != RUN_OK)
                    return err;
                run_len = 0;
            }
        }

        // LITERAL logic
        lit_buf[lit_len++] = b;

        // start of run if last 3 bytes of lit_buf are equal
        if (lit_len >= 3 &&
            lit_buf[lit_len - 1] == lit_buf[lit_len - 2] &&
            lit_buf[lit_len - 2] == lit_buf[lit_len - 3]) {

            // write previous literal bytes if any
            if (lit_len > 3) {
                err = rle_write_group_literal(lit_buf, lit_len - 3, out_buf, &out_idx, out_cap);
                if (err.code != RUN_OK)
                    return err;
            }
            run_byte = b;
            run_len = 3;
            lit_len = 0;
        } else if (lit_len == 128) {
            err = rle_write_group_literal(lit_buf, 128, out_buf, &out_idx, out_cap);
            if (err.code != RUN_OK)
                return err;
            lit_len = 0;
        }
    }

    // flush remaining bytes
    if (run_len >= 3) {
        err = rle_write_group_run(run_byte, run_len, out_buf, &out_idx, out_cap);
        if (err.code != RUN_OK)
            return err;
    } else if (lit_len > 0) {
        err = rle_write_group_literal(lit_buf, lit_len, out_buf, &out_idx, out_cap);
        if (err.code != RUN_OK)
            return err;
    }

    res->out_written = out_idx;
    res->in_consumed = in_idx;
    res->last_byte_bits = 8; // RLE works with full bytes

    return (run_err_t){.code = RUN_OK};
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