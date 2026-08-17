/*
 * MIT License
 *
 * Copyright (c) 2026 tayne3
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 * 2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 */
#ifndef OPTPARSE_OPTPARSE_H
#define OPTPARSE_OPTPARSE_H

#include <stdio.h>

#ifndef OPTPARSE_API
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(OPTPARSE_LIB_EXPORT)
#define OPTPARSE_API __declspec(dllexport)
#elif defined(OPTPARSE_SHARED)
#define OPTPARSE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if defined(OPTPARSE_LIB_EXPORT) || defined(OPTPARSE_SHARED)
#define OPTPARSE_API __attribute__((visibility("default")))
#endif
#endif
#endif
#ifndef OPTPARSE_API
#define OPTPARSE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** F(code, name, desc) */
#define OPTPARSE_ERROR_FOREACH(F)                \
    F(0, NONE, "none")                           \
    F(1, DONE, "no more options")                \
    F(2, INVALID, "invalid option")              \
    F(3, MISSING, "option requires an argument") \
    F(4, TOOMANY, "option takes no arguments")

typedef enum optparse_error {
#define F(code, name, desc) OPTPARSE_ERROR_##name = code,
    OPTPARSE_ERROR_FOREACH(F)
#undef F
} optparse_error_t;

/** @brief Return a human-readable description for @p s. */
OPTPARSE_API const char* optparse_strerror(optparse_error_t s);

/**
 * @brief Parser state; initialize with optparse_init().
 *
 * Only permute should be set by the caller, before parsing begins.
 * All other fields are read-only; inspect them after optparse_next().
 */
typedef struct optparse_s {
    char*  optarg;  /* argument string for the current option, or NULL */
    char** argv;    /* argv passed to optparse_init(); for error reporting */
    int    permute; /* non-zero (default) permutes non-options to end; zero = POSIX mode */
    int    optind;  /* index of next argv element to process */
    int    optopt;  /* shortname of option just matched (0 for unknown long options) */
    int    subind;  /* internal: byte offset within current short-option cluster */
} optparse_t;

typedef enum optparse_argtype {
    OPTPARSE_NONE = 0,
    OPTPARSE_REQUIRED,
    OPTPARSE_OPTIONAL,
} optparse_argtype_t;

/**
 * @brief Option descriptor.
 *
 * Terminate arrays with {0, 0, OPTPARSE_NONE, NULL}.
 * Set desc to NULL to hide an entry from help output.
 */
typedef struct optparse_def {
    const char*        longname;
    int                shortname; /* printable ASCII; use non-printable for long-only options */
    optparse_argtype_t argtype;
    const char*        desc;    /* help text; NULL hides this entry */
    const char*        metavar; /* argument placeholder in help, default "ARG" */
} optparse_def_t;

#define OPTPARSE_DEF_NULL {NULL, 0, OPTPARSE_NONE, NULL, NULL}

/**
 * @brief Initialize parser state; must be called before optparse_next().
 * @param self  Parser state to initialize.
 * @param argv  Argument vector from main(); argv[0] is skipped.
 */
OPTPARSE_API void optparse_init(optparse_t* self, char** argv);

/**
 * @brief Consume and return the next positional argument.
 *
 * Useful for stepping past sub-commands before resuming option parsing.
 *
 * @param self  Parser state.
 * @return Next argument string, or NULL if none remain.
 */
OPTPARSE_API char* optparse_arg(optparse_t* self);

/**
 * @brief Count the remaining argv elements.

 * @param self  Parser state.
 * @return Number of remaining argv elements.
 */
OPTPARSE_API int optparse_narg(const optparse_t* self);

/**
 * @brief Parse the next option.
 *
 * Supports short options (-x), short clusters (-xyz), and GNU-style
 * long options (--foo, --foo=bar). When permute is set, non-option
 * arguments are shifted to the end so all options are processed first.
 *
 * @param self    Parser state (modified in place).
 * @param defs    Option descriptors, terminated by {0,0,OPTPARSE_NONE,NULL}.
 * @param out_id  Receives the matched option's shortname; may be NULL.
 * @return OPTPARSE_ERROR_NONE on success, OPTPARSE_ERROR_DONE when finished, or an error code.
 *         On error, self->optopt holds the offending option character.
 */
OPTPARSE_API optparse_error_t optparse_next(optparse_t* self, const optparse_def_t* defs, int* out_id);

/**
 * @brief Column layout for optparse_help().
 *
 *   |<----------- width ----------->|
 *   |  -o, --option=ARG  description|
 *   |<--- max_left --->|            |
 *   |                  |<-min_desc->|
 */
typedef struct optparse_help_config {
    int width;    /**< total line width */
    int min_desc; /**< minimum columns reserved for description */
    int max_left; /**< maximum columns for the option part */
} optparse_help_config_t;

