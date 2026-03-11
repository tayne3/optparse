/**
 * @file optparse.h
 * @brief Portable, reentrant, embeddable command-line option parser.
 *
 * A C99 replacement for POSIX getopt() / GNU getopt_long() that fixes three
 * fundamental flaws of the standard:
 *
 *  1. All state is stored in a user-supplied struct — fully reentrant and
 *     thread-safe; nested sub-argument parsing is supported naturally.
 *
 *  2. The parser can be reset at any time via optparse_init(), and
 *     optparse_arg() allows stepping over sub-commands so that option parsing
 *     can continue with a fresh option string.
 *
 *  3. Error messages are stored inside the struct (errmsg field) rather than
 *     being printed to stderr.
 *
 * STB-style single-header library. In exactly ONE source file, define:
 *   #define OPTPARSE_IMPLEMENTATION
 * before including this header.
 *
 * This is free and unencumbered software released into the public domain.
 */
#ifndef OPTPARSE_OPTPARSE_H
#define OPTPARSE_OPTPARSE_H

#include <stddef.h>
#include <stdio.h>

#ifndef OPTPARSE_API
#define OPTPARSE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Core parser state.
 *
 * After each optparse() / optparse_long() call, caller may read:
 *   optind  – index of next argv element
 *   optopt  – the option character just parsed
 *   optarg  – argument for current option (may be NULL)
 *   errmsg  – error string (non-empty only when '?' returned)
 *
 * Caller may set before/between calls:
 *   permute – non-zero (default) to permute non-options to end;
 *             set 0 to stop at first non-option (POSIX mode)
 */
typedef struct optparse {
    char   errmsg[64];
    char*  optarg;
    char** argv;
    int    permute;
    int    optind;
    int    optopt;
    int    subopt; /* internal: offset within short-opt cluster */
} optparse_t;

typedef enum optparse_argtype {
    OPTPARSE_NONE     = 0,
    OPTPARSE_REQUIRED = 1,
    OPTPARSE_OPTIONAL = 2,
} optparse_argtype_t;

/**
 * @brief Long option descriptor.
 *
 * Terminate array with {0, 0, OPTPARSE_NONE, NULL}.
 * Set argdesc to NULL to hide an option from help output.
 */
typedef struct optparse_long {
    const char*        longname;
    int                shortname; /* corresponding short char, non-printable-ASCII for long-only */
    optparse_argtype_t argtype;
    const char*        argdesc;
    const char*        argname; /* placeholder name, default "ARG" */
} optparse_long_t;

/// @brief Initialize parser state; must be called before any parse call.
/// @param options parser state to initialize
/// @param argv    argument vector (typically from main()); argv[0] is skipped
OPTPARSE_API void optparse_init(optparse_t* options, char** argv);

/// @brief Parse next short option.
/// @param options   parser state
/// @param optstring getopt()-style option string: no colon = no argument, one colon = required, two colons = optional
/// @return option character, -1 when done, '?' on error
OPTPARSE_API int optparse(optparse_t* options, const char* optstring);

/// @brief Parse next option, supporting both short and GNU-style long options.
/// @param options   parser state
/// @param longopts  long option descriptor array, terminated with {0, 0, OPTPARSE_NONE, NULL}
/// @param longindex receives index into longopts, or -1 for short options
/// @return option character / shortname, -1 when done, '?' on error
OPTPARSE_API int optparse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex);

/// @brief Retrieve next non-option argument; useful for stepping over sub-commands.
/// @param options parser state
/// @return next non-option argument string, or NULL if none remain
OPTPARSE_API char* optparse_arg(optparse_t* options);

typedef struct optparse_help_config {
    int width;
    int min_desc;
    int max_left;
} optparse_help_config_t;

#define OPTPARSE_HELP_CONFIG_DEFAULT {80, 26, 36}

/// @brief Print usage line to out.
/// @param out destination.
/// @param progname name of the program.
/// @param longopts long option descriptor array (optional, used to detect if [options] should be shown).
/// @param pos_args positional arguments argdesc (optional, e.g., "SOURCE DEST").
OPTPARSE_API void optparse_usage(FILE* out, const char* progname, const optparse_long_t* longopts,
                                 const char* pos_args);

/// @brief Print formatted help to out.
/// @param out destination, typically stdout or stderr.
/// @param longopts same array passed to optparse_long().
OPTPARSE_API void optparse_help(FILE* out, const optparse_long_t* longopts, const optparse_help_config_t* cfg);

#ifdef __cplusplus
}
#endif

