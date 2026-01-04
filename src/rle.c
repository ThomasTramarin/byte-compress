#include "rle.h"
#include "crc.h"
#include <stdio.h>
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
 * Handle operation failure.
 * Prints an error message, closes any open files, and deletes the output file if provided.
 */
int rle_cleanup_failed_output(FILE *fi, FILE *fo, const char *output_path, const char *msg) {
    if (msg)
        fprintf(stderr, "Error: %s\n", msg);

    if (fi)
        fclose(fi);
    if (fo) {
        fclose(fo);
        if (output_path) {
            if (remove(output_path) != 0) {
                fprintf(stderr, "Error: failed to remove incomplete output file: %s\n", output_path);
            }
        }
    }

    return 1; // return error code
}

/**
 * Compress a file using a RLE different implementation with MSB flag.
 *
 * Algorithm:
 *  1. Accumulate literal bytes until 3 identical bytes are found (start RUN)
 *  2. Flush literal bytes before the run
 *  3. Write the run until the byte changes or maximum length is reached (128)
 *  4. Repeat until EOF
 *  5. Update CRC32 during writing and append a trailer at the end.
 */
int rle_compress(const char *input_path, const char *output_path) {
    FILE *fi = fopen(input_path, "rb");
    FILE *fo = fopen(output_path, "wb");
    // Paths should already be validated, this is just a safety check
    if (!fi || !fo)
        return rle_cleanup_failed_output(fi, fo, output_path, "cannot open input or output file");

    // Write rle header (1 byte)
    rle_header_t h;
    h.common.type = TYPE_RLE;
    fwrite(&h, sizeof(h), 1, fo);

    uint32_t crc = 0xFFFFFFFF;

    uint8_t buffer[BLOCK_SIZE];  // read buffer
    size_t read;                 // the number of bytes read in the current block
    uint8_t literal_buffer[128]; // accumulate literal bytes
    size_t literal_len = 0;
    uint8_t run_byte;
    size_t run_len = 0;

    // read block by block
    while ((read = fread(buffer, 1, BLOCK_SIZE, fi)) > 0) {
        for (size_t i = 0; i < read; i++) {
            uint8_t b = buffer[i];

            crc = crc32_update(crc, b);

            // RUN state
            if (run_len >= 3) {

                if (b == run_byte) {
                    run_len++;

                    // flush full run
                    if (run_len == 128) {
                        rle_write_group_run(run_byte, run_len, fo);
                        run_len = 0;
                    }

                    continue;
                }

                // run ended -> write it
                rle_write_group_run(run_byte, run_len, fo);
                run_len = 0; // stop RUN state

                // current byte becomes literal
                literal_buffer[literal_len++] = b;
                continue;
            }

            // LITERAL state
            literal_buffer[literal_len++] = b;

            // start of run (3 identical bytes)
            if (literal_len >= 3 &&
                literal_buffer[literal_len - 1] == literal_buffer[literal_len - 2] &&
                literal_buffer[literal_len - 2] == literal_buffer[literal_len - 3]) {

                size_t n = literal_len - 3;

                // write literal bytes before RUN starts
                if (n > 0) {
                    rle_write_group_literal(literal_buffer, n, fo);
                }

                literal_len = 0;
                run_byte = b;
                run_len = 3; // start RUN state
                continue;
            }

            // flush literal if buffer full
            if (literal_len == 128) {
                rle_write_group_literal(literal_buffer, literal_len, fo);
                literal_len = 0;
            }
        }
    }

    // flush remaining bytes at EOF
    // if the status is RUN
    if (run_len >= 3) {
        rle_write_group_run(run_byte, run_len, fo);
    } else if (literal_len > 0) {
        // if the status is literal
        rle_write_group_literal(literal_buffer, literal_len, fo);
    }

    crc ^= 0xFFFFFFFF;

    // write the trailer
    rle_trailer_t t;
    t.common.crc32 = crc;
    fwrite(&t, sizeof(t), 1, fo);

    fclose(fi);
    fclose(fo);

    return 0;
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
int rle_decompress(const char *input_path, const char *output_path) {
    FILE *fi = fopen(input_path, "rb");
    FILE *fo = fopen(output_path, "wb");
    // Paths should already be validated, this is just a safety check
    if (!fi || !fo)
        return rle_cleanup_failed_output(fi, fo, output_path, "cannot open input or output file");

    // Calculate the size of compressed data (without header and trailer)
    fseek(fi, 0, SEEK_END);
    long file_size = ftell(fi);
    long compressed_data_size = file_size - sizeof(rle_header_t) - sizeof(rle_trailer_t);
    fseek(fi, 0, SEEK_SET);

    // Read the header
    rle_header_t h;
    fread(&h, sizeof(h), 1, fi);
    if (h.common.type != TYPE_RLE)
        return rle_cleanup_failed_output(fi, fo, output_path, "Input file is not TYPE_RLE");

    uint32_t crc_moving = 0xFFFFFFFF;

    uint8_t write_buf[128];

    int bytes_read = 0;

    while (bytes_read < compressed_data_size) {
        int c = fgetc(fi);
        if (c == EOF)
            break;
        bytes_read++;

        uint8_t count_byte = (uint8_t)c;
        uint8_t count_value = (count_byte & 0b01111111) + 1;

        if (bytes_read + ((count_byte & 0b10000000) ? 1 : count_value) > compressed_data_size) {
            return rle_cleanup_failed_output(fi, fo, output_path, "the input file is corrupted or invalid");
        }

        // RUN
        if (count_byte & 0b10000000) {
            int run_byte = fgetc(fi);
            if (run_byte == EOF)
                return rle_cleanup_failed_output(fi, fo, output_path, "unexpected EOF while reading run byte");
            bytes_read++;
            memset(write_buf, run_byte, count_value);
        } else { // LITERAL
            if (fread(write_buf, 1, count_value, fi) != count_value)
                return rle_cleanup_failed_output(fi, fo, output_path, "unexpected EOF while reading literal bytes");
            bytes_read += count_value;
        }
        fwrite(write_buf, 1, count_value, fo);
        crc_moving = crc32_update_from_buf(crc_moving, write_buf, count_value);
    }

    // here, the pointer is at the start of the trailer
    crc_moving ^= 0xFFFFFFFF;

    // read the calculated crc in the trailer
    rle_trailer_t t;
    fread(&t, sizeof(rle_trailer_t), 1, fi);

    // check that values are the same
    if (crc_moving == t.common.crc32)
        printf("Decompression completed successfully. You can check the output file: %s\n", output_path);
    else
        return rle_cleanup_failed_output(fi, fo, output_path,
                                         "decompression failed: input file is corrupted or invalid");

    fclose(fi);
    fclose(fo);
    return 0;
}