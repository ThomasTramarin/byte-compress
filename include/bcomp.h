#ifndef BCOMP_H
#define BCOMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// ----- ALGORITHMS -----
// #define BCOMP_ALGO_AUTO 0 // auto-select best algorithm (future implementation)
#define BCOMP_ALGO_RAW 1 // raw copy if compression is not efficient
#define BCOMP_ALGO_RLE 2 // run-length encoding
#define BCOMP_ALGO_MAX BCOMP_ALGO_RLE

// ----- UNCOMPRESSED PAYLOAD SIZE LIMITS -----
#define BCOMP_UNCOMPRESSED_PAYLOAD_SIZE_MIN (4 * 1024)         // 4 KB
#define BCOMP_UNCOMPRESSED_PAYLOAD_SIZE_DEFAULT (64 * 1024)    // 64 KB
#define BCOMP_UNCOMPRESSED_PAYLOAD_SIZE_MAX (16 * 1024 * 1024) // 16 MB

// ----- ERRORS -----
typedef enum {
    BCOMP_OK = 0,
    BCOMP_ERR_IO,
    BCOMP_ERR_MEM,
    BCOMP_ERR_INVALID_ARG,
    BCOMP_ERR_INVALID_FORMAT,
} bcomp_err_code;

typedef struct {
    bcomp_err_code code;
    const char *msg; // optional
    int sys_errno;   // optional
} bcomp_err_t;

#define BCOMP_RETURN_ERR(_code, _msg, _errno) \
    do {                                      \
        return (bcomp_err_t){                 \
            .code = (_code),                  \
            .msg = (_msg),                    \
            .sys_errno = (_errno)};           \
    } while (0)

#define BCOMP_RETURN_ERR_MSG(_code, _msg) BCOMP_RETURN_ERR(_code, _msg, 0)

// ----- COMPRESSION -----
typedef struct {
    uint8_t algo; // BCOMP_ALGO_*
    size_t block_size;
    /** Block size in bytes.
     *  You can use standard dimensions (BCOMP_BLOCK_SIZE_*)
     *  or provide a value manually  */
} bcomp_compression_config_t;

typedef struct {
    size_t original_size;
    size_t compressed_size;
    size_t bytes_saved;
    int blocks_processed;
} bcomp_compress_result_t;

bcomp_err_t bcomp_compress_stream(FILE *in, FILE *out, const bcomp_compression_config_t *config, bcomp_compress_result_t *res);

#endif