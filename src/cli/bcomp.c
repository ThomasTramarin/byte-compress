#include "parser.h"
#include <stdio.h>

extern cli_cmd_t compress_cmd;
extern cli_cmd_t help_cmd;

static cli_opt_t bcomp_cmd_options[] = {

};

static cli_cmd_t *bcomp_cmd_subcommands[] = {
    &compress_cmd,
    &help_cmd,
};

void bcomp_cmd_help() {
    printf("usage: bcomp <cmd> [options]\n");
}

int bcomp_cmd_run(cli_ctx_t *ctx) {
    bcomp_cmd_help();
    return 0;
}

cli_cmd_t bcomp_cmd = {
    .name = "bcomp",
    .options = bcomp_cmd_options,
    .option_count = sizeof(bcomp_cmd_options) / sizeof(bcomp_cmd_options[0]),
    .subcommands = bcomp_cmd_subcommands,
    .subcommands_count = sizeof(bcomp_cmd_subcommands) / sizeof(bcomp_cmd_subcommands[0]),
    .description = "bcomp is a command-line interface program for data compression.",
    .run = bcomp_cmd_run,
    .parent = NULL,
};