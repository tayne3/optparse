#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define OPTPARSE_API static
#define OPTPARSE_IMPLEMENTATION
#include "optparse/optparse.h"

enum {
    OPT_HELP = 128,
    OPT_VERSION,
};

static void print_help(const char* prog_name) {
    fprintf(stderr, "Usage: %s [options] [args...]\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a, --amend         amend the previous commit\n");
    fprintf(stderr, "  -b, --brief         output brief description\n");
    fprintf(stderr, "  -c, --color=<val>   use colored output\n");
    fprintf(stderr, "  -d, --delay[=val]   delay with optional value\n");
    fprintf(stderr, "  -v, --verbose       enable verbose output\n");
    fprintf(stderr, "      --version       display version information and exit\n");
    fprintf(stderr, "      --help          display this help message and exit\n");
}

int main(int argc, char** argv) {
    (void)argc;
    struct optparse_long longopts[] = {
        {"amend", 'a', OPTPARSE_NONE},     {"brief", 'b', OPTPARSE_NONE},
        {"color", 'c', OPTPARSE_REQUIRED}, {"delay", 'd', OPTPARSE_OPTIONAL},
        {"verbose", 'v', OPTPARSE_NONE},   {"version", OPT_VERSION, OPTPARSE_NONE},
        {"help", OPT_HELP, OPTPARSE_NONE}, {0},
    };

    bool        amend   = false;
    bool        brief   = false;
    bool        verbose = false;
    const char* color   = "white";
    int         delay   = 0;

    char*           arg;
    int             option;
    struct optparse options;

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
            case 'a': amend = true; break;
            case 'b': brief = true; break;
            case 'c': color = options.optarg; break;
            case 'd': delay = options.optarg ? atoi(options.optarg) : 1; break;
            case 'v': verbose = true; break;
            case OPT_VERSION:
                printf("long example version 1.0\n");
                exit(EXIT_SUCCESS);
                break;
            case OPT_HELP:
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case '?': fprintf(stderr, "%s: %s\n", argv[0], options.errmsg); exit(EXIT_FAILURE);
        }
    }

    printf("Final configuration:\n");
    printf("  amend: %s\n", amend ? "true" : "false");
    printf("  brief: %s\n", brief ? "true" : "false");
    printf("  verbose: %s\n", verbose ? "true" : "false");
    printf("  color: %s\n", color);
    printf("  delay: %d\n", delay);

    printf("\nRemaining arguments:\n");
    while ((arg = optparse_arg(&options))) { printf("  %s\n", arg); }

    return 0;
}
