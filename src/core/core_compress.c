#include "algorithms.h"
#include "bcff.h"
#include "bcomp_core.h"
#include "crc.h"
#include "errors.h"

/**
 * Execute the compress process by segmenting input data into frames.
 *
 * Data is processed in independent units (frames) with a maximum size
 * of BCFF_FRAME_MAX_SIZE.
 * Compression context resets at each frame boundary.
 */
run_err_t compress_engine(FILE *in, FILE *out, uint8_t algo_id) {
    // write global header
    bcff_header_t global_hdr = {
        .algorithm = algo_id,
        .flags = (in == stdin) ? BCFF_FLAG_STREAMING : 0,
    };
    write_bcff_header(&global_hdr, out);

    // initialize the global context
    compress_ctx_t ctx = {
        .global_crc = crc32_init(),
    };

    // buffers
    uint8_t in_buf[BCFF_FRAME_MAX_SIZE];
    uint8_t out_buf[BCFF_FRAME_MAX_SIZE + 1024]; // buffer for compressed payload

    compress_algo_fn_t compress_fn;

    switch (algo_id) {
    case BCFF_ALGO_RLE:
        compress_fn = rle_compress;
        break;
    default:
        compress_fn = NULL;
        break;
    }

    size_t bytes_read;
    int frames_written = 0;

    while (1) {
        bytes_read = fread(in_buf, 1, sizeof(in_buf), in);

        // If read error
        if (bytes_read == 0 && ferror(in)) {
            return (run_err_t){.code = RUN_ERR_IO, .msg = "error during reading input"};
        }

        // If the source is empty at the first frame (0 bytes)
        if (bytes_read == 0 && frames_written == 0 && feof(in)) {

            // write an empty frame with BCFF_FRAME_FLAG_LAST
            bcff_frame_header_t empty_hdr = {
                .flags = BCFF_FRAME_FLAG_LAST,
                .compressed_size = 0,
                .uncompressed_size = 0,
            };

            write_bcff_frame_header(&empty_hdr, NULL, out);

            break;
        }

        frames_written++;
        ctx.global_crc = crc32_update_buf(ctx.global_crc, in_buf, bytes_read);

        // compress
        compress_result_t res = {0};
        run_err_t err = compress_fn(in_buf, bytes_read, out_buf, sizeof(out_buf), &res);
        if (err.code != RUN_OK)
            return err;

        int is_last = feof(in) || (bytes_read < sizeof(in_buf));

        // prepare the frame header
        bcff_frame_header_t frame_hdr = {
            .last_byte_bits = res.last_byte_bits,
            .flags = is_last ? BCFF_FRAME_FLAG_LAST : 0,
            .compressed_size = res.out_written,
            .uncompressed_size = res.in_consumed};

        // write the frame header
        write_bcff_frame_header(&frame_hdr, out_buf, out);
        // write the payload
        fwrite(out_buf, 1, res.out_written, out);

        if (is_last)
            break;
    }

    ctx.global_crc = crc32_finalize(ctx.global_crc);

    // prepare the global trailer
    bcff_trailer_t trailer = {
        .crc32 = ctx.global_crc,
    };

    // write the trailer
    write_bcff_trailer(&trailer, out);

    return (run_err_t){.code = RUN_OK};
}