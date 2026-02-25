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

#ifndef OPTPARSE_API
#define OPTPARSE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Core parser state.
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
	char **argv;
	int    permute;
	int    optind;
	int    optopt;
	char  *optarg;
	char   errmsg[64];
	int    subopt; /* internal: offset within short-opt cluster */
} optparse_t;

typedef enum optparse_argtype {
	OPTPARSE_NONE     = 0,
	OPTPARSE_REQUIRED = 1,
	OPTPARSE_OPTIONAL = 2,
} optparse_argtype_t;

/* Long option descriptor. Terminate array with {0, 0, OPTPARSE_NONE}. */
typedef struct optparse_long {
	const char        *longname;
	int                shortname; /* corresponding short char, or >127 for long-only */
	optparse_argtype_t argtype;
} optparse_long_t;

OPTPARSE_API void optparse_init(optparse_t *options, char **argv);

/* Parse next short option. Returns option character, -1 when done, '?' on error.
 * Option string follows getopt() conventions: no colon = no argument,
 * one colon = required, two colons = optional.
 */
OPTPARSE_API int optparse(optparse_t *options, const char *optstring);

/* Parse next option, supporting both short and GNU-style long options.
 * longindex receives index into longopts (-1 for short options).
 */
OPTPARSE_API int optparse_long(optparse_t *options, const optparse_long_t *longopts, int *longindex);

/* Retrieve next non-option argument. Useful for stepping over sub-commands
 * to continue parsing with a fresh option string.
 */
OPTPARSE_API char *optparse_arg(optparse_t *options);

#ifdef __cplusplus
}
#endif

/* IMPLEMENTATION */
#ifdef OPTPARSE_IMPLEMENTATION

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

static int optparse__error(optparse_t *options, const char *msg, const char *data) {
	unsigned    p   = 0;
	const char *sep = " -- '";

	while (*msg && p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = *msg++; }
	while (*sep && p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = *sep++; }
	while (*data && p < sizeof(options->errmsg) - 2) { options->errmsg[p++] = *data++; }

	if (p < sizeof(options->errmsg) - 1) { options->errmsg[p++] = '\''; }
	options->errmsg[p] = '\0';

	return '?';
}

static int optparse__is_dashdash(const char *arg) {
	return (arg != NULL) && (arg[0] == '-') && (arg[1] == '-') && (arg[2] == '\0');
}

static int optparse__is_shortopt(const char *arg) {
	return (arg != NULL) && (arg[0] == '-') && (arg[1] != '-') && (arg[1] != '\0');
}

static int optparse__is_longopt(const char *arg) {
	return (arg != NULL) && (arg[0] == '-') && (arg[1] == '-') && (arg[2] != '\0');
}

/* Move non-option argument toward end of option block to implement
 * "permute non-options to end" behavior.
 */
static void optparse__permute(optparse_t *options, int index) {
	char *nonoption = options->argv[index];
	int   i;
	for (i = index; i < options->optind - 1; ++i) { options->argv[i] = options->argv[i + 1]; }
	options->argv[options->optind - 1] = nonoption;
}

static int optparse__argtype(const char *optstring, char c) {
	int count = (int)OPTPARSE_NONE;
	if (c == ':') { return -1; }
	for (; *optstring && c != *optstring; ++optstring) {}
	if (!*optstring) { return -1; }
	if (optstring[1] == ':') { count += (optstring[2] == ':') ? 2 : 1; }
	return count;
}

static int optparse_longopts_end(const optparse_long_t *longopts, int i) {
	return !longopts[i].longname && !longopts[i].shortname;
}

/* Build optstring from longopts array so short-option fallback path can be reused. */
static void optparse__from_long(const optparse_long_t *longopts, char *optstring) {
	char *p = optstring;
	int   i, a;
	for (i = 0; !optparse_longopts_end(longopts, i); ++i) {
		if (longopts[i].shortname > 0 && longopts[i].shortname < 127) {
			*p++ = (char)longopts[i].shortname;
			for (a = 0; a < (int)longopts[i].argtype; ++a) { *p++ = ':'; }
		}
	}
	*p = '\0';
}

static int optparse__longopts_match(const char *longname, const char *option) {
	const char *a = option, *n = longname;
	if (longname == NULL) { return 0; }
	for (; *a && *n && *a != '='; ++a, ++n) {
		if (*a != *n) { return 0; }
	}
	return (*n == '\0') && (*a == '\0' || *a == '=');
}

static char *optparse__longopts_arg(char *option) {
	for (; *option && *option != '='; ++option) {}
	return (*option == '=') ? option + 1 : NULL;
}

/* Fall back to short-option parsing when encountering short-opt cluster
 * inside optparse_long(). Also fills in longindex.
 */
