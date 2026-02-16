/**
 * @file global_header.c
 *
 * @brief Implementation of BCF Global Header serialization and deserialization.
 *
 * This file provides functions to serialize and deserialize the global header
 * of the BCF format. It supports multiple major versions via a version dispatch table
 *
 *
 * Layout of the global header (in memory):
 *  - 4 bytes: MAGIC ('B', 'C', 'F', 0x00)
 *  - 1 byte : Major version
 *  - 1 byte : Minor version
 *  - N bytes: Version-specific data (size varies by major version)
 *  - 4 bytes: CRC32 of all preceding bytes
 *
 * Major versions determine which handler to use via the dispatch table.
 * Minor versions are managed internally by the major version handler.
 */

#include "bcf.h"
#include "bcomp_endian.h"
#include "crc.h"
#include <string.h>

static const uint8_t BCF_GH_MAGIC_BYTES[4] = {'B', 'C', 'F', 0x00};

/**
 * Version 1.x logic (private)
 */

/**
 * @brief Serializes the version 1.x specific part of the header.
 *
 * Version 1 reserves 6 bytes, currently (in version 1.0) zero-filled.
 *
 * @param hdr Pointer to the global header structure
 * @param out_specific Pointer to the buffer where version-specific bytes are written
 * @return Number of bytes written (6), or negative error code
 */
static int gh_v1_serialize(const bcf_global_header_t *hdr, uint8_t *out_specific) {
    if (hdr->ver_minor != 0) {
        // currently only version 0 is supported
        return BCF_ERR_UNSUPPORTED;
    }

    memset(out_specific, 0, BCF_GH_V1_LEN);
    return BCF_GH_V1_LEN;
}

/**
 * @brief Deserializes the version 1.x specific part of the header.
 *
 * Since version 1 does not store additional data, this function
 * simply returns the number of bytes to skip.
 *
 * @param hdr Pointer to the header structure to populate
 * @param in_specific Pointer to the version-specific bytes in the buffer
 * @return Number of bytes read (6), or negative error code
 */
static int gh_v1_deserialize(bcf_global_header_t *hdr, const uint8_t *in_specific) {
    if (hdr->ver_minor != 0) {
        // currently only version 0 is supported
        return BCF_ERR_UNSUPPORTED;
    }

    (void)in_specific;
    return BCF_GH_V1_LEN;
}

/**
 * VERSION DISPACTH TABLE
 */

/**
 * @brief Dispatch table entry definition
 *
 * Each entry correstonds to a major version and points to version-specific
 * serialize/deserialize functions.
 *
 * Fields:
 *  - major: the major version number
 *  - specific_size: the number of bytes for the version-specific segment
 *  - serialize/deserialize: pointer to the version-specific serialize/deserialize function
 */
typedef struct {
    uint8_t major;
    uint32_t specific_size;
    int (*serialize)(const bcf_global_header_t *, uint8_t *);
    int (*deserialize)(bcf_global_header_t *, const uint8_t *);
} gh_version_entry_t;

/* Table of supported major versions */
static const gh_version_entry_t gh_version_table[] = {
    {.major = 1, .specific_size = BCF_GH_V1_LEN, .serialize = gh_v1_serialize, .deserialize = gh_v1_deserialize},
};

/* Lookup function for major version */
static const gh_version_entry_t *gh_find_entry(uint8_t major) {
    for (size_t i = 0;
         i < sizeof(gh_version_table) / sizeof(gh_version_table[0]);
         i++) {
        if (gh_version_table[i].major == major)
            return &gh_version_table[i];
    }

    return NULL;
}

/**
 * Public functions
 */

/**
 * @brief Serializes the global header into a buffer.
 *
 * Dispatch:
 *  - Uses the major version to select the handler from the dispatch table.
 *  - Minor versions are managed internally by the major version handler
 *
 * @param hdr Pointer to the header structure to serialize
 * @param out_buf Pointer to destination buffer. If NULL, returns required buffer size
 *                (to allocate the correct number of bytes)
 * @return Number of bytes written on success, or a negative bcf_status_t error code
 */
int bcf_serialize_global_header(const bcf_global_header_t *hdr, uint8_t *out_buf) {
    if (!hdr)
        return BCF_ERR_INVALID_ARG;

    const gh_version_entry_t *entry = gh_find_entry(hdr->ver_major);

    if (!entry)
        return BCF_ERR_UNSUPPORTED;

    uint32_t total_size = BCF_GH_MAGIC_LEN + BCF_GH_VERSION_LEN + entry->specific_size + BCF_GH_CRC_LEN;

    if (!out_buf)
        return total_size;

    uint32_t offset = 0;

    // FIXED PART (MAGIG + version)
    memcpy(out_buf, BCF_GH_MAGIC_BYTES, BCF_GH_MAGIC_LEN);
    offset += BCF_GH_MAGIC_LEN;

    out_buf[offset++] = hdr->ver_major;
    out_buf[offset++] = hdr->ver_minor;

    // VERSION-SPECIFIC (Starting from Offset 6)
    int written = entry->serialize(hdr, out_buf + offset);
    if (written < 0)
        return written;

    if ((uint32_t)written != entry->specific_size)
        return BCF_ERR_INTERNAL;

    offset += written;

    // INTEGRITY PART (CRC32)
    crc32_t crc = crc32_calculate(out_buf, offset);

    // Write CRC at the end of the data
    write_uint32_le(out_buf + offset, crc);
    offset += BCF_GH_CRC_LEN;

    return offset;
}

/**
 * @brief Deserializes a global header from a buffer.
 *
 * @param out Pointer to the global header structure to populate
 * @param in_buf Pointer to the input buffer containing serialized header
 * @return Number of bytes consumed on success, or negative bcf_status_t error code
 */
int bcf_deserialize_global_header(bcf_global_header_t *out, const uint8_t *in_buf) {
    if (!out || !in_buf)
        return BCF_ERR_INVALID_ARG;

    uint32_t offset = 0;

    // verify magic bytes
    if (memcmp(in_buf, BCF_GH_MAGIC_BYTES, BCF_GH_MAGIC_LEN) != 0) {
        return BCF_ERR_BAD_MAGIC;
    }

    offset += BCF_GH_MAGIC_LEN;

    // read major and minor versions
    out->ver_major = in_buf[offset++];
    out->ver_minor = in_buf[offset++];

    // lookup dispatch table for major version
    const gh_version_entry_t *entry = gh_find_entry(out->ver_major);

    if (!entry)
        return BCF_ERR_UNSUPPORTED;

    // deserialize version-specific segment
    int read_bytes = entry->deserialize(out, in_buf + offset);
    if (read_bytes < 0)
        return read_bytes;

    offset += read_bytes;

    // verify CRC32
    uint32_t saved_crc = read_uint32_le(in_buf + offset);
    uint32_t calculated_crc = crc32_calculate(in_buf, offset);

    if (saved_crc != calculated_crc)
        return BCF_ERR_CRC_MISMATCH;

    offset += BCF_GH_CRC_LEN;

    return offset;
}