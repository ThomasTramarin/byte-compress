#include "errors.h"
#include <stdio.h>
#include <string.h>

// CLI ERRORS
const char *cli_err_code_to_string(cli_err_code_t code) {
    switch (code) {
    case CLI_OK:
        return "success";
    case CLI_ERR_UNKNOWN_OPTION:
        return "unknown option";
    case CLI_ERR_MISSING_OPTION_VALUE:
        return "missing option value";
    case CLI_ERR_MISSING_REQUIRED_OPTION:
        return "missing required option";
    case CLI_ERR_TOO_MANY_POSITIONALS:
        return "too many positional arguments";
    case CLI_ERR_MISSING_POSITIONAL:
        return "missing positional argument";
    case CLI_ERR_UNKNOWN_COMMAND:
        return "unknown command";
    case CLI_ERR_INVALID_CHOICE:
        return "invalid choice";
    default:
        return "unknown cli error";
    }
}

void cli_print_error(const cli_err_t *err, const cli_ctx_t *ctx) {
    if (!err || err->code == CLI_OK)
        return;

    fprintf(stderr, "error: %s", cli_err_code_to_string(err->code));
    if (err->arg) {
        fprintf(stderr, " '%s'", err->arg);
    }
    fprintf(stderr, "\n\n");

    // print the command entered by the user
    for (int i = 0; i < ctx->argc; i++) {
        fprintf(stderr, " %s", ctx->argv[i]);
    }
    fprintf(stderr, "\n  ");

    // TODO: caret
    // TODO: expected message/did you mean

    // TODO: edit this message to support help with other subcommands commands
    fprintf(stderr, "\nRun '%s help' for more information.\n", ctx->argv[0]);
}

// RUNTIME ERRORS
const char *run_err_code_to_string(run_err_code_t code) {
    switch (code) {
    case RUN_OK:
        return "success";
    case RUN_ERR_IO:
        return "I/O error";
    case RUN_ERR_BUF_OVERFLOW:
        return "buffer overflow error";
    default:
        return "unknown runtime error";
    }
}
void run_print_error(const run_err_t *err) {
    if (!err || err->code == RUN_OK)
        return;

    fprintf(stderr, "runtime error: %s", run_err_code_to_string(err->code));

    if (err->msg && err->msg[0] != '\0') {
        fprintf(stderr, ": %s", err->msg);
    }

    if (err->sys_errno != 0) {
        fprintf(stderr, " (%s)", strerror(err->sys_errno));
    }

    fprintf(stderr, "\n");
}