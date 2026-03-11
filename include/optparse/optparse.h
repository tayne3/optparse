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

/**
 * @brief Help formatter layout configuration.
 *
 * Controls column widths for optparse_help() output:
 *
 *   |<----------- width ----------->|
 *   |  -o, --option=ARG  description|
 *   |<--- max_left --->|            |
 *   |                  |<-min_desc->|
 *   |-------------------------------|
 */
typedef struct optparse_help_config {
    int width;    /**< total line width */
    int min_desc; /**< minimum columns reserved for description */
    int max_left; /**< maximum columns for option part before forcing a line break */
} optparse_help_config_t;

#define OPTPARSE_HELP_CONFIG_INIT {80, 26, 36}

/**
 * @brief Output callback for help/usage formatting.
 *
 * The formatter calls this function to emit text fragments.
 * Typical implementations: fwrite wrapper, string buffer append, etc.
 *
 * @param str  text fragment (NOT null-terminated)
 * @param len  byte count of @p str
 * @param userdata opaque context passed through from the caller
 */
typedef void (*optparse_write_cb)(const char* str, int len, void* userdata);

/**
 * @brief Print usage line via callback.
 *
 * Output format: "Usage: <progname> [options] <pos_args>\n"
 *
 * @param write    output callback
 * @param userdata     opaque context forwarded to @p write
 * @param progname program name (typically argv[0])
 * @param longopts long option array; if non-NULL and contains visible options, "[options]" is appended (may be NULL)
 * @param count    element count of @p longopts, or -1 to auto-detect sentinel
 * @param pos_args positional argument synopsis appended after "[options]" (may be NULL, e.g. "SOURCE DEST")
 */
OPTPARSE_API void optparse_usage(optparse_write_cb write, void* userdata, const char* progname,
                                 const optparse_long_t* longopts, int count, const char* pos_args);

/**
 * @brief Print formatted option help via callback.
 *
 * Iterates @p longopts; entries whose argdesc is NULL or empty are skipped.
 *
 * @param write    output callback
 * @param userdata     opaque context forwarded to @p write
 * @param longopts same descriptor array passed to optparse_long()
 * @param count    element count of @p longopts, or -1 to auto-detect sentinel
 * @param cfg      layout config, or NULL for defaults (see OPTPARSE_HELP_CONFIG_INIT)
 */
OPTPARSE_API void optparse_help(optparse_write_cb write, void* userdata, const optparse_long_t* longopts, int count,
                                const optparse_help_config_t* cfg);

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

static inline int optparse__strlen(const char* s) {
    int len = 0;
    while (s && s[len]) { len++; }
    return len;
}

static inline void optparse__write_c(optparse_write_cb write, void* userdata, char c) {
    char buf[1];
    buf[0] = c;
    write(buf, 1, userdata);
}

static inline void optparse__write_s(optparse_write_cb write, void* userdata, const char* s) {
    if (s) { write(s, optparse__strlen(s), userdata); }
}

static inline void optparse__write_n(optparse_write_cb write, void* userdata, char c, int n) {
    for (int i = 0; i < n; ++i) { optparse__write_c(write, userdata, c); }
}

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

