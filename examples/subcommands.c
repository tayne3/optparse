#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#endif

#include "optparse/optparse.h"

static void cmd_msleep(uint32_t ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec req;
    req.tv_sec  = (time_t)(ms / 1000U);
    req.tv_nsec = (long)((ms % 1000U) * 1000000UL);
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {}
#endif
}

static int cmd_echo(char** argv) {
    static const optparse_def_t defs[] = {
        {NULL, 'h', OPTPARSE_NONE, NULL, NULL},
        {NULL, 'n', OPTPARSE_NONE, NULL, NULL},
        OPTPARSE_DEF_NULL,
    };

    int              i, id;
    bool             newline = true;
    optparse_t       options;
    optparse_error_t status;

    optparse_init(&options, argv);
    options.permute = 0;
    while ((status = optparse_next(&options, defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'h': puts("usage: echo [-hn] [ARG]..."); return 0;
            case 'n': newline = false; break;
        }
    }

    if (status != OPTPARSE_ERROR_DONE) {
        fprintf(stderr, "%s: %s\n", argv[0], optparse_strerror(status));
        return 1;
    }

    argv += options.optind;

    for (i = 0; argv[i]; ++i) { printf("%s%s", i ? " " : "", argv[i]); }
    if (newline) { putchar('\n'); }

    fflush(stdout);
    return !!ferror(stdout);
}

static int cmd_sleep(char** argv) {
    static const optparse_def_t defs[] = {
        {NULL, 'h', OPTPARSE_NONE, NULL, NULL},
        OPTPARSE_DEF_NULL,
    };

    int              i, id;
    optparse_t       options;
    optparse_error_t status;

    optparse_init(&options, argv);
    while ((status = optparse_next(&options, defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'h': puts("usage: sleep [-h] [NUMBER]..."); return 0;
        }
    }

    if (status != OPTPARSE_ERROR_DONE) {
        fprintf(stderr, "%s: %s\n", argv[0], optparse_strerror(status));
        return 1;
    }

    for (i = 0; argv[i]; ++i) {
        const int seconds = atoi(argv[i]);
        if (seconds > 0) { cmd_msleep(seconds * 1000); }
    }
    return 0;
}

static void usage(FILE* f) {
    fprintf(f, "usage: opt_subcommands [-h] <echo|sleep> [OPTION]...\n");
}

int main(int argc, char** argv) {
    (void)argc;

    static const optparse_def_t global_defs[] = {
        {NULL, 'h', OPTPARSE_NONE, NULL, NULL},
        OPTPARSE_DEF_NULL,
    };

    int              i, id;
    char**           subargv;
    optparse_t       options;
    optparse_error_t status;

    static const struct {
        char name[8];
        int (*cmd)(char**);
    } cmds[] = {
        {"echo", cmd_echo},
        {"sleep", cmd_sleep},
    };
    int ncmds = sizeof(cmds) / sizeof(*cmds);

    optparse_init(&options, argv);
    options.permute = 0;

    while ((status = optparse_next(&options, global_defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'h': usage(stdout); return 0;
        }
    }

    if (status != OPTPARSE_ERROR_DONE) {
        usage(stderr);
        fprintf(stderr, "%s: %s\n", argv[0], optparse_strerror(status));
        return 1;
    }

    subargv = argv + options.optind;
    if (!subargv[0]) {
        fprintf(stderr, "%s: missing subcommand\n", argv[0]);
        usage(stderr);
        return 1;
    }

    for (i = 0; i < ncmds; ++i) {
        if (!strcmp(cmds[i].name, subargv[0])) { return cmds[i].cmd(subargv); }
    }
    fprintf(stderr, "%s: invalid subcommand: %s\n", argv[0], subargv[0]);
    return 1;
}
