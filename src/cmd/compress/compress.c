#include "parser.h"
#include <stdio.h>

static cli_opt_t compress_opt[] = {};

static cli_pos_t compress_pos[] = {};

int compress_run(cli_ctx_t *ctx);

void compress_cmd_help() {
    printf("usage: bcomp compress <algorithm> [options]\n");
}

cli_cmd_t compress_cmd = {
    .name = "compress",
    .options = compress_opt,
    .option_count = sizeof(compress_opt) / sizeof(compress_opt[0]),
    .positionals = compress_pos,
    .positional_count = sizeof(compress_pos) / sizeof(compress_pos[0]),
    .run = compress_run,
    .help = compress_cmd_help,
};

int compress_run(cli_ctx_t *ctx) {
    cli_err_t err = parse_arguments(&compress_cmd, ctx);

    if (err.code != CLI_OK) {
        cli_print_error(&err, ctx);
        return 1;
    }

    printf("run\n");

    return 0;
}
