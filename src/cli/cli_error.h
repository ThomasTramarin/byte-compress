#ifndef CLI_ERROR_H
#define CLI_ERROR_H

#include "cli.h"

/* Exit status CLI */
#define EXIT_OK 0
#define EXIT_CLI_ERROR 2
#define EXIT_IO_ERROR 3
#define EXIT_COMPRESS_DECOMPRESS_ERROR 4

/* Internal Errors */
typedef enum {
    CLI_OK = 0,

    CLI_ERR_UNKNOWN_OPTION,
    CLI_ERR_MISSING_OPTION_VALUE,
    CLI_ERR_MISSING_REQUIRED_OPTION,

    CLI_ERR_TOO_MANY_POSITIONALS,
    CLI_ERR_MISSING_POSITIONAL,

    CLI_ERR_INVALID_CHOICE,

    CLI_ERR_UNKNOWN_COMMAND,
} cli_err_code_t;

typedef struct {
    cli_err_code_t code;
    const char *arg;
    int argv_idx;
    const char *expected;
} cli_err_t;

const char *cli_err_code_to_string(cli_err_code_t code);
void cli_print_error(const cli_err_t *err, const cli_ctx_t *ctx);
#endif