#define OPTPARSE_HELP_CONFIG_INIT {80, 26, 36}

/**
 * @brief Print a usage line: "Usage: <progname> [options] <pos_args>\n"
 *
 * @param out       Output stream (typically stdout or stderr).
 * @param progname  Program name, typically argv[0].
 * @param defs      Descriptor array; if non-NULL and non-empty, "[options]" is appended. May be NULL.
 * @param count     Number of entries in defs, or -1 to stop at sentinel.
 * @param pos_args  Positional argument synopsis, e.g. "SOURCE DEST". May be NULL.
 */
OPTPARSE_API void optparse_usage(FILE* out, const char* progname, const optparse_def_t* defs, int count,
                                 const char* pos_args);

/**
 * @brief Print formatted option descriptions.
 *
 * Entries with a NULL or empty desc are skipped. Pass a sub-range via
 * defs pointer and count to print sections with custom headers in between.
 *
 * @param out    Output stream.
 * @param defs   Descriptor array, same as passed to optparse_next().
 * @param count  Number of entries to print, or -1 to stop at sentinel.
 * @param cfg    Layout config, or NULL for defaults (OPTPARSE_HELP_CONFIG_INIT).
 */
OPTPARSE_API void optparse_help(FILE* out, const optparse_def_t* defs, int count, const optparse_help_config_t* cfg);

#ifdef __cplusplus
}

namespace optparse {

enum class Error {
    None    = OPTPARSE_ERROR_NONE,
    Done    = OPTPARSE_ERROR_DONE,
    Invalid = OPTPARSE_ERROR_INVALID,
    Missing = OPTPARSE_ERROR_MISSING,
    TooMany = OPTPARSE_ERROR_TOOMANY,
};

enum class ArgType {
    None     = OPTPARSE_NONE,
    Required = OPTPARSE_REQUIRED,
    Optional = OPTPARSE_OPTIONAL,
};

struct Option : public optparse_def_t {
    Option() {
        longname  = nullptr;
        shortname = 0;
        argtype   = OPTPARSE_NONE;
        desc      = nullptr;
        metavar   = nullptr;
    }
    Option(const char* ln, int sn, ArgType at, const char* d = nullptr, const char* mv = nullptr) {
        longname  = ln;
        shortname = sn;
        argtype   = static_cast<optparse_argtype_t>(at);
        desc      = d;
        metavar   = mv;
    }
};

using HelpConfig = optparse_help_config_t;

class Parser {
public:
    explicit Parser(char** argv) noexcept { optparse_init(&d, argv); }

    Parser(const Parser&)            = delete;
    Parser& operator=(const Parser&) = delete;

    /** @brief Parse the next option. */
    Error next(const Option* defs, int* out_id = nullptr) {
        return static_cast<Error>(optparse_next(&d, static_cast<const optparse_def_t*>(defs), out_id));
    }

    /** @brief Consume and return the next positional argument. */
    char* arg() noexcept { return optparse_arg(&d); }

    /** @brief Count the remaining positional arguments.  */
    int narg() const noexcept { return optparse_narg(&d); }

    /** @brief Argument of the option most recently matched by next(); read-only, does not advance. */
    char* optarg() const noexcept { return d.optarg; }

    /** @brief Shortname of the option most recently matched by next() (0 for unknown long options). */
    int optopt() const noexcept { return d.optopt; }

    /** @brief Index of the next argv element to be processed. */
    int optind() const noexcept { return d.optind; }

    /** @brief Byte offset within the current short-option cluster. */
    int subind() const noexcept { return d.subind; }

    /** @brief Whether argv permutation is enabled. */
    bool permute() const noexcept { return d.permute != 0; }

    /** @brief Enable or disable argv permutation; set before parsing. */
    void set_permute(bool v) noexcept { d.permute = v ? 1 : 0; }

    /** @brief Human-readable message for an error code. */
    static const char* strerror(Error s) noexcept { return optparse_strerror(static_cast<optparse_error_t>(s)); }

    /** @brief Print a "Usage: ..." line to out. */
    static void usage(FILE* out, const char* progname, const Option* defs, int count = -1,
                      const char* pos_args = nullptr) {
        optparse_usage(out, progname, static_cast<const optparse_def_t*>(defs), count, pos_args);
    }

    /** @brief Print a formatted option list to out. */
    static void help(FILE* out, const Option* defs, int count = -1, const HelpConfig* cfg = nullptr) {
        optparse_help(out, static_cast<const optparse_def_t*>(defs), count, cfg);
    }

private:
    optparse_t d;
};

}  // namespace optparse
#endif

#endif /* OPTPARSE_OPTPARSE_H */
