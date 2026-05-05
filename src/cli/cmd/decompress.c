#include "../cli_helpers.h"
#include "../parser.h"
#include "bcomp.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

// ARGUMENTS
static char *input = NULL;
static char *output = NULL;
// this flag overrides 'output', meaning that if the user writes -c,
// the output is always printed to stdout
static int to_stdout = 0;
static int force = 0;

// OPTIONS
static cli_opt_t decompress_opt[] = {
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

static cli_pos_t decompress_pos[] = {};

int decompress_run(cli_ctx_t *ctx);

cli_cmd_t decompress_cmd = {
    .name = "decompress",
    .options = decompress_opt,
    .option_count = ARR_SIZE(decompress_opt),
    .positionals = decompress_pos,
    .positional_count = ARR_SIZE(decompress_pos),
    .run = decompress_run,
    .description = "Decompress data",
    .parent = &bcomp_cmd,
};

int decompress_run(cli_ctx_t *ctx) {
    cli_err_t err = parse_arguments(&decompress_cmd, ctx);

    if (err.code != CLI_OK) {
        cli_print_error(&err, ctx);
        return EXIT_CLI_ERROR;
    }

    FILE *ip = NULL;
    FILE *op = NULL;
    int exit_code = EXIT_OK;
    char auto_output[256];
    const char *final_output_path = NULL;

    // the file must be a file
    if (input != NULL && file_is_directory(input)) {
        fprintf(stderr, "error: input is a directory (%s)\n", input);
        exit_code = EXIT_IO_ERROR;
        goto cleanup;
    }

    // open input
    if (input != NULL) {
        ip = fopen(input, "rb"); // read binary
        if (ip == NULL) {
            fprintf(stderr, "error: failed to open input file (%s)\n", input);
            return EXIT_IO_ERROR;
        }
    } else {
        ip = stdin;
    }

    if (to_stdout) {
        final_output_path = NULL;
    } else if (output != NULL) {
        final_output_path = output;
    } else if (input != NULL) {
        // copy str
        strncpy(auto_output, input, sizeof(auto_output));
        auto_output[sizeof(auto_output) - 1] = '\0';

        size_t len = strlen(auto_output);
        const char *suffix = ".bcomp";
        size_t s_len = strlen(suffix);

        if (len > s_len && strcmp(auto_output + len - s_len, suffix) == 0) {
            auto_output[len - s_len] = '\0'; // remove ".bcomp"
        } else {
            strncat(auto_output, ".out", sizeof(auto_output) - strlen(auto_output) - 1);
        }
        final_output_path = auto_output;

    } else {
        final_output_path = NULL;
    }

    if (final_output_path != NULL && !force && file_exists(final_output_path)) {
        fprintf(stderr, "error: output file (%s) already exists, use --force to overwrite\n", final_output_path);
        exit_code = EXIT_IO_ERROR;
        goto cleanup;
    }

    // open output
    if (final_output_path == NULL) {
        op = stdout;
    } else {
        op = fopen(final_output_path, "wb");
        if (op == NULL) {
            fprintf(stderr, "error: failed to open output file (%s)\n", final_output_path);
            exit_code = EXIT_IO_ERROR;
            goto cleanup;
        }
    }

    bcomp_err_t r_err = bcomp_decompress_stream(ip, op);

    if (r_err.code != BCOMP_OK) {
        fprintf(stderr, "error: decompression failed: %s\n",
                r_err.msg ? r_err.msg : "unknown error");
        exit_code = EXIT_COMPRESS_DECOMPRESS_ERROR;
        goto cleanup;
    }

    fprintf(stderr, "decompression successful\n");

cleanup:
    // close files (only if they are not stdin/stdout)
    if (ip && ip != stdin)
        fclose(ip);
    if (op && op != stdout)
        fclose(op);

    return exit_code;
}
