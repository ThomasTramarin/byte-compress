#include "bcf_block.h"
#include "bcomp.h"

bcomp_err_t algo_raw_decompress(const bcf_tlvs *tlvs, FILE *out) {
    bcf_tlv_entry_t *payload = bcf_tlvs_get(tlvs, BCF_BK_TAG_DATA_PAYLOAD);

    if (!payload) {
        return (bcomp_err_t){.code = BCOMP_ERR_INTERNAL};
    }

    // TODO: error
    fwrite(payload->value, 1, payload->length, out);

    return (bcomp_err_t){.code = BCOMP_OK};
}