#include "parser.h"
#include <stdio.h>

extern cli_cmd_t compress_cmd;

static cli_opt_t bcomp_cmd_options[] = {

};

static cli_cmd_t *bcomp_cmd_subcommands[] = {
    &compress_cmd,
};

void bcomp_cmd_help() {
    printf("usage: bcomp [options]\n");
}

int bcomp_cmd_run(int argc, char **argv) {
    bcomp_cmd_help();
    return 0;
}

cli_cmd_t bcomp_cmd = {
    .name = "bcomp",
    .options = bcomp_cmd_options,
    .option_count = sizeof(bcomp_cmd_options) / sizeof(bcomp_cmd_options[0]),
    .subcommands = bcomp_cmd_subcommands,
    .subcommands_count = sizeof(bcomp_cmd_subcommands) / sizeof(bcomp_cmd_subcommands[0]),
    .run = bcomp_cmd_run,
    .help = bcomp_cmd_help,
};