/* ======================================================================
 * IMPLEMENTATION
 * ====================================================================== */
#ifdef OPTPARSE_IMPLEMENTATION

#include <string.h>

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

static int optparse__set_error(optparse_t* options, const char* msg, const char* data) {
    unsigned int p   = 0;
    const char*  sep = " -- '";

    while (*msg && p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = *msg++; }
    while (*sep && p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = *sep++; }
    while (*data && p < sizeof(options->errmsg) - 2) { options->errmsg[p++] = *data++; }

    if (p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = '\''; }
    options->errmsg[p] = '\0';

    return '?';
}

static inline int optparse__is_dashdash(const char* arg) {
    return arg && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static inline int optparse__is_shortopt(const char* arg) {
    return arg && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static inline int optparse__is_longopt(const char* arg) {
    return arg && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static void optparse__shift_args(char** argv, int from, int to, int count) {
    for (int k = 0; k < count; ++k) {
        char* tmp = argv[from + k];
        for (int j = from + k; j > to + k; --j) { argv[j] = argv[j - 1]; }
        argv[to + k] = tmp;
    }
}

static int optparse__get_short_argtype(const char* optstring, char c) {
    if (c == ':') { return -1; }
    for (; *optstring && c != *optstring; ++optstring) {}
    if (optstring[0] == '\0') { return -1; }
    if (optstring[1] == ':') { return optstring[2] == ':' ? 2 : 1; }
    return 0;
}

static inline int optparse__longopts_end(const optparse_long_t* longopts, int i) {
    return !longopts[i].longname && !longopts[i].shortname;
}

static int optparse__longopts_match(const char* longname, const char* option) {
    const char *a = option, *n = longname;
    if (!longname) { return 0; }
    for (; *a && *n && *a != '='; ++a, ++n) {
        if (*a != *n) { return 0; }
    }
    return *n == '\0' && (*a == '\0' || *a == '=');
}

static char* optparse__longopts_arg(char* option) {
    for (; *option && *option != '='; ++option) {}
    return *option == '=' ? option + 1 : NULL;
}

OPTPARSE_API void optparse_init(optparse_t* options, char** argv) {
    options->errmsg[0] = '\0';
    options->optarg    = NULL;
    options->argv      = argv;
    options->permute   = 1;
    options->optind    = argv[0] ? 1 : 0;
    options->optopt    = 0;
    options->subopt    = 0;
}

static int optparse__get_long_argtype(const optparse_long_t* longopts, int shortname) {
    for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
        if (longopts[i].shortname == shortname) { return (int)longopts[i].argtype; }
    }
    return -1;
}

static int optparse__parse_short(optparse_t* options, const char* optstring, const optparse_long_t* longopts) {
    char* option;
    int   type;
    char* next;

    options->errmsg[0] = '\0';
    options->optopt    = 0;
    options->optarg    = NULL;

    option = options->argv[options->optind];
    if (!option) { return -1; }
    if (optparse__is_dashdash(option)) {
        ++options->optind;
        return -1;
    }
    if (!optparse__is_shortopt(option)) { return -1; }

    option += options->subopt + 1;
    options->optopt = option[0];
    type =
        optstring ? optparse__get_short_argtype(optstring, option[0]) : optparse__get_long_argtype(longopts, option[0]);
    next = options->argv[options->optind + 1];

    switch (type) {
        case OPTPARSE_NONE:
            if (option[1]) {
                ++options->subopt;
            } else {
                options->subopt = 0;
                ++options->optind;
            }
            return option[0];

        case OPTPARSE_REQUIRED:
            options->subopt = 0;
            ++options->optind;
            if (option[1]) {
                options->optarg = option + 1;
            } else if (next != NULL) {
                options->optarg = next;
                ++options->optind;
            } else {
                char str[2]     = {option[0], 0};
                options->optarg = NULL;
                return optparse__set_error(options, OPTPARSE_MSG_MISSING, str);
            }
            return option[0];

        case OPTPARSE_OPTIONAL:
            options->subopt = 0;
            ++options->optind;
            options->optarg = option[1] ? option + 1 : NULL;
            return option[0];

        case -1:
        default: {
            char str[2] = {option[0], 0};
            ++options->optind;
            options->subopt = 0;
            return optparse__set_error(options, OPTPARSE_MSG_INVALID, str);
        }
    }
}

OPTPARSE_API int optparse(optparse_t* options, const char* optstring) {
    for (int i = options->optind; options->argv[i]; ++i) {
        if (optparse__is_dashdash(options->argv[i])) {
            int original_optind = options->optind;
            if (i > original_optind) { optparse__shift_args(options->argv, i, original_optind, 1); }
            options->optind = original_optind + 1;
            return -1;
        }
        if (optparse__is_shortopt(options->argv[i])) {
            int original_optind = options->optind;
            options->optind     = i;
            int r               = optparse__parse_short(options, optstring, NULL);
            int consumed        = options->optind - i;
            if (i > original_optind) { optparse__shift_args(options->argv, i, original_optind, consumed); }
            options->optind = original_optind + consumed;
            return r;
        }
        if (!options->permute) {
            options->optind = i;
            return -1;
        }
    }
    return -1;
}

OPTPARSE_API char* optparse_arg(optparse_t* options) {
    char* option    = options->argv[options->optind];
    options->subopt = 0;
    if (option != NULL) { ++options->optind; }
    return option;
}

static void optparse__lookup_long_index(optparse_t* options, const optparse_long_t* longopts, int* longindex) {
    for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
        if (longopts[i].shortname == options->optopt) {
            *longindex = i;
            return;
        }
    }
    *longindex = -1;
}

static int optparse__parse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex) {
    char* option = options->argv[options->optind];

    options->errmsg[0] = '\0';
    options->optopt    = 0;
    options->optarg    = NULL;
    option += 2;
    ++options->optind;

    for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
        const char* name = longopts[i].longname;
        if (!optparse__longopts_match(name, option)) { continue; }

        if (longindex) { *longindex = i; }

        options->optopt = longopts[i].shortname;
        char* arg       = optparse__longopts_arg(option);

        if (longopts[i].argtype == OPTPARSE_NONE && arg != NULL) {
            return optparse__set_error(options, OPTPARSE_MSG_TOOMANY, name);
        }

        if (arg != NULL) {
            options->optarg = arg;
        } else if (longopts[i].argtype == OPTPARSE_REQUIRED) {
            options->optarg = options->argv[options->optind];
            if (options->optarg == NULL) {
                return optparse__set_error(options, OPTPARSE_MSG_MISSING, name);
            } else {
                ++options->optind;
            }
        }

        return options->optopt;
    }

    return optparse__set_error(options, OPTPARSE_MSG_INVALID, option);
}

