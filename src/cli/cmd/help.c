#include "../parser.h"
#include <stdio.h>
#include <string.h>

extern cli_cmd_t bcomp_cmd;

// OPTIONS
static cli_opt_t help_opt[] = {};

// NOTE: This positional exists only to define the "help" command;
//       it is not actually used. Multiple subcommands are already supported automatically.
static cli_pos_t help_pos[] = {
    {
        .name = "command",
        .description = "Command to show help for",
        .type = CLI_ARG_TYPE_STRING,
        .flags = 0,
    },
};

int help_run(cli_ctx_t *ctx);

cli_cmd_t help_cmd = {
    .name = "help",
    .options = help_opt,
    .option_count = ARR_SIZE(help_opt),
    .positionals = help_pos,
    .positional_count = ARR_SIZE(help_pos),
    .run = help_run,
    .parent = &bcomp_cmd,
    .description = "Display help information about commands.",
};

void print_help(cli_ctx_t *ctx, cli_cmd_t *cmd);

int help_run(cli_ctx_t *ctx) {
    cli_cmd_t *target = NULL;

    // print the root help
    if (ctx->argc == 2) {
        target = ctx->root_cmd;
    } else {
        cli_ctx_t subctx = *ctx;
        subctx.offset = 2; // skip "bcomp help"

        cli_err_t err = resolve_command(ctx->root_cmd, &subctx, &target);

        if (err.code != CLI_OK) {
            cli_print_error(&err, ctx);
            return 1;
        }
    }

    print_help(ctx, target);

    return 0;
}

/**
 * Helper to print text with wrap and indent
 */
void print_wrapped(const char *text, int col_width) {
    int screen_width = 90;
    int max_desc_len = screen_width - col_width;

    if (max_desc_len < 20)
        max_desc_len = 40;

    const char *ptr = text;
    int first_line = 1;

    while (*ptr) {
        if (!first_line) {
            printf("%*s", col_width, "");
        }

        int curr_len = strlen(ptr);
        int chunk_size = curr_len;

        // if the remaining part is longer than the available space on the line
        if (chunk_size > max_desc_len) {
            chunk_size = max_desc_len;

            // find the last space to avoid newline in the middle of a word
            while (ptr[chunk_size] != ' ' && ptr[chunk_size] != '\0')
                chunk_size--;
        }

        // print chunk_size characters
        printf("%.*s\n", chunk_size, ptr);

        ptr += chunk_size;

        // skip spaces at the beginning of the new line
        while (*ptr == ' ')
            ptr++;

        first_line = 0;
    }
}

/**
 * This function generates the help message based on command
 */
void print_help(cli_ctx_t *ctx, cli_cmd_t *cmd) {
    const int COL_WIDTH = 28;
    const int INDENT = 2;

    // --- USAGE ---
    cli_cmd_t *stack[16];
    int depth = 0;
    cli_cmd_t *curr = cmd;
    while (curr && curr != ctx->root_cmd) {
        stack[depth++] = curr;
        curr = curr->parent;
    }

    printf("USAGE:\n%*s%s", INDENT, "", ctx->argv[0]);
    // print commands
    for (int i = depth - 1; i >= 0; i--) {
        printf(" %s", stack[i]->name);
    }
    // print positionals
    for (int i = 0; i < cmd->positional_count; i++) {
        cli_pos_t pos = cmd->positionals[i];
        printf(pos.flags & CLI_ARG_FLAG_REQUIRED ? " <%s>" : " [%s]", pos.name);
    }
    // print options
    if (cmd->option_count > 0)
        printf(" [OPTIONS]");

    // --- DESCRIPTION ---
    printf("\n\nDESCRIPTION:\n");
    printf("%*s", INDENT, "");
    print_wrapped(cmd->description, INDENT);

    // --- POSITIONALS ---
    if (cmd->positional_count > 0) {
        printf("\n\nPOSITIONALS:\n");
        for (int i = 0; i < cmd->positional_count; i++) {
            cli_pos_t pos = cmd->positionals[i];

            // indent + name
            int n = printf("%*s%s", INDENT, "", pos.name);

            // dynamic padding
            if (n < COL_WIDTH)
                printf("%*s", COL_WIDTH - n, "");
            else
                printf("\n%*s", COL_WIDTH, "");

            char full_desc[1024];
            int offset = snprintf(full_desc, sizeof(full_desc), "%s", pos.description);

            // if there are choices, print them inside '{}'
            if (pos.choices_count > 0) {
                offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, " {");
                for (int j = 0; j < pos.choices_count; j++) {
                    offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, "%s%s", pos.choices[j], (j < pos.choices_count - 1) ? "|" : "");
                }
                offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, "}");
            }
            // if required
            if (pos.flags & CLI_ARG_FLAG_REQUIRED)
                snprintf(full_desc + offset, sizeof(full_desc) - offset, " [required]");
            print_wrapped(full_desc, COL_WIDTH);
        }
    }

    // --- OPTIONS ---
    if (cmd->option_count > 0) {
        printf("\nOPTIONS:\n");
        for (int i = 0; i < cmd->option_count; i++) {
            cli_opt_t opt = cmd->options[i];
            char buf[128];
            int n = 0;

            n += snprintf(buf + n, sizeof(buf) - n, "%*s", INDENT, "");
            if (opt.short_name)
                n += snprintf(buf + n, sizeof(buf) - n, "-%c%s", opt.short_name, opt.long_name ? ", " : "");
            if (opt.long_name)
                n += snprintf(buf + n, sizeof(buf) - n, "--%s", opt.long_name);
            if (opt.type != CLI_ARG_TYPE_BOOL)
                n += snprintf(buf + n, sizeof(buf) - n, " <%s>", opt.long_name ? opt.long_name : "value");

            printf("%s", buf);

            // dynamic padding
            if (n < COL_WIDTH)
                printf("%*s", COL_WIDTH - n, "");
            else
                printf("\n%*s", COL_WIDTH, "");

            char full_desc[1024];
            int offset = snprintf(full_desc, sizeof(full_desc), "%s", opt.description);

            // if there are choices, print them inside '{}'
            if (opt.choices_count > 0) {
                offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, " {");
                for (int j = 0; j < opt.choices_count; j++) {
                    offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, "%s%s",
                                       opt.choices[j], (j < opt.choices_count - 1) ? "|" : "");
                }
                offset += snprintf(full_desc + offset, sizeof(full_desc) - offset, "}");
            }
            // if required
            if (opt.flags & CLI_ARG_FLAG_REQUIRED) {
                snprintf(full_desc + offset, sizeof(full_desc) - offset, " [required]");
            }

            print_wrapped(full_desc, COL_WIDTH);
        }
    }

    // --- SUBCOMMANDS ---
    if (cmd->subcommands_count > 0) {
        printf("\nSUBCOMMANDS:\n");
        for (int i = 0; i < cmd->subcommands_count; i++) {
            cli_cmd_t *sub = cmd->subcommands[i];

            int n = printf("%*s%s", INDENT, "", sub->name);

            // dynamic padding
            if (n < COL_WIDTH)
                printf("%*s", COL_WIDTH - n, "");
            else
                printf("\n%*s", COL_WIDTH, "");

            // description
            print_wrapped(sub->description, COL_WIDTH);
        }
    }

    printf("\n");
}