/**
 * @file bcf.h
 *
 * @brief Interface for BCF serialization/deserialization
 */

#ifndef BCF_H
#define BCF_H

#include "bcf_block.h"
#include "bcf_global_header.h"

/* Error codes (used in serialization/deserialization functions) */
typedef enum {
    BCF_SUCCESS = 0,
    BCF_ERR_INVALID_ARG = -1,
    BCF_ERR_UNSUPPORTED_MAJOR = -2,
    BCF_ERR_UNSUPPORTED_MINOR = -3,
    BCF_ERR_BAD_MAGIC = -4,
    BCF_ERR_CRC_MISMATCH = -5,
    BCF_ERR_IO = -6,
    BCF_ERR_EOF = -7,
    BCF_ERR_TRUNCATED = -8,
    BCF_ERR_INTERNAL = -9,
    BCF_ERR_MEM = -10,
    BCF_ERR_BUILDER_DEAD = -11,
} bcf_status_t;

#endif
