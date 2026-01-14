#include "parser.h"
#include "cli.h"
#include "errors.h"
#include <string.h>

cli_cmd_t *resolve_command(cli_cmd_t *cmd, cli_ctx_t *ctx) {
    cli_cmd_t *current = cmd;

    while (ctx->offset < ctx->argc) {
        char *arg = ctx->argv[ctx->offset];

        if (arg[0] == '-')
            break;

        if (current->subcommands_count == 0)
            break;

        int found = 0;

        for (int i = 0; i < current->subcommands_count; i++) {
            cli_cmd_t *sub = current->subcommands[i];

            if (strcmp(arg, sub->name) == 0) {
                current = sub;
                ctx->offset++;
                found = 1;
                break;
            }
        }

        if (!found)
            break;
    }

    return current;
}

static cli_err_t parse_long_option(cli_cmd_t *cmd, cli_ctx_t *ctx, int *i) {
    char *raw_arg = ctx->argv[*i];
    char *arg = raw_arg + 2; // skip --

    // if the '=' symbol is found, argument name and value are in the same argv[i]
    char *eq = strchr(arg, '=');
    size_t name_len = eq ? (size_t)(eq - arg) : strlen(arg);

    // search for the long option
    for (int j = 0; j < cmd->option_count; j++) {
        cli_opt_t *opt = &cmd->options[j];

        if (strlen(opt->long_name) == name_len && strncmp(opt->long_name, arg, name_len) == 0) {
            // option match
            if (opt->type == CLI_ARG_TYPE_BOOL) {
                *(int *)opt->value = 1;
            } else if (opt->type == CLI_ARG_TYPE_STRING) {
                char *val = NULL;

                if (eq) {
                    val = eq + 1; // after '='
                    if (!*val) {
                        // empty string
                        return (cli_err_t){.code = CLI_ERR_MISSING_OPTION_VALUE, .arg = opt->long_name, .argv_idx = *i};
                    }
                } else if (*i + 1 < ctx->argc) {
                    val = ctx->argv[++(*i)]; // next argument
                } else {
                    // missing value
                    return (cli_err_t){
                        .code = CLI_ERR_MISSING_OPTION_VALUE,
                        .arg = opt->long_name,
                        .argv_idx = *i,
                    };
                }

                *(char **)opt->value = val;
            }

            // mark the option as seen (used later to verify the required constraint)
            opt->flags |= CLI_ARG_FLAG_SEEN;

            return (cli_err_t){.code = CLI_OK};
        }
    }

    // option not found
    return (cli_err_t){.code = CLI_ERR_UNKNOWN_OPTION, .arg = raw_arg, .argv_idx = *i};
}

static cli_err_t parse_short_option(cli_cmd_t *cmd, cli_ctx_t *ctx, int *i) {
    char *arg = ctx->argv[*i] + 1; // skip -
    int arg_len = strlen(arg);

    // iterate over each argument character
    for (int j = 0; j < arg_len; j++) {
        char short_name = arg[j];
        cli_opt_t *opt = NULL;

        // search for the matching short option
        for (int k = 0; k < cmd->option_count; k++) {
            if (cmd->options[k].short_name == short_name) {
                opt = &cmd->options[k];
                break;
            }
        }

        // option not found
        if (!opt) {
            return (cli_err_t){
                .code = CLI_ERR_UNKNOWN_OPTION,
                .arg = ctx->argv[*i],
                .argv_idx = *i,
            };
        }

        if (opt->type == CLI_ARG_TYPE_BOOL) {
            *(int *)opt->value = 1;
        } else if (opt->type == CLI_ARG_TYPE_STRING) {
            char *val = NULL;

            // case 1: value is attached (-ifile)
            if (j + 1 < arg_len) {
                val = &arg[j + 1]; // the value is the string starting from the character after the option name

                j = arg_len; // stop the loop
            }
            // case 2: value is separated (-i file)
            // check if there is at least anhother argument
            else if (*i + 1 < ctx->argc) {
                val = ctx->argv[++(*i)]; // increment to consume next string
            } else {
                // value not provided
                return (cli_err_t){
                    .code = CLI_ERR_MISSING_OPTION_VALUE,
                    .arg = opt->long_name,
                    .argv_idx = *i,
                };
            }

            *(char **)opt->value = val;
        }

        opt->flags |= CLI_ARG_FLAG_SEEN;
    }

    return (cli_err_t){.code = CLI_OK};
}

static cli_err_t parse_positional(cli_cmd_t *cmd, const char *arg, int *positional_idx) {
    if (*positional_idx >= cmd->positional_count) {
        return (cli_err_t){
            .code = CLI_ERR_TOO_MANY_POSITIONALS,
            .arg = arg,
        };
    }

    cli_pos_t *pos = &cmd->positionals[*positional_idx];

    if (pos->type == CLI_ARG_TYPE_STRING) {
        *(char **)pos->value = (char *)arg;
    }

    pos->flags |= CLI_ARG_FLAG_SEEN;

    (*positional_idx)++;

    return (cli_err_t){.code = CLI_OK};
}

/**
 * Parse command arguments.
 *
 */
cli_err_t parse_arguments(cli_cmd_t *cmd, cli_ctx_t *ctx) {
    int end_of_options = 0;
    int positional_idx = 0;

    for (int i = ctx->offset; i < ctx->argc; i++) {
        char *arg = ctx->argv[i];

        // OPTIONS
        if (!end_of_options && arg[0] == '-') {
            // every argument after -- is threated as a positional argument even if it stats with -
            if (strcmp(arg, "--") == 0) {
                end_of_options = 1;
                continue;
            }

            // parse option
            cli_err_t err;
            if (arg[1] == '-') {
                err = parse_long_option(cmd, ctx, &i);
            } else {
                err = parse_short_option(cmd, ctx, &i);
            }

            if (err.code != CLI_OK)
                return err;
            continue;
        }

        // POSITIONALS
        cli_err_t err = parse_positional(cmd, arg, &positional_idx);
        if (err.code != CLI_OK)
            return err;
    }

    // check required options
    for (int i = 0; i < cmd->option_count; i++) {
        cli_opt_t *opt = &cmd->options[i];
        // if the argument is marked as required but the user is not using it
        if ((opt->flags & CLI_ARG_FLAG_REQUIRED) && !(opt->flags & CLI_ARG_FLAG_SEEN)) {

            return (cli_err_t){
                .code = CLI_ERR_MISSING_REQUIRED_OPTION,
                .arg = opt->long_name,
            };
        }
    }

    // check required positionals
    for (int i = 0; i < cmd->positional_count; i++) {
        cli_pos_t *pos = &cmd->positionals[i];

        if ((pos->flags & CLI_ARG_FLAG_REQUIRED) && !(pos->flags & CLI_ARG_FLAG_SEEN)) {
            return (cli_err_t){
                .code = CLI_ERR_MISSING_POSITIONAL,
                .arg = pos->name,
            };
        }
    }

    return (cli_err_t){.code = CLI_OK};
}