#ifndef CLI_H
#define CLI_H

typedef enum {
    ALGO_RLE,
} compress_algo_t;

typedef enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS,
} compress_mode_t;

typedef struct {
    compress_algo_t algorithm;
    compress_mode_t mode;
    const char *input_path;
    const char *output_path;
} cli_args_t;

void print_help(char *name);
cli_args_t parse_cli(int argc, char **argv);
int run_cli(int argc, char **argv);

#endif