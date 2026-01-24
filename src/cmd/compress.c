#include "bcff.h"
#include "bcomp_core.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

// ARGUMENTS
static char *algorithm = NULL;
static char *input = NULL;
static char *output = NULL;
// this flag overrides 'output', meaning that if the user writes -c,
// the output is always printed to stdout
static int to_stdout = 0;

// OPTIONS
static cli_opt_t compress_opt[] = {
    {
        .long_name = "input",
        .short_name = 'i',
        .type = CLI_ARG_TYPE_STRING,
        .value = &input,
        .description = "Path to the input file (stdin if not specified).",
    },
    {
        .long_name = "output",
        .short_name = 'o',
        .type = CLI_ARG_TYPE_STRING,
        .value = &output,
        .description = "Path to the output file (<input>.bcomp if not specified).",
    },
    {
        .long_name = "stdout",
        .short_name = 'c',
        .type = CLI_ARG_TYPE_BOOL,
        .value = &to_stdout,
        .description = "Write output to stdout instead of a file. Overrides the --output flag.",
    },
};

// POSITIONALS
static const char *compress_pos_choices[] = {"rle"};

static cli_pos_t compress_pos[] = {
    {
        .name = "algorithm",
        .type = CLI_ARG_TYPE_STRING,
        .value = &algorithm,
        .flags = CLI_ARG_FLAG_REQUIRED,
        .choices = compress_pos_choices,
        .choices_count = ARR_SIZE(compress_pos_choices),
        .description = "Compression algorithm to use.",
    },
};

int compress_run(cli_ctx_t *ctx);

cli_cmd_t compress_cmd = {
    .name = "compress",
    .options = compress_opt,
    .option_count = ARR_SIZE(compress_opt),
    .positionals = compress_pos,
    .positional_count = ARR_SIZE(compress_pos),
    .run = compress_run,
    .description = "Compress data using the specified algorithm. "
                   "Supports streaming from stdin and automatic output naming. "
                   "Use '-c' to pipe the result directly to other tools.",
    .parent = &bcomp_cmd,
};

int compress_run(cli_ctx_t *ctx) {
    cli_err_t err = parse_arguments(&compress_cmd, ctx);

    if (err.code != CLI_OK) {
        cli_print_error(&err, ctx);
        return 1;
    }

    FILE *ip = NULL;
    FILE *op = NULL;
    char auto_output[256];

    // open input
    if (input != NULL) {
        ip = fopen(input, "rb");
        if (ip == NULL) {
            perror("error opening input file");
            return 1;
        }
    } else {
        ip = stdin;
    }

    // open output
    if (to_stdout) {
        op = stdout;
    } else if (output != NULL) {
        op = fopen(output, "wb");
    } else if (input != NULL) {
        snprintf(auto_output, sizeof(auto_output), "%s.bcomp", input);
        op = fopen(auto_output, "wb");
    } else {
        op = stdout;
    }

    // output error handling
    if (op == NULL) {
        perror("error opening output file");
        if (ip != NULL)
            fclose(ip);
        return 1;
    }

    // determine the algorithm
    uint8_t algo_id;
    if (strcmp(algorithm, "rle") == 0) {
        algo_id = BCFF_ALGO_RLE;
    }

    run_err_t res = {0};

    res = compress_engine(ip, op, algo_id);

    if (res.code != RUN_OK) {
        run_print_error(&res);
        return 1;
    }

    // close files (only if they are not stdin/stdout)
    if (ip && ip != stdin)
        fclose(ip);
    if (op && op != stdout)
        fclose(op);

    if (res.code != RUN_OK) {
        run_print_error(&res);
        return 1;
    }

    return 0;
}
