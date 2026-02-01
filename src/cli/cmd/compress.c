#include "cli_helpers.h"
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
static int force = 0;

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
    {
        .long_name = "force",
        .short_name = 'f',
        .type = CLI_ARG_TYPE_BOOL,
        .value = &force,
        .description = "Overwrite output file if it already exists",
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
    const char *final_output_path = NULL;
    // run_err_t r_err = {.code = RUN_OK, .msg = NULL, .sys_errno = 0};

    // open input
    if (input != NULL) {
        ip = fopen(input, "rb");
        if (ip == NULL) {
            //     r_err.code = RUN_ERR_IO;
            //     r_err.msg = "failed to open input file";
            //     r_err.sys_errno = errno;
            //     run_print_error(&r_err);
            return 1;
        }
    } else {
        ip = stdin;
    }

    // determine output path
    if (to_stdout) {
        final_output_path = NULL; // stdout
    } else if (output != NULL) {
        final_output_path = output;
    } else if (input != NULL) {
        snprintf(auto_output, sizeof(auto_output), "%s.bcomp", input);
        final_output_path = auto_output;
    } else {
        final_output_path = NULL; // stdout by default if input is stdin
    }

    // check if the output file already exists
    if (final_output_path != NULL && !force && file_exists(final_output_path)) {
        // r_err.code = RUN_ERR_IO;
        // r_err.msg = "output file already exists (use --force to overwrite)";
        // r_err.sys_errno = EEXIST;
        // run_print_error(&r_err);
        // if (ip != stdin)
        //     fclose(ip);
        return 1;
    }

    // open output
    if (final_output_path == NULL) {
        op = stdout;
    } else {
        op = fopen(final_output_path, "wb");
        if (op == NULL) {
            // r_err.code = RUN_ERR_IO;
            // r_err.msg = "failed to open output file";
            // r_err.sys_errno = errno;
            // run_print_error(&r_err);
            // if (ip != stdin)
            //     fclose(ip);
            return 1;
        }
    }

    // determine the algorithm
    uint8_t algo_id;
    if (strcmp(algorithm, "rle") == 0) {
        // algo_id = BCF_ALGO_RLE;
    }

    // r_err = compress_engine(ip, op, algo_id);

    // if (r_err.code != RUN_OK) {
    // run_print_error(&r_err);
    return 1;
    // }

    // close files (only if they are not stdin/stdout)
    // if (ip && ip != stdin)
    //     fclose(ip);
    // if (op && op != stdout)
    //     fclose(op);

    return 0;
}
