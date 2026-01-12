#ifndef PARSER_H
#define PARSER_H

typedef enum {
    CLI_OPT_BOOL,
    CLI_OPT_STRING
} cli_opt_type_t;

typedef struct {
    const char *long_name;
    const char short_name;
    cli_opt_type_t type;
    int required; // 1 true, 0 false
    void *value;  // the value will be here
} cli_opt_t;

typedef struct cli_cmd_t cli_cmd_t;

struct cli_cmd_t {
    const char *name;
    cli_opt_t *options;
    int option_count;
    cli_cmd_t **subcommands;
    int subcommands_count;
    int (*run)(int argc, char **argv);
    void (*help)(void);
};

cli_cmd_t *resolve_command(cli_cmd_t *cmd, int argc, char **argv);

#endif