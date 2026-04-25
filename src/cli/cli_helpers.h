#ifndef CLI_HELPERS
#define CLI_HELPERS

#ifdef _WIN32
#include <io.h>
#ifndef F_OK
#define F_OK 0
#endif
#define ACCESS _access
#else
#include <unistd.h>
#define ACCESS access
#endif

#define ARR_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int file_exists(const char *path);
int file_is_directory(const char *path);

#endif