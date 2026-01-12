#include "cli.h"
#include "crc.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

int main(int argc, char **argv) {
    cli_cmd_t *cmd = resolve_command(&bcomp_cmd, argc - 1, argv + 1);
    init_crc_table();
    return cmd->run(argc - 1, argv + 1);
}