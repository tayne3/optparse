#include <stdbool.h>
#include <stdlib.h>

#include "optparse/optparse.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#define OPTIDX_INPUT   4
#define OPTIDX_OUTPUT  4
#define OPTIDX_GENERAL 2

static const optparse_def_t defs[] = {
    /* ---- Input options ---- */
    [0] = {"input", 'i', OPTPARSE_REQUIRED, "read from FILE instead of stdin", "FILE"},
    [1] = {"format", 'f', OPTPARSE_REQUIRED, "input format: json, xml or csv", "FMT"},
    [2] = {"encoding", 'e', OPTPARSE_REQUIRED, "input character encoding (default UTF-8)", "ENC"},
    [3] = {"no-header", 'H', OPTPARSE_NONE, "skip the first header row", NULL},
    /* ---- Output options ---- */
    [4] = {"output", 'o', OPTPARSE_REQUIRED, "write to FILE instead of stdout", "FILE"},
    [5] = {"indent", 'n', OPTPARSE_OPTIONAL, "indentation width in spaces (default 2)", "N"},
    [6] = {"quiet", 'q', OPTPARSE_NONE, "suppress all non-error output", NULL},
    /* ---- General options ---- */
    [7] = {"version", 'v', OPTPARSE_NONE, "print version string and exit", NULL},
    [8] = {"help", 'h', OPTPARSE_NONE, "display this help message and exit", NULL},
    OPTPARSE_DEF_NULL,
};

/* Detect terminal width at runtime; returns fallback on failure. */
static int term_width(int fallback) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) { return (int)ws.ws_col; }
#endif
    return fallback;
}

static void print_help() {
    optparse_help_config_t cfg = OPTPARSE_HELP_CONFIG_INIT;
    cfg.width                  = term_width(80);

    optparse_usage(stderr, "opt_usage", defs, -1, "SOURCE [DEST]");

    fprintf(stderr, "\nConvert SOURCE from one structured text format to another.\n"
                    "When DEST is omitted, output is written to stdout.\n");

    fprintf(stderr, "\nInput options:\n");
    optparse_help(stderr, &defs[0], OPTIDX_INPUT, &cfg);

    fprintf(stderr, "\nOutput options:\n");
    optparse_help(stderr, &defs[OPTIDX_INPUT], OPTIDX_OUTPUT, &cfg);

    fprintf(stderr, "\nGeneral options:\n");
    optparse_help(stderr, &defs[OPTIDX_INPUT + OPTIDX_OUTPUT], OPTIDX_GENERAL, &cfg);

    fprintf(stderr, "\nExamples:\n"
                    "  opt_usage -f json -o out.csv data.json\n"
                    "  opt_usage --format=xml --indent=4 input.xml result.xml\n");
}

int main(int argc, char** argv) {
    (void)argc;

    const char* input    = NULL;
    const char* output   = NULL;
    const char* format   = "json";
    const char* encoding = "UTF-8";
    int         indent   = 2;
    bool        no_hdr   = false;
    bool        quiet    = false;

    optparse_t opt;
    optparse_init(&opt, argv);

    optparse_error_t err;
    int              id;
    while ((err = optparse_next(&opt, defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'i': input = opt.optarg; break;
            case 'f': format = opt.optarg; break;
            case 'e': encoding = opt.optarg; break;
            case 'H': no_hdr = true; break;
            case 'o': output = opt.optarg; break;
            case 'n': indent = opt.optarg ? atoi(opt.optarg) : 2; break;
            case 'q': quiet = true; break;
            case 'v': printf("opt_usage version 1.0\n"); return EXIT_SUCCESS;
            case 'h': print_help(); return EXIT_SUCCESS;
        }
    }

    if (err != OPTPARSE_ERROR_DONE) {
        const char* err_msg = optparse_strerror(err);
        if (opt.optopt) {
            fprintf(stderr, "opt_usage: %s: -%c\n", err_msg, opt.optopt);
        } else {
            fprintf(stderr, "opt_usage: %s: %s\n", err_msg, opt.argv[opt.optind - 1]);
        }
        return EXIT_FAILURE;
    }

    printf("Configuration:\n");
    printf("  input:    %s\n", input ? input : "(stdin)");
    printf("  output:   %s\n", output ? output : "(stdout)");
    printf("  format:   %s\n", format);
    printf("  encoding: %s\n", encoding);
    printf("  indent:   %d\n", indent);
    printf("  no-header:%s\n", no_hdr ? "yes" : "no");
    printf("  quiet:    %s\n", quiet ? "yes" : "no");

    printf("\nRemaining arguments:\n");
    char* arg;
    while ((arg = optparse_arg(&opt))) { printf("  %s\n", arg); }

    return EXIT_SUCCESS;
}
