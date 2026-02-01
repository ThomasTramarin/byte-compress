#include "errors.h"
#include <stdio.h>
#include <string.h>

// RUNTIME ERRORS
const char *run_err_code_to_string(run_err_code_t code) {
    switch (code) {
    case RUN_OK:
        return "success";
    case RUN_ERR_IO:
        return "I/O error";
    case RUN_ERR_BUF_OVERFLOW:
        return "buffer overflow error";
    default:
        return "unknown runtime error";
    }
}
void run_print_error(const run_err_t *err) {
    if (!err || err->code == RUN_OK)
        return;

    fprintf(stderr, "runtime error: %s", run_err_code_to_string(err->code));

    if (err->msg && err->msg[0] != '\0') {
        fprintf(stderr, ": %s", err->msg);
    }

    if (err->sys_errno != 0) {
        fprintf(stderr, " (%s)", strerror(err->sys_errno));
    }

    fprintf(stderr, "\n");
}