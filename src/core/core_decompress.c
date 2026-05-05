#include "algo.h"
#include "bcf.h"
#include "bcomp.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

bcomp_err_t bcomp_decompress_stream(FILE *in, FILE *out) {

    if (!in)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "input FILE* is null");

    if (!out)
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INVALID_ARG, "output FILE* is null");

    bcf_global_header_t gh;

    int r = bcf_gh_read(in, &gh);

    if (r < 0) {
        BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "error: failed to deserialize global header");
    }

    // READ LOOP
    while (1) {
        bcf_block_t bk;
        memset(&bk, 0, sizeof(bk));

        r = bcf_bk_read(in, gh.ver_major, &bk);

        if (r == BCF_ERR_EOF)
            break;

        if (r < 0) {
            free(bk.payload);
            BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "error: failed to read block");
        }

        bcf_tlvs tlvs;
        r = bcf_bk_parse_tlvs(&bk, &tlvs);

        if (r < 0) {
            free(bk.payload);
            BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "error: failed to parse tlvs");
        }

        // DATA
        if (bk.type == BCF_BK_TYPE_DATA) {
            bcf_tlv_entry_t *algo = bcf_tlvs_get(&tlvs, BCF_BK_TAG_DATA_ALGO_ID);
            if (!algo || algo->length != 1) {
                free(bk.payload);
                BCOMP_RETURN_ERR_MSG(BCOMP_ERR_INTERNAL, "error: failed to read algo id");
            }

            switch (algo->value[0]) {
            case BCF_BK_ALGO_RAW:
                algo_raw_decompress(&tlvs, out);
                // TODO: error check
                break;

            case BCF_BK_ALGO_RLE:
                algo_rle_decompress(&tlvs, out);
                break;
            }
        }

        free(bk.payload);
    }

    return (bcomp_err_t){.code = BCOMP_OK};
}