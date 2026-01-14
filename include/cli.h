#ifndef CLI_H
#define CLI_H

typedef struct {
    int argc;
    char **argv;
    int offset; // the starting index (each command receives complete argc and argv
                // but command's arguments start from argv[offset])
} cli_ctx_t;

#endif