OPTPARSE_API int optparse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex) {
    int i;
    for (i = options->optind; options->argv[i]; ++i) {
        char* arg = options->argv[i];
        if (optparse__is_dashdash(arg)) {
            int original_optind = options->optind;
            if (i > original_optind) { optparse__shift_args(options->argv, i, original_optind, 1); }
            options->optind = original_optind + 1;
            return -1;
        }

        int is_short = optparse__is_shortopt(arg);
        int is_long  = optparse__is_longopt(arg);

        if (is_short || is_long) {
            int original_optind = options->optind;
            options->optind     = i;
            int r;
            if (is_short) {
                r = optparse__parse_short(options, NULL, longopts);
                if (r != -1 && longindex != NULL) { optparse__lookup_long_index(options, longopts, longindex); }
            } else {
                r = optparse__parse_long(options, longopts, longindex);
            }

            int consumed = options->optind - i;
            if (i > original_optind) { optparse__shift_args(options->argv, i, original_optind, consumed); }
            options->optind = original_optind + consumed;
            return r;
        }

        if (!options->permute) {
            options->optind = i;
            return -1;
        }
    }
    return -1;
}

static inline optparse_help_config_t optparse__resolve_config(const optparse_help_config_t* cfg) {
    optparse_help_config_t r = cfg ? *cfg : (optparse_help_config_t)OPTPARSE_HELP_CONFIG_DEFAULT;
    if (r.width <= 0) { r.width = 80; }
    if (r.min_desc <= 0) { r.min_desc = 26; }
    if (r.max_left <= 0) { r.max_left = 36; }
    return r;
}

// Width of the left column for one option entry (without trailing spaces).
static int optparse__help_leftwidth(const optparse_long_t* opt) {
    int         has_short = (opt->shortname >= 33 && opt->shortname < 127);
    int         has_long  = (opt->longname && opt->longname[0]);
    const char* argname   = opt->argname ? opt->argname : "ARG";

    int w = 0;
    if (has_long) {
        w = 8 + (int)strlen(opt->longname);  // "  -x, --longname" / "      --longname"
    } else if (has_short) {
        w = 4;  // "  -x"
    }

    switch (opt->argtype) {
        case OPTPARSE_REQUIRED: w += 1 + (int)strlen(argname); break;  // =ARG
        case OPTPARSE_OPTIONAL: w += 3 + (int)strlen(argname); break;  // [=ARG]
        default: break;
    }
    return w;
}

