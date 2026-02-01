#include "cli_helpers.h"
#include <stdio.h>

int file_exists(const char *path) {
    if (path == NULL)
        return 0;
    return ACCESS(path, F_OK) == 0;
}