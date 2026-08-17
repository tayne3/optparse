#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "optparse/optparse.h"

enum {
    OPT_DELAY = 128,
    OPT_VERBOSE,
};

static const optparse_def_t defs[] = {
    {"amend", 'a', OPTPARSE_NONE, "amend the previous commit", NULL},
    {"brief", 'b', OPTPARSE_NONE, "output brief description", NULL},
    {"color", 'c', OPTPARSE_REQUIRED, "use colored output", "COLOR"},
    {"delay", OPT_DELAY, OPTPARSE_OPTIONAL, "delay with optional value", "MS"},
    {"verbose", OPT_VERBOSE, OPTPARSE_NONE, "enable verbose output", NULL},
    {"version", 'v', OPTPARSE_NONE, "display version information and exit", NULL},
    {"help", 'h', OPTPARSE_NONE, "display this help message and exit", NULL},
    OPTPARSE_DEF_NULL,
};

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "missing parameter\n");
        optparse_usage(stderr, argv[0], defs, -1, NULL);
        return 1;
    }

    bool        amend   = false;
    bool        brief   = false;
    bool        verbose = false;
    const char* color   = "white";
    int         delay   = 0;

    optparse_t opt;
    optparse_init(&opt, argv);

    optparse_error_t err;
    int              id;
    while ((err = optparse_next(&opt, defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'a': amend = true; break;
            case 'b': brief = true; break;
            case 'c': color = opt.optarg; break;
            case OPT_DELAY: delay = opt.optarg ? atoi(opt.optarg) : 1; break;
            case OPT_VERBOSE: verbose = true; break;
            case 'v': printf("opt_basic version 1.0\n"); exit(EXIT_SUCCESS);
            case 'h':
                optparse_usage(stderr, "opt_basic", defs, -1, NULL);
                fprintf(stderr, "\nOptions:\n");
                optparse_help(stderr, defs, -1, NULL);
                fprintf(stderr, "\n");
                exit(EXIT_SUCCESS);
        }
    }
    if (err != OPTPARSE_ERROR_DONE) {
        if (opt.optopt > 0 && opt.optopt < 128) {
            fprintf(stderr, "opt_basic: %s: -%c\n", optparse_strerror(err), opt.optopt);
        } else {
            fprintf(stderr, "opt_basic: %s: %s\n", optparse_strerror(err), opt.argv[opt.optind - 1]);
        }
        exit(EXIT_FAILURE);
    }

    printf("Final configuration:\n");
    printf("  amend: %s\n", amend ? "true" : "false");
    printf("  brief: %s\n", brief ? "true" : "false");
    printf("  verbose: %s\n", verbose ? "true" : "false");
    printf("  color: %s\n", color);
    printf("  delay: %d\n", delay);

    printf("\nRemaining arguments:\n");
    char* arg;
    while ((arg = optparse_arg(&opt))) { printf("  %s\n", arg); }

    return 0;
}
