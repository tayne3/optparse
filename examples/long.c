#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define OPTPARSE_API static
#define OPTPARSE_IMPLEMENTATION
#include "optparse/optparse.h"

enum {
    OPT_DELAY = 128,
    OPT_VERBOSE,
};

int main(int argc, char** argv) {
    (void)argc;
    optparse_long_t longopts[] = {
        {"amend", 'a', OPTPARSE_NONE, "amend the previous commit", NULL},
        {"brief", 'b', OPTPARSE_NONE, "output brief description", NULL},
        {"color", 'c', OPTPARSE_REQUIRED, "use colored output", "COLOR"},
        {"delay", OPT_DELAY, OPTPARSE_OPTIONAL, "delay with optional value", "MS"},
        {"verbose", OPT_VERBOSE, OPTPARSE_NONE, "enable verbose output", NULL},
        {"version", 'v', OPTPARSE_NONE, "display version information and exit", NULL},
        {"help", 'h', OPTPARSE_NONE, "display this help message and exit", NULL},
        {0},
    };

    bool        amend   = false;
    bool        brief   = false;
    bool        verbose = false;
    const char* color   = "white";
    int         delay   = 0;

    char*      arg;
    int        option;
    optparse_t options;

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
            case 'a': amend = true; break;
            case 'b': brief = true; break;
            case 'c': color = options.optarg; break;
            case OPT_DELAY: delay = options.optarg ? atoi(options.optarg) : 1; break;
            case OPT_VERBOSE: verbose = true; break;
            case 'v':
                printf("long version 1.0\n");
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                optparse_usage(stderr, "long", longopts, -1, "[args...]");
                fprintf(stderr, "\nOptions:\n");
                optparse_help(stderr, longopts, -1, NULL);
                fprintf(stderr, "\n");
                exit(EXIT_SUCCESS);
                break;
            case '?': fprintf(stderr, "long: %s\n", options.errmsg); exit(EXIT_FAILURE);
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
