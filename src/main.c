#include "cli.h"
#include "crc.h"
#include "errors.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

int main(int argc, char **argv) {
    cli_ctx_t ctx = {
        .argc = argc,
        .argv = argv,
        .offset = 1,
    };

    init_crc_table();

    cli_cmd_t *cmd = resolve_command(&bcomp_cmd, &ctx);

    return cmd->run(&ctx);
}