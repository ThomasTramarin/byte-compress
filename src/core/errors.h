#ifndef ERROR_H
#define ERROR_H
#include "cli.h"
#include <errno.h>

// CLI ERRORS
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

// RUNTIME ERRORS
typedef enum {
    RUN_OK = 0,
    RUN_ERR_IO,
    RUN_ERR_BUF_OVERFLOW,
} run_err_code_t;

typedef struct {
    run_err_code_t code;
    const char *msg;
    int sys_errno;
} run_err_t;

#endif

const char *run_err_code_to_string(run_err_code_t code);
void run_print_error(const run_err_t *err);