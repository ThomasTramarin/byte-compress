/**
 * @file bcf.h
 *
 * @brief Interface for BCF serialization/deserialization
 */

#ifndef BCF_H
#define BCF_H

#include "version.h"
#include <stdint.h>

/* Global Header constants */
#define BCF_GH_MAGIC_LEN 4
#define BCF_GH_VERSION_LEN 2
#define BCF_GH_CRC_LEN 4

#define BCF_CURRENT_GH_VER_MAJOR BCOMP_VER_FORMAT_MAJOR
#define BCF_CURRENT_GH_VER_MINOR BCOMP_VER_FORMAT_MINOR

#define BCF_GH_V1_LEN 6 /* Number of bytes used in version 1.x */

/* Error codes */
typedef enum {
    BCF_SUCCESS = 0,
    BCF_ERR_INVALID_ARG = -1,
    BCF_ERR_BAD_MAGIC = -2,
    BCF_ERR_UNSUPPORTED = -3,
    BCF_ERR_CRC_MISMATCH = -4,
    BCF_ERR_INTERNAL = -5
} bcf_status_t;

/**
 * Global Header structure
 *
 * Any future fields added here will remain inside this struct to maintain
 * compatibility with older code and headers.
 *
 * @note CRC and MAGIC are not stored here. They are handled
 *       during serialization/deserialization.
 */
typedef struct {
    uint8_t ver_major;
    uint8_t ver_minor;
} bcf_global_header_t;

/* Functions */
int bcf_serialize_global_header(const bcf_global_header_t *hdr, uint8_t *out_buf);
int bcf_deserialize_global_header(bcf_global_header_t *out, const uint8_t *in_buf);

#endif
