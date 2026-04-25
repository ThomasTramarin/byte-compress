#include "cli_helpers.h"
#include <stdio.h>
#include <sys/stat.h>

int file_exists(const char *path) {
    if (path == NULL)
        return 0;
    return ACCESS(path, F_OK) == 0;
}

int file_is_directory(const char *path) {
    if (path == NULL)
        return 0;

    struct stat st;

#ifdef _WIN32
    if (_stat(path, &st) != 0) {
        return 0;
    }
#else
    if (stat(path, &st) != 0) {
        return 0;
    }
#endif

    return S_ISDIR(st.st_mode);
}