OPTPARSE_API char* optparse_arg(optparse_t* options) {
    char* option    = options->argv[options->optind];
    options->subopt = 0;
    if (option != NULL) { ++options->optind; }
    return option;
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

static inline optparse_help_config_t optparse__resolve_config(const optparse_help_config_t* cfg) {
    optparse_help_config_t r = OPTPARSE_HELP_CONFIG_INIT;
    if (cfg) {
        r.width    = cfg->width > 0 ? cfg->width : 80;
        r.min_desc = cfg->min_desc > 0 ? cfg->min_desc : 26;
        r.max_left = cfg->max_left > 0 ? cfg->max_left : 36;
    }
    return r;
}

static int optparse__help_width(const optparse_long_t* opt) {
    const int   has_short = (opt->shortname >= 33 && opt->shortname < 127);
    const int   has_long  = (opt->longname && opt->longname[0]);
    const char* argname   = opt->argname ? opt->argname : "ARG";

    int w = 0;
    if (has_long) {
        w = 8 + optparse__strlen(opt->longname);  // "  -x, --longname" / "      --longname"
    } else if (has_short) {
        w = 4;  // "  -x"
    }

    switch (opt->argtype) {
        case OPTPARSE_REQUIRED: w += 1 + optparse__strlen(argname); break;  // =ARG
        case OPTPARSE_OPTIONAL: w += 3 + optparse__strlen(argname); break;  // [=ARG]
        default: break;
    }
    return w;
}

static int optparse__help_option(optparse_write_cb write, void* userdata, const optparse_long_t* opt, int col) {
    const int   has_short = (opt->shortname >= 33 && opt->shortname < 127);
    const int   has_long  = (opt->longname && opt->longname[0]);
    const char* argname   = opt->argname ? opt->argname : "ARG";

    int printed = 0;
    if (has_short && has_long) {
        optparse__write_s(write, userdata, "  -");
        optparse__write_c(write, userdata, (char)opt->shortname);
        optparse__write_s(write, userdata, ", --");
        optparse__write_s(write, userdata, opt->longname);
        printed = 8 + optparse__strlen(opt->longname);
    } else if (has_short) {
        optparse__write_s(write, userdata, "  -");
        optparse__write_c(write, userdata, (char)opt->shortname);
        printed = 4;
    } else {
        optparse__write_s(write, userdata, "      --");
        if (has_long) { optparse__write_s(write, userdata, opt->longname); }
        printed = 8 + (has_long ? optparse__strlen(opt->longname) : 0);
    }

    switch (opt->argtype) {
        case OPTPARSE_REQUIRED:
            optparse__write_c(write, userdata, '=');
            optparse__write_s(write, userdata, argname);
            printed += 1 + optparse__strlen(argname);
            break;
        case OPTPARSE_OPTIONAL:
            optparse__write_s(write, userdata, "[=");
            optparse__write_s(write, userdata, argname);
            optparse__write_c(write, userdata, ']');
            printed += 3 + optparse__strlen(argname);
            break;
        default: break;
    }
    if (printed < col) {
        optparse__write_n(write, userdata, ' ', col - printed);
        printed = col;
    }
    return printed;
}

static void optparse__help_desc(optparse_write_cb write, void* userdata, const char* desc, int col, int width) {
    int avail = width - col;
    int pos   = 0;
    if (avail < 10) { avail = 10; }

    while (*desc) {
        if (*desc == '\n') {
            optparse__write_c(write, userdata, '\n');
            optparse__write_n(write, userdata, ' ', col);
            pos = 0;
            ++desc;
            continue;
        }
        int wlen = 0;
        for (const char* w = desc; *w && *w != ' ' && *w != '\n'; ++w, ++wlen) {}

        if (pos > 0 && pos + 1 + wlen > avail) {
            optparse__write_c(write, userdata, '\n');
            optparse__write_n(write, userdata, ' ', col);
            pos = 0;
        }
        if (pos > 0) {
            optparse__write_c(write, userdata, ' ');
            ++pos;
        }
        write(desc, wlen, userdata);
        pos += wlen;
        desc += wlen;
        while (*desc == ' ') { ++desc; }
    }
    optparse__write_c(write, userdata, '\n');
}

OPTPARSE_API void optparse_usage(optparse_write_cb write, void* userdata, const char* progname,
                                 const optparse_long_t* longopts, int count, const char* pos_args) {
    optparse__write_s(write, userdata, "Usage: ");
    optparse__write_s(write, userdata, progname);
    if (longopts) {
        int has_opts = 0;
        for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&longopts[i]); ++i) {
            if (longopts[i].longname || (longopts[i].shortname >= 33 && longopts[i].shortname < 127)) {
                has_opts = 1;
                break;
            }
        }
        if (has_opts) { optparse__write_s(write, userdata, " [options]"); }
    }
    if (pos_args && pos_args[0]) {
        optparse__write_c(write, userdata, ' ');
        optparse__write_s(write, userdata, pos_args);
    }
    optparse__write_c(write, userdata, '\n');
}

OPTPARSE_API void optparse_help(optparse_write_cb write, void* userdata, const optparse_long_t* longopts, int count,
                                const optparse_help_config_t* cfg) {
    if (!write || !longopts || !count || optparse__is_end(&longopts[0])) { return; }
    optparse_help_config_t c = optparse__resolve_config(cfg);

    const int desc_max = c.min_desc >= c.width ? c.width / 2 : c.width - c.min_desc;
    int       desc_col = 0, actual_max = 0;
    for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&longopts[i]); ++i) {
        if (!longopts[i].argdesc || !longopts[i].argdesc[0]) { continue; }
        int lw = optparse__help_width(&longopts[i]) + 2;
        if (lw > actual_max) { actual_max = lw; }
        if (lw <= c.max_left && lw <= desc_max) {
            if (lw > desc_col) { desc_col = lw; }
        }
    }
    if (desc_col == 0 && actual_max > 0) { desc_col = actual_max < desc_max ? actual_max : desc_max; }
    if (desc_col > desc_max) { desc_col = desc_max; }
    if (desc_col < 2) { desc_col = 2; }

    for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&longopts[i]); ++i) {
        const optparse_long_t* opt = &longopts[i];
        if (!opt->argdesc || !opt->argdesc[0]) { continue; }

        int lw = optparse__help_width(opt);
        if (lw >= desc_col) {
            optparse__help_option(write, userdata, opt, 0);
            optparse__write_c(write, userdata, '\n');
            optparse__write_n(write, userdata, ' ', desc_col);
        } else {
            optparse__help_option(write, userdata, opt, desc_col);
        }
        optparse__help_desc(write, userdata, opt->argdesc, desc_col, c.width);
    }
}

#ifdef __cplusplus
}
#endif
#endif  // OPTPARSE_IMPLEMENTATION
#endif  // OPTPARSE_OPTPARSE_H
