#ifndef CLI_H
#define CLI_H

typedef struct cli_cmd_t cli_cmd_t;

typedef struct {
    int argc;
    char **argv;
    int offset; // the starting index (each command receives complete argc and argv
                // but command's arguments start from argv[offset])

    cli_cmd_t *root_cmd;
} cli_ctx_t;

#endif