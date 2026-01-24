#include "rle.h"
#include "crc.h"
#include "errors.h"
#include <string.h>

#define BLOCK_SIZE 4096

/**
 * Write a run block to the file.
 *
 * Format:
 * - 1 byte: MSB = 1 (flag run), lower 7 bits = count - 1 (range 1..128)
 * - 1 byte: the byte to repeat
 */
static void rle_write_group_run(uint8_t byte, uint8_t count, FILE *fp) {
    uint8_t group[2];
    group[1] = byte;
    group[0] = count - 1;
    group[0] |= 0b10000000; // set the first bit to 1
    fwrite(group, 1, 2, fp);
}

/**
 * Write a literal block (non-repeated bytes) to the file.
 *
 * Format:
 * - 1 byte: MSB = 0 (flag literal), lower 7 bits = count - 1 (range 1..128)
 * - N bytes: the literal bytes
 */
static void rle_write_group_literal(uint8_t not_compressed[], uint8_t count, FILE *fp) {
    fputc(count - 1, fp);
    fwrite(not_compressed, 1, count, fp);
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
 */
run_err_t rle_compress(FILE *fi, FILE *fo) {
    if (!fi)
        return (run_err_t){.code = RUN_ERR_IO, .msg = "input file pointer is NULL", .sys_errno = errno};

    if (!fo)
        return (run_err_t){.code = RUN_ERR_IO, .msg = "output file pointer is NULL", .sys_errno = errno};

    // Write rle header (1 byte)
    rle_header_t h;
    h.common.type = TYPE_RLE;
    fwrite(&h, sizeof(h), 1, fo);

    uint32_t crc = 0xFFFFFFFF;

    uint8_t in_buf[BLOCK_SIZE]; // read buffer
    size_t read;                // the number of bytes read in the current block
    uint8_t lit_buf[128];       // accumulate literal bytes
    size_t lit_len = 0;
    uint8_t run_byte;
    size_t run_len = 0;

    // read block by block
    while ((read = fread(in_buf, 1, BLOCK_SIZE, fi)) > 0) {
        for (size_t i = 0; i < read; i++) {
            uint8_t b = in_buf[i];

            crc = crc32_update(crc, b);

            // RUN state
            if (run_len >= 3) {

                if (b == run_byte && run_len < 128) {
                    run_len++;
                    continue;
                } else {
                    // finish run (or it is full)
                    rle_write_group_run(run_byte, run_len, fo);
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
                    rle_write_group_literal(lit_buf, lit_len - 3, fo);
                }
                run_byte = b;
                run_len = 3;
                lit_len = 0;
            } else if (lit_len == 128) {
                rle_write_group_literal(lit_buf, 128, fo);
                lit_len = 0;
            }
        }
    }

    // flush remaining bytes
    if (run_len >= 3) {
        rle_write_group_run(run_byte, run_len, fo);
    } else if (lit_len > 0) {
        rle_write_group_literal(lit_buf, lit_len, fo);
    }

    crc ^= 0xFFFFFFFF;

    // write the trailer
    rle_trailer_t t;
    t.common.crc32 = crc;
    fwrite(&t, sizeof(t), 1, fo);

    return (run_err_t){.code = RUN_OK, .msg = NULL, .sys_errno = 0};
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