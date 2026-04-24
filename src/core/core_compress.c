#include "algo.h"
#include "bcf.h"
#include "bcomp.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**
 * Compresses data from an input stream and writes the compressed
 * output to another stream using the BCF format.
 *
 * @param in
 *      Pointer to the input FILE stream containing raw data.
 *      The stream must be opened for writing.
 * @param out
 *      Pointer to the output FILE stream where the compressed BCF data
 *      will be written. The stream must be opened for writing.
 *
 * @param config
 *      Pointer to a bcomp_compression_config_t structure describing
 *      the compression behavior. Must not be NULL.
 *
 * @param res
 *      An optional pointer to a struct were informational data
 *      will be store on success.
 *
 * @return
 *      Optional pointer to a result structure that will be filled with
 *      informational data on success.
 */
bcomp_err_t bcomp_compress_stream(FILE *in, FILE *out,
                                  const bcomp_compression_config_t *config,
                                  bcomp_compress_result_t *res) {
    memset(res, 0, sizeof(bcomp_compress_result_t));

    if (!in)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "input FILE* is null");

    if (!out)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "output FILE* is null");

    if (config->algo > BCOMP_ALGO_MAX)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "invalid algorithm id");

    if (config->uncompressed_payload_size < BCOMP_UNCOMPRESSED_PAYLOAD_SIZE_MIN || config->uncompressed_payload_size > BCOMP_UNCOMPRESSED_PAYLOAD_SIZE_MAX) {
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "the selected block size is invalid");
    }

    // write the global header based on the newer version
    bcf_global_header_t gh = {
        .ver_major = BCF_CURRENT_GH_VER_MAJOR,
        .ver_minor = BCF_CURRENT_GH_VER_MINOR,
        .uncompressed_payload_size = config->uncompressed_payload_size,
    };

    // get the size of the header
    int gh_size = bcf_gh_sizeof(&gh);
    if (gh_size < 0)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_FORMAT, "global header validation failed");

    res->compressed_size = gh_size;

    // global header
    uint8_t *gh_buf = malloc(gh_size);

    if (!gh_buf)
        BCOMP_RETURN_ERR(BCOMP_ERR_MEM, "memory allocation error", errno);

    // serialize and write
    bcf_gh_serialize(&gh, gh_buf);
    fwrite(gh_buf, 1, gh_size, out);
    free(gh_buf);

    bcf_bk_builder_t b;
    bcf_bk_builder_init(&b, BCF_BK_TYPE_DATA, config->uncompressed_payload_size + 128);

    // COMPRESSION LOOP
    uint8_t *raw_buf = malloc(config->uncompressed_payload_size);
    if (!raw_buf) {
        BCOMP_RETURN_ERR(BCOMP_ERR_MEM, "failed to allocate read buffer", errno);
    }

    uint32_t seq_num = 0;
    size_t bytes_read;

    while ((bytes_read = fread(raw_buf, 1, config->uncompressed_payload_size, in)) > 0) {
        bcf_bk_builder_reset(&b, BCF_BK_TYPE_DATA);

        bcomp_err_t err = algo_rle_compress(&b, raw_buf, (uint32_t)bytes_read);

        if (err.code != BCOMP_OK) {
            free(raw_buf);
            bcf_bk_builder_free(&b);
            return err;
        }

        // commit
        bcf_block_t block;
        int r = bcf_bk_builder_commit(&b, seq_num, &block);

        if (r != 0) {
            free(raw_buf);
            bcf_bk_builder_free(&b);
            BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "builder commit failed");
        }

        r = bcf_bk_builder_write(&block, BCF_CURRENT_GH_VER_MAJOR, out);
        if (r != 0) {
            free(raw_buf);
            bcf_bk_builder_free(&b);
            BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "failed to write block to disk");
        }

        if (res) {
            res->original_size += bytes_read;
            res->compressed_size += block.payload_size + BCF_BK_HDR_V1_LEN;
            res->blocks_processed++;
        }

        seq_num++;
    }

    free(raw_buf);
    bcf_bk_builder_free(&b);

    if (ferror(in)) {
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_IO, "error reading input stream");
    }

    return (bcomp_err_t){
        .code = BCOMP_OK,
    };
}