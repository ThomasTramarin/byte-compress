#include "errors.h"
#include <stdio.h>
#include <string.h>

const char *cli_err_code_to_string(cli_err_code_t code) {
    switch (code) {
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
    default:
        return "unknown error";
    }
}

/**
 * Function to print a good cli error message
 */
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
    fprintf(stderr, "\nRun '%s --help' for more information.\n", ctx->argv[0]);
}