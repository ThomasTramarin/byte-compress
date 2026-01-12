#include "parser.h"
#include <string.h>

cli_cmd_t *resolve_command(cli_cmd_t *cmd, int argc, char **argv) {
    if (argc < 1)
        return cmd;

    for (int i = 0; i < cmd->subcommands_count; i++) {
        if (strcmp(argv[0], cmd->subcommands[i]->name) == 0) {
            return resolve_command(cmd->subcommands[i], argc - 1, argv + 1);
        }
    }

    return cmd;
}