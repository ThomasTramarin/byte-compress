#ifndef ERROR_H
#define ERROR_H

#include <errno.h>

// RUNTIME ERRORS
typedef enum {
    RUN_OK = 0,
    RUN_ERR_IO,
    RUN_ERR_BUF_OVERFLOW,
} run_err_code_t;

typedef struct {
    run_err_code_t code;
    const char *msg;
    int sys_errno;
} run_err_t;

#endif

const char *run_err_code_to_string(run_err_code_t code);
void run_print_error(const run_err_t *err);