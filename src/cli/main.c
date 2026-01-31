#include "cli.h"
#include "errors.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

int main(int argc, char **argv) {
    cli_ctx_t ctx = {
        .argc = argc,
        .argv = argv,
        .offset = 1,
        .root_cmd = &bcomp_cmd,
    };

    cli_cmd_t *cmd;

    cli_err_t err = resolve_command(&bcomp_cmd, &ctx, &cmd);

    if (err.code != CLI_OK) {
        cli_print_error(&err, &ctx);
        return 1;
    }

    return cmd->run(&ctx);
}