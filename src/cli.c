#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void cli_error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

/**
 * Prints the help message
 *
 * @param name The name of the program (argv[0])
 */
void print_help(char *name) {
    printf(
        "Usage: %s <algorithm> <mode> <input> <output> [options]\n\n"
        "Algorithms:\n"
        "   rle             Run-Length Encoding\n\n"
        "Mode:\n"
        "   compress, c     Compress the input file\n"
        "   decompress, d   Decompress the input file\n\n"
        "Options:\n"
        "   --help, -h      Show this help message \n",
        name);
}

/**
 * This function parse cli arguments and returns a struct representing them.
 * If the user enters an invalid argument, the function exits the program using exit()
 */
cli_args_t parse_cli(int argc, char **argv) {

    // check help flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            exit(0);
        }
    }

    int required_args = 4; // algo, mode, input, output

    if (argc - 1 < required_args) {
        cli_error("missing arguments. Use -h or --help for usage.");
    }

    if (argc - 1 > required_args) {
        cli_error("too many arguments. Use -h or --help for usage.");
    }

    cli_args_t args;

    // parse algorithm
    if (strcmp(argv[1], "rle") == 0) {
        args.algorithm = ALGO_RLE;
    } else {
        cli_error("algorithm is not valid. Use -h or --help for usage.");
    }

    // parse mode
    if (strcmp(argv[2], "c") == 0 || strcmp(argv[2], "compress") == 0) {
        args.mode = MODE_COMPRESS;
    } else if (strcmp(argv[2], "d") == 0 || strcmp(argv[2], "decompress") == 0) {
        args.mode = MODE_DECOMPRESS;
    } else {
        cli_error("mode is not valid. Use -h or --help for usage.");
    }

    // parse input file
    FILE *fi = fopen(argv[3], "rb"); // open the file, NULL if the file does not exists
    if (fi == NULL) {
        cli_error("input file does not exist.");
    }
    args.input_path = argv[3];
    fclose(fi);

    // check if output file exists
    FILE *fo = fopen(argv[4], "rb");
    if (fo) {
        fclose(fo);
        cli_error("output file already exists.");
    }

    // check if output file can be created
    fo = fopen(argv[4], "wb");
    if (!fo) {
        perror("Error: cannot create output file");
        exit(1);
    }
    fclose(fo);

    args.output_path = argv[4];

    return args;
}

int run_cli(int argc, char **argv) {
    cli_args_t args = parse_cli(argc, argv); // here args contains valid fields

    return 0; // success
}