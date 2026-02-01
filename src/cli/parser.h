#ifndef PARSER_H
#define PARSER_H
#include "cli.h"
#include "cli_error.h"
#include "cli_helpers.h"
#include <stdint.h>

#define CLI_ARG_FLAG_REQUIRED 0x01
#define CLI_ARG_FLAG_SEEN 0x02

typedef enum {
    CLI_ARG_TYPE_BOOL,
    CLI_ARG_TYPE_STRING
} cli_arg_type_t;

typedef struct {
    const char *long_name;
    const char short_name;
    cli_arg_type_t type;
    uint8_t flags;
    void *value;

    const char **choices;
    int choices_count;

    const char *description;
} cli_opt_t;

typedef struct {
    const char *name;
    cli_arg_type_t type;
    uint8_t flags;
    void *value;

    const char **choices;
    int choices_count;

    const char *description;
} cli_pos_t;

typedef struct cli_cmd_t cli_cmd_t;

struct cli_cmd_t {
    const char *name;
    char *description;

    cli_opt_t *options;
    int option_count;

    cli_cmd_t **subcommands;
    int subcommands_count;

    cli_pos_t *positionals;
    int positional_count;

    int (*run)(cli_ctx_t *ctx);

    cli_cmd_t *parent;
};

cli_err_t resolve_command(cli_cmd_t *root, cli_ctx_t *ctx, cli_cmd_t **out_cmd);
cli_err_t parse_arguments(cli_cmd_t *cmd, cli_ctx_t *ctx);

#endif