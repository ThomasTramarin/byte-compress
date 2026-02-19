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
 * The first 6 bytes are necessary to determine the size of the entire
 * header.
 *
 * Major versions determine which handler to use via the dispatch table.
 * Minor versions are managed internally by the major version handler.
 */

#include "bcf.h"
#include "bcomp_endian.h"
#include "crc.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t BCF_GH_MAGIC_BYTES[4] = {'B', 'C', 'F', 0x00};

/**
 * Version 1.x logic (private)
 */

/**
 * @brief Validates the version 1.x header logic.
 *
 * @param hdr Pointer to the global header structure
 * @return BCF_SUCCESS if valid, or a negative error code
 */
static int gh_v1_validate(const bcf_global_header_t *hdr) {
    if (hdr->ver_minor != 0) {
        // currently only version 1.0 is supported
        return BCF_ERR_UNSUPPORTED_MINOR;
    }
    return BCF_SUCCESS;
}

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
    (void)hdr; // minor version already validated by gh_v1_validate

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
    (void)hdr; // minor version already validated by gh_v1_validate

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
 *  - validate: pointer to the function used to validate the header fields before serialize/deserialize the version-specific segment
 *  - serialize/deserialize: pointer to the version-specific serialize/deserialize function
 */
typedef struct {
    uint8_t major;
    uint32_t specific_size;
    int (*validate)(const bcf_global_header_t *);
    int (*serialize)(const bcf_global_header_t *, uint8_t *);
    int (*deserialize)(bcf_global_header_t *, const uint8_t *);
} gh_version_entry_t;

/* Table of supported major versions */
static const gh_version_entry_t gh_version_table[] = {
    {.major = 1, .specific_size = BCF_GH_V1_LEN, .validate = gh_v1_validate, .serialize = gh_v1_serialize, .deserialize = gh_v1_deserialize},
};

/* Lookup function for major version */
static inline const gh_version_entry_t *gh_find_entry(uint8_t major) {
    for (size_t i = 0;
         i < sizeof(gh_version_table) / sizeof(gh_version_table[0]);
         i++) {
        if (gh_version_table[i].major == major)
            return &gh_version_table[i];
    }

    return NULL;
}

/**
 * @brief Internal helper to validate and get entry
 */
static inline int gh_get_valid_entry(const bcf_global_header_t *hdr, const gh_version_entry_t **out_entry) {
    if (!hdr)
        return BCF_ERR_INVALID_ARG;

    const gh_version_entry_t *entry = gh_find_entry(hdr->ver_major);
    if (!entry)
        return BCF_ERR_UNSUPPORTED_MAJOR;

    int status = entry->validate(hdr);
    if (status != BCF_SUCCESS)
        return status;

    if (out_entry)
        *out_entry = entry;
    return BCF_SUCCESS;
}

/**
 * Public functions
 */

/**
 * @brief Calculates total size and validates the content.
 *
 * @param hdr Pointer to the header structure
 * @return Number of bytes written on success, or a negative bcf_status_t error code
 */
int bcf_gh_sizeof(const bcf_global_header_t *hdr) {
    const gh_version_entry_t *entry = NULL;
    int status = gh_get_valid_entry(hdr, &entry);
    if (status != BCF_SUCCESS)
        return status;

    return BCF_GH_MAGIC_LEN + BCF_GH_VERSION_LEN + entry->specific_size + BCF_GH_CRC_LEN;
}

/**
 * @brief Calculate the total size of the global header from the first 6 bytes
 *
 * This function is used to know how much memory allocate before reading
 * data from a stream.
 */
int bcf_gh_sizeof_prefix(const uint8_t prefix[6]) {
    if (!prefix)
        return BCF_ERR_INVALID_ARG;
    if (memcmp(prefix, BCF_GH_MAGIC_BYTES, BCF_GH_MAGIC_LEN) != 0)
        return BCF_ERR_BAD_MAGIC;

    const gh_version_entry_t *entry = gh_find_entry(prefix[4]);
    if (!entry)
        return BCF_ERR_UNSUPPORTED_MAJOR;

    return BCF_GH_MAGIC_LEN + BCF_GH_VERSION_LEN + entry->specific_size + BCF_GH_CRC_LEN;
}

/**
 * @brief Serializes the global header into a buffer.
 *
 * Dispatch:
 *  - Uses the major version to select the handler from the dispatch table.
 *  - Minor versions are managed internally by the major version handler
 *
 * @param hdr Pointer to the header structure to serialize
 * @param out_buf Pointer to destination buffer
 * @return Number of bytes written on success, or a negative bcf_status_t error code
 */
int bcf_gh_serialize(const bcf_global_header_t *hdr, uint8_t *out_buf) {
    if (!hdr || !out_buf)
        return BCF_ERR_INVALID_ARG;

    const gh_version_entry_t *entry = NULL;
    int status = gh_get_valid_entry(hdr, &entry);
    if (status != BCF_SUCCESS)
        return status;

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

    offset += (uint32_t)written;

    // INTEGRITY PART (CRC32)
    crc32_t crc = crc32_calculate(out_buf, offset);

    // Write CRC at the end of the data
    write_uint32_le(out_buf + offset, crc);
    offset += BCF_GH_CRC_LEN;

    return (int)offset;
}

/**
 * @brief Deserializes a global header from a buffer.
 *
 * @param out Pointer to the global header structure to populate
 * @param in_buf Pointer to the input buffer containing serialized header
 * @return Number of bytes consumed on success, or negative bcf_status_t error code
 */
int bcf_gh_deserialize(bcf_global_header_t *out, const uint8_t *in_buf) {
    if (!out || !in_buf)
        return BCF_ERR_INVALID_ARG;

    // Reset out struct
    memset(out, 0, sizeof(bcf_global_header_t));

    uint32_t offset = 0;

    // verify magic bytes
    if (memcmp(in_buf, BCF_GH_MAGIC_BYTES, BCF_GH_MAGIC_LEN) != 0) {
        return BCF_ERR_BAD_MAGIC;
    }

    offset += BCF_GH_MAGIC_LEN;

    // read major and minor versions
    out->ver_major = in_buf[offset++];
    out->ver_minor = in_buf[offset++];

    const gh_version_entry_t *entry = NULL;
    int status = gh_get_valid_entry(out, &entry);
    if (status != BCF_SUCCESS)
        return status;

    // deserialize version-specific segment
    int read_bytes = entry->deserialize(out, in_buf + offset);
    if (read_bytes < 0)
        return read_bytes;

    offset += (uint32_t)read_bytes;

    // verify CRC32
    uint32_t saved_crc = read_uint32_le(in_buf + offset);
    uint32_t calculated_crc = crc32_calculate(in_buf, offset);

    if (saved_crc != calculated_crc)
        return BCF_ERR_CRC_MISMATCH;

    offset += BCF_GH_CRC_LEN;

    return (int)offset;
}