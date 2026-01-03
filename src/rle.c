#include "rle.h"
#include <stdio.h>

#define BLOCK_SIZE 4096

/**
 * Write a run block to the file.
 * - MSB = 1 indicates a run
 * - 7 bits = count - 1 (range 1..128)
 * - Followed by the byte to repeat
 */
static void rle_write_group_run(uint8_t byte, uint8_t count, FILE *fp) {
    uint8_t group[2];
    group[1] = byte;
    group[0] = count - 1;
    group[0] |= 0b10000000; // set the first bit to 1
    fwrite(group, 1, 2, fp);
}

/**
 * Write a literal block to the file.
 * - MSB = 0 indicates literal
 * - 7 bits = count - 1 (range 1..128)
 * - Followed by the bytes to copy
 */
static void rle_write_group_literal(uint8_t not_compressed[], uint8_t count, FILE *fp) {
    fputc(count - 1, fp);
    fwrite(not_compressed, 1, count, fp);
}

/**
 * Compress a file using a RLE different implementation with MSB flag.
 *
 * Algorithm:
 *  1. Accumulate literal bytes until 3 identical bytes are found (start RUN)
 *  2. Flush literal bytes before the run
 *  3. Write the run until the byte changes or maximum length is reached (128)
 *  4. Repeat until EOF
 */
int rle_compress(const char *input_path, const char *output_path) {
    FILE *fi = fopen(input_path, "rb");
    FILE *fo = fopen(output_path, "wb");
    // Paths should already be validated, this is just a safety check
    if (!fi || !fo)
        return 1;

    // Write rle header (1 byte)
    rle_header_t h;
    h.common.type = TYPE_RLE;
    fwrite(&h, sizeof(h), 1, fo);

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

    fclose(fi);
    fclose(fo);

    return 0;
}
int rle_decompress(const char *input_path, const char *output_path) {}