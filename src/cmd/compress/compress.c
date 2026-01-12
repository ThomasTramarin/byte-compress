#include "parser.h"
#include <stdio.h>

static char *input_file = NULL;
static char *output_file = NULL;

static cli_opt_t compress_opt[] = {};

void compress_cmd_help() {
    printf("usage: bcomp compress <algorithm> [options]\n");
}

int compress_run(int argc, char **argv) {
    compress_cmd_help();
    return 0;
}

cli_cmd_t compress_cmd = {
    .name = "compress",
    .options = compress_opt,
    .option_count = 0,
    .run = compress_run,
    .help = compress_cmd_help,
};