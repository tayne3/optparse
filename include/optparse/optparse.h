/**
 * @file optparse.h
 * @brief Portable, reentrant, embeddable command-line option parser.
 *
 * A C99 replacement for POSIX getopt() / GNU getopt_long() that fixes three
 * fundamental flaws of the standard:
 *
 *  1. All state is stored in a userdata-supplied struct — fully reentrant and
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

/**
 * @brief Initialize parser state; must be called before any parse call.
 * @param options parser state to initialize
 * @param argv    argument vector (typically from main()); argv[0] is skipped
 */
OPTPARSE_API void optparse_init(optparse_t* options, char** argv);

/**
 * @brief Parse next short option.
 * @param options   parser state
 * @param optstring getopt()-style option string: no colon = no argument, one colon = required, two colons = optional
 * @return option character, -1 when done, '?' on error
 */
OPTPARSE_API int optparse(optparse_t* options, const char* optstring);

/**
 * @brief Parse next option, supporting both short and GNU-style long options.
 * @param options   parser state
 * @param longopts  long option descriptor array, terminated with {0, 0, OPTPARSE_NONE, NULL}
 * @param longindex receives index into longopts, or -1 for short options
 * @return option character / shortname, -1 when done, '?' on error
 */
OPTPARSE_API int optparse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex);

/**
 * @brief Retrieve next non-option argument; useful for stepping over sub-commands.
 * @param options parser state
 * @return next non-option argument string, or NULL if none remain
 */
OPTPARSE_API char* optparse_arg(optparse_t* options);

#ifdef __cplusplus
}
#endif

/* ======================================================================
 * IMPLEMENTATION
 * ====================================================================== */
#ifdef OPTPARSE_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

static int optparse__error(optparse_t* options, const char* msg, const char* data) {
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

static inline int optparse__is_short(const char* arg) {
    return arg && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static inline int optparse__is_long(const char* arg) {
    return arg && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static inline int optparse__is_end(const optparse_long_t* opt) {
    return !opt->longname && !opt->shortname;
}

static int optparse__type_short(const char* optstring, char c) {
    if (c == ':') { return -1; }
    for (; *optstring && c != *optstring; ++optstring) {}
    if (optstring[0] == '\0') { return -1; }
    if (optstring[1] == ':') { return optstring[2] == ':' ? 2 : 1; }
    return 0;
}

static int optparse__type_long(const optparse_long_t* longopts, int shortname) {
    for (int i = 0; !optparse__is_end(&longopts[i]); ++i) {
        if (longopts[i].shortname == shortname) { return (int)longopts[i].argtype; }
    }
    return -1;
}

static void optparse__permute(char** argv, int from, int to, int count) {
    for (int k = 0; k < count; ++k) {
        char* tmp = argv[from + k];
        for (int j = from + k; j > to + k; --j) { argv[j] = argv[j - 1]; }
        argv[to + k] = tmp;
    }
}

static int optparse__match(const char* longname, const char* option) {
    const char *a = option, *n = longname;
    if (!longname) { return 0; }
    for (; *a && *n && *a != '='; ++a, ++n) {
        if (*a != *n) { return 0; }
    }
    return *n == '\0' && (*a == '\0' || *a == '=');
}

static char* optparse__get_value(char* option) {
    for (; *option && *option != '='; ++option) {}
    return *option == '=' ? option + 1 : NULL;
}

static int optparse__find_short(const optparse_long_t* longopts, int shortname) {
    for (int i = 0; !optparse__is_end(&longopts[i]); ++i) {
        if (longopts[i].shortname == shortname) { return i; }
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
    if (!optparse__is_short(option)) { return -1; }

    option += options->subopt + 1;
    options->optopt = option[0];
    type            = optstring ? optparse__type_short(optstring, option[0]) : optparse__type_long(longopts, option[0]);
    next            = options->argv[options->optind + 1];

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
                return optparse__error(options, OPTPARSE_MSG_MISSING, str);
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
            return optparse__error(options, OPTPARSE_MSG_INVALID, str);
        }
    }
}

static int optparse__parse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex) {
    char* option = options->argv[options->optind];

    options->errmsg[0] = '\0';
    options->optopt    = 0;
    options->optarg    = NULL;
    option += 2;
    ++options->optind;

    for (int i = 0; !optparse__is_end(&longopts[i]); ++i) {
        const char* name = longopts[i].longname;
        if (!optparse__match(name, option)) { continue; }

        if (longindex) { *longindex = i; }

        options->optopt = longopts[i].shortname;
        char* val       = optparse__get_value(option);

        if (longopts[i].argtype == OPTPARSE_NONE && val != NULL) {
            return optparse__error(options, OPTPARSE_MSG_TOOMANY, name);
        }

        if (val != NULL) {
            options->optarg = val;
        } else if (longopts[i].argtype == OPTPARSE_REQUIRED) {
            options->optarg = options->argv[options->optind];
            if (options->optarg == NULL) {
                return optparse__error(options, OPTPARSE_MSG_MISSING, name);
            } else {
                ++options->optind;
            }
        }

        return options->optopt;
    }

    return optparse__error(options, OPTPARSE_MSG_INVALID, option);
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

OPTPARSE_API int optparse(optparse_t* options, const char* optstring) {
    for (int i = options->optind; options->argv[i]; ++i) {
        if (optparse__is_dashdash(options->argv[i])) {
            const int target = options->optind;
            if (i > target) { optparse__permute(options->argv, i, target, 1); }
            options->optind = target + 1;
            return -1;
        }
        if (optparse__is_short(options->argv[i])) {
            const int target   = options->optind;
            options->optind    = i;
            const int r        = optparse__parse_short(options, optstring, NULL);
            const int consumed = options->optind - i;
            if (i > target) { optparse__permute(options->argv, i, target, consumed); }
            options->optind = target + consumed;
            return r;
        }
        if (!options->permute) {
            options->optind = i;
            return -1;
        }
    }
    return -1;
}

OPTPARSE_API int optparse_long(optparse_t* options, const optparse_long_t* longopts, int* longindex) {
    for (int i = options->optind; options->argv[i]; ++i) {
        char* arg = options->argv[i];
        if (optparse__is_dashdash(arg)) {
            const int target = options->optind;
            if (i > target) { optparse__permute(options->argv, i, target, 1); }
            options->optind = target + 1;
            return -1;
        }

        const int is_short = optparse__is_short(arg);
        const int is_long  = optparse__is_long(arg);

        if (is_short || is_long) {
            const int target = options->optind;
            options->optind  = i;
            int r;
            if (is_short) {
                r = optparse__parse_short(options, NULL, longopts);
                if (r != -1 && longindex != NULL) { *longindex = optparse__find_short(longopts, options->optopt); }
            } else {
                r = optparse__parse_long(options, longopts, longindex);
            }

            const int consumed = options->optind - i;
            if (i > target) { optparse__permute(options->argv, i, target, consumed); }
            options->optind = target + consumed;
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

#ifdef __cplusplus
}
#endif
#endif /* OPTPARSE_IMPLEMENTATION */
#endif /* OPTPARSE_H */
