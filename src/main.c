#include "cli.h"
#include "crc.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    init_crc_table();
    return run_cli(argc, argv);
}