// Print left column and pad to `col` chars.  Always consistent path.
static int optparse__help_printleft(const optparse_long_t* opt, int col, FILE* out) {
    int         has_short = (opt->shortname >= 33 && opt->shortname < 127);
    int         has_long  = (opt->longname && opt->longname[0]);
    const char* argname   = opt->argname ? opt->argname : "ARG";

    int printed = 0;
    if (has_short && has_long) {
        printed = fprintf(out, "  -%c, --%s", (char)opt->shortname, opt->longname);
    } else if (has_short) {
        printed = fprintf(out, "  -%c", (char)opt->shortname);
    } else {
        printed = fprintf(out, "      --%s", has_long ? opt->longname : "");
    }

    // argtype suffix
    switch (opt->argtype) {
        case OPTPARSE_REQUIRED: printed += fprintf(out, "=%s", argname); break;
        case OPTPARSE_OPTIONAL: printed += fprintf(out, "[=%s]", argname); break;
        default: break;
    }
    // pad to col
    while (printed < col) {
        fputc(' ', out);
        ++printed;
    }
    return printed;
}

/* Word-wrap argdesc; continuation lines indent to `col`. */
static void optparse__help_printdesc(const char* desc, int col, int width, FILE* out) {
    int avail = width - col;
    int pos   = 0;
    if (avail < 10) { avail = 10; }

    while (*desc) {
        if (*desc == '\n') {
            fputc('\n', out);
            fprintf(out, "%*s", col, "");
            pos = 0;
            ++desc;
            continue;
        }
        int wlen = 0;
        for (const char* w = desc; *w && *w != ' ' && *w != '\n'; ++w, ++wlen) {}

        if (pos > 0 && pos + 1 + wlen > avail) {
            fputc('\n', out);
            fprintf(out, "%*s", col, "");
            pos = 0;
        }
        if (pos > 0) {
            fputc(' ', out);
            ++pos;
        }
        fwrite(desc, 1, (size_t)wlen, out);
        pos += wlen;
        desc += wlen;
        while (*desc == ' ') { ++desc; }
    }
    fputc('\n', out);
}

OPTPARSE_API void optparse_usage(FILE* out, const char* progname, const optparse_long_t* longopts,
                                 const char* pos_args) {
    fprintf(out, "Usage: %s", progname);
    if (longopts) {
        int has_opts = 0;
        for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
            if (longopts[i].longname || (longopts[i].shortname >= 33 && longopts[i].shortname < 127)) {
                has_opts = 1;
                break;
            }
        }
        if (has_opts) { fprintf(out, " [options]"); }
    }
    if (pos_args && pos_args[0]) { fprintf(out, " %s", pos_args); }
    fputc('\n', out);
}

OPTPARSE_API void optparse_help(FILE* out, const optparse_long_t* longopts, const optparse_help_config_t* cfg) {
    optparse_help_config_t c = optparse__resolve_config(cfg);

    int desc_max = c.min_desc >= c.width ? c.width / 2 : c.width - c.min_desc;
    int desc_col = 0, actual_max = 0;
    for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
        if (!longopts[i].argdesc || !longopts[i].argdesc[0]) { continue; }
        int lw = optparse__help_leftwidth(&longopts[i]) + 2;
        if (lw > actual_max) { actual_max = lw; }
        if (lw <= c.max_left && lw <= desc_max) {
            if (lw > desc_col) { desc_col = lw; }
        }
    }
    if (desc_col == 0 && actual_max > 0) { desc_col = actual_max < desc_max ? actual_max : desc_max; }
    if (desc_col > desc_max) { desc_col = desc_max; }
    if (desc_col < 2) { desc_col = 2; }

    for (int i = 0; !optparse__longopts_end(longopts, i); ++i) {
        const optparse_long_t* opt = &longopts[i];
        if (!opt->argdesc || !opt->argdesc[0]) { continue; }

        int lw = optparse__help_leftwidth(opt);
        if (lw >= desc_col) {
            optparse__help_printleft(opt, 0, out);
            fputc('\n', out);
            fprintf(out, "%*s", desc_col, "");
        } else {
            optparse__help_printleft(opt, desc_col, out);
        }
        optparse__help_printdesc(opt->argdesc, desc_col, c.width, out);
    }
}

#endif  // OPTPARSE_IMPLEMENTATION
#endif  // OPTPARSE_OPTPARSE_H
