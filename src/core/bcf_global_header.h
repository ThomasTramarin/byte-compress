/**
 * @file bcf_global_header.h
 *
 * @brief Interface for BCF serialization/deserialization
 */

#ifndef BCF_GLOBAL_HEADER_H
#define BCF_GLOBAL_HEADER_H

#include "version.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Global Header constants */
#define BCF_GH_MAGIC_LEN 4
#define BCF_GH_VERSION_LEN 2
#define BCF_GH_CRC_LEN 4

#define BCF_CURRENT_GH_VER_MAJOR BCOMP_VER_FORMAT_MAJOR
#define BCF_CURRENT_GH_VER_MINOR BCOMP_VER_FORMAT_MINOR

#define BCF_GH_V1_LEN 6 /* Number of bytes used in version 1.x */

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
    // Fixed
    uint8_t ver_major;
    uint8_t ver_minor;

    // Specific
    uint32_t uncompressed_payload_size;
} bcf_global_header_t;

/* Functions */
int bcf_gh_serialize(const bcf_global_header_t *hdr, uint8_t *out_buf);
int bcf_gh_deserialize(bcf_global_header_t *out, const uint8_t *in_buf);
int bcf_gh_sizeof(const bcf_global_header_t *hdr);
int bcf_gh_sizeof_prefix(const uint8_t prefix[6]);

int bcf_gh_read(FILE *in, bcf_global_header_t *out);

#endif
