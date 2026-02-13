#include "parser.h"
#include "version.h"
#include <stdio.h>

extern cli_cmd_t compress_cmd;
extern cli_cmd_t help_cmd;

static int display_version;

static cli_opt_t bcomp_cmd_options[] = {
    {
        .long_name = "version",
        .short_name = 'v',
        .type = CLI_ARG_TYPE_BOOL,
        .description = "Display Bcomp versions",
        .value = &display_version,
    },
};

static cli_cmd_t *bcomp_cmd_subcommands[] = {
    &compress_cmd,
    &help_cmd,
};

int bcomp_cmd_run(cli_ctx_t *ctx) {
    if (display_version) {
        printf("bcomp-cli %d.%d.%d (core: %d.%d.%d, format: %d.%d)\n",
               BCOMP_VER_CLI_MAJOR, BCOMP_VER_CLI_MINOR, BCOMP_VER_CLI_PATCH,
               BCOMP_VER_CORE_MAJOR, BCOMP_VER_CORE_MINOR, BCOMP_VER_CORE_PATCH,
               BCOMP_VER_FORMAT_MAJOR, BCOMP_VER_FORMAT_MINOR);

        return 0;
    }

    fprintf(stderr, "bcomp: missing command\nTry 'bcomp help' for more information.\n");

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