static int optparse__long_fallback(optparse_t *options, const optparse_long_t *longopts, int *longindex) {
	int  result;
	char optstring[96 * 3 + 1];

	optparse__from_long(longopts, optstring);
	result = optparse(options, optstring);

	if (longindex != NULL) {
		*longindex = -1;
		if (result != -1) {
			int i;
			for (i = 0; !optparse_longopts_end(longopts, i); ++i) {
				if (longopts[i].shortname == options->optopt) {
					*longindex = i;
					break;
				}
			}
		}
	}
	return result;
}

OPTPARSE_API void optparse_init(optparse_t *options, char **argv) {
	options->argv      = argv;
	options->permute   = 1;
	options->optind    = (argv[0] != NULL) ? 1 : 0;
	options->subopt    = 0;
	options->optarg    = NULL;
	options->optopt    = 0;
	options->errmsg[0] = '\0';
}

OPTPARSE_API int optparse(optparse_t *options, const char *optstring) {
	char *option;
	int   type;
	char *next;

	options->errmsg[0] = '\0';
	options->optopt    = 0;
	options->optarg    = NULL;

	option = options->argv[options->optind];
	if (option == NULL) { return -1; }
	if (optparse__is_dashdash(option)) {
		options->optind++;
		return -1;
	}
	if (!optparse__is_shortopt(option)) {
		if (options->permute) {
			int index = options->optind++;
			int r     = optparse(options, optstring);
			optparse__permute(options, index);
			options->optind--;
			return r;
		} else {
			return -1;
		}
	}

	option += options->subopt + 1;
	options->optopt = option[0];
	type            = optparse__argtype(optstring, (char)option[0]);
	next            = options->argv[options->optind + 1];

	switch (type) {
		case -1: {
			char str[2] = {option[0], 0};
			options->optind++;
			options->subopt = 0;
			return optparse__error(options, OPTPARSE_MSG_INVALID, str);
		}

		case OPTPARSE_NONE:
			if (option[1]) {
				options->subopt++;
			} else {
				options->subopt = 0;
				options->optind++;
			}
			return option[0];

		case OPTPARSE_REQUIRED:
			options->subopt = 0;
			options->optind++;
			if (option[1]) {
				options->optarg = option + 1;
			} else if (next != NULL) {
				options->optarg = next;
				options->optind++;
			} else {
				char str[2]     = {option[0], 0};
				options->optarg = NULL;
				return optparse__error(options, OPTPARSE_MSG_MISSING, str);
			}
			return option[0];

		case OPTPARSE_OPTIONAL:
			options->subopt = 0;
			options->optind++;
			options->optarg = option[1] ? option + 1 : NULL;
			return option[0];

		default: return 0;
	}
}

OPTPARSE_API char *optparse_arg(optparse_t *options) {
	char *option    = options->argv[options->optind];
	options->subopt = 0;
	if (option != NULL) { options->optind++; }
	return option;
}

OPTPARSE_API int optparse_long(optparse_t *options, const optparse_long_t *longopts, int *longindex) {
	char *option = options->argv[options->optind];
	int   i;

	if (option == NULL) { return -1; }
	if (optparse__is_dashdash(option)) {
		options->optind++;
		return -1;
	}

	if (optparse__is_shortopt(option)) { return optparse__long_fallback(options, longopts, longindex); }
	if (!optparse__is_longopt(option)) {
		if (options->permute) {
			int index = options->optind++;
			int r     = optparse_long(options, longopts, longindex);
			optparse__permute(options, index);
			options->optind--;
			return r;
		} else {
			return -1;
		}
	}

	options->errmsg[0] = '\0';
	options->optopt    = 0;
	options->optarg    = NULL;
	option += 2;
	options->optind++;

	for (i = 0; !optparse_longopts_end(longopts, i); i++) {
		const char *name = longopts[i].longname;
		if (!optparse__longopts_match(name, option)) { continue; }

		if (longindex != NULL) { *longindex = i; }

		options->optopt = longopts[i].shortname;
		char *arg       = optparse__longopts_arg(option);

		if (longopts[i].argtype == OPTPARSE_NONE && arg != NULL) { return optparse__error(options, OPTPARSE_MSG_TOOMANY, name); }

		if (arg != NULL) {
			options->optarg = arg;
		} else if (longopts[i].argtype == OPTPARSE_REQUIRED) {
			options->optarg = options->argv[options->optind];
			if (options->optarg == NULL) {
				return optparse__error(options, OPTPARSE_MSG_MISSING, name);
			} else {
				options->optind++;
			}
		}

		return options->optopt;
	}

	return optparse__error(options, OPTPARSE_MSG_INVALID, option);
}

#endif /* OPTPARSE_IMPLEMENTATION */
#endif /* OPTPARSE_OPTPARSE_H */
