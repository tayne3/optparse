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
#include "optparse/optparse.h"

#include <stdlib.h>
#include <string.h>

const char* optparse_strerror(optparse_error_t s) {
    switch (s) {
#define F(code, name, desc) \
    case code: return desc;
        OPTPARSE_ERROR_FOREACH(F)
#undef F
        default: return "unknown status";
    }
}

static int optparse__is_dashdash(const char* a) {
    return a && a[0] == '-' && a[1] == '-' && a[2] == '\0';
}
static int optparse__is_short(const char* a) {
    return a && a[0] == '-' && a[1] != '-' && a[1] != '\0';
}
static int optparse__is_long(const char* a) {
    return a && a[0] == '-' && a[1] == '-' && a[2] != '\0';
}
static int optparse__is_end(const optparse_def_t* d) {
    return !d->longname && !d->shortname;
}

static int optparse__match(const char* longname, const char* option) {
    const char *a = option, *n = longname;
    if (!longname) { return 0; }
    for (; *a && *n && *a != '='; ++a, ++n) {
        if (*a != *n) { return 0; }
    }
    return *n == '\0' && (*a == '\0' || *a == '=');
}

static void optparse__permute(char** argv, int from, int to, int count) {
    for (int k = 0; k < count; ++k) {
        char* tmp = argv[from + k];
        for (int j = from + k; j > to + k; --j) { argv[j] = argv[j - 1]; }
        argv[to + k] = tmp;
    }
}

static optparse_error_t optparse__parse_short(optparse_t* opts, const optparse_def_t* defs, int* out_id) {
    opts->optopt = 0;
    opts->optarg = NULL;

    char* option = opts->argv[opts->optind];
    if (!option) { return OPTPARSE_ERROR_DONE; }
    if (optparse__is_dashdash(option)) {
        ++opts->optind;
        return OPTPARSE_ERROR_DONE;
    }
    if (!optparse__is_short(option)) { return OPTPARSE_ERROR_DONE; }

    option += opts->subind + 1;
    opts->optopt = option[0];
    if (out_id) { *out_id = option[0]; } /* record before returning */

    char* next = opts->argv[opts->optind + 1];

    int type = -1;
    if (option[0] >= 33 && option[0] < 127) {
        for (int i = 0; !optparse__is_end(&defs[i]); ++i) {
            if (defs[i].shortname == (int)option[0]) { type = (int)defs[i].argtype; }
        }
    }

    switch (type) {
        case OPTPARSE_NONE:
            if (option[1]) {
                ++opts->subind;
            } else {
                opts->subind = 0;
                ++opts->optind;
            }
            return OPTPARSE_ERROR_NONE;

        case OPTPARSE_REQUIRED:
            opts->subind = 0;
            ++opts->optind;
            if (option[1]) {
                opts->optarg = option + 1;
            } else if (next) {
                opts->optarg = next;
                ++opts->optind;
            } else {
                return OPTPARSE_ERROR_MISSING;
            }
            return OPTPARSE_ERROR_NONE;

        case OPTPARSE_OPTIONAL:
            opts->subind = 0;
            ++opts->optind;
            opts->optarg = option[1] ? option + 1 : NULL;
            return OPTPARSE_ERROR_NONE;

        default:
            opts->subind = 0;
            ++opts->optind;
            return OPTPARSE_ERROR_INVALID;
    }
}

static optparse_error_t optparse__parse_long(optparse_t* opts, const optparse_def_t* defs, int* out_id) {
    opts->optopt = 0;
    opts->optarg = NULL;

    char* option = opts->argv[opts->optind] + 2; /* skip "--" */
    ++opts->optind;

    for (int i = 0; !optparse__is_end(&defs[i]); ++i) {
        if (!optparse__match(defs[i].longname, option)) { continue; }

        opts->optopt = defs[i].shortname;
        if (out_id) { *out_id = defs[i].shortname; }

        char* val = strchr(option, '=');
        if (defs[i].argtype == OPTPARSE_NONE && val) { return OPTPARSE_ERROR_TOOMANY; }
        if (val) {
            opts->optarg = val + 1;
        } else if (defs[i].argtype == OPTPARSE_REQUIRED) {
            opts->optarg = opts->argv[opts->optind];
            if (!opts->optarg) { return OPTPARSE_ERROR_MISSING; }
            ++opts->optind;
        }
        return OPTPARSE_ERROR_NONE;
    }
    return OPTPARSE_ERROR_INVALID;
}

void optparse_init(optparse_t* opts, char** argv) {
    opts->optarg  = NULL;
    opts->argv    = argv;
    opts->permute = 1;
    opts->optind  = argv[0] ? 1 : 0;
    opts->optopt  = 0;
    opts->subind  = 0;
}

char* optparse_shift(optparse_t* opts) {
    char* arg    = opts->argv[opts->optind];
    opts->subind = 0;
    if (arg) { ++opts->optind; }
    return arg;
}

/*
 * Scan forward from optind. When an option is found at index i, permute
 * the non-option tokens in [optind, i) to after the consumed option tokens,
 * then advance optind past all consumed tokens.
 */
optparse_error_t optparse_next(optparse_t* opts, const optparse_def_t* defs, int* out_id) {
    for (int i = opts->optind; opts->argv[i]; ++i) {
        char* arg = opts->argv[i];

        if (optparse__is_dashdash(arg)) {
            int target = opts->optind;
            if (i > target) { optparse__permute(opts->argv, i, target, 1); }
            opts->optind = target + 1;
            return OPTPARSE_ERROR_DONE;
        }

        int is_short = optparse__is_short(arg);
        int is_long  = optparse__is_long(arg);
        if (!is_short && !is_long) {
            if (!opts->permute) {
                opts->optind = i;
                return OPTPARSE_ERROR_DONE;
            }
            continue;
        }

        int target   = opts->optind;
        opts->optind = i;
        optparse_error_t r =
            is_short ? optparse__parse_short(opts, defs, out_id) : optparse__parse_long(opts, defs, out_id);
        int consumed = opts->optind - i;
        if (i > target) optparse__permute(opts->argv, i, target, consumed);
        opts->optind = target + consumed;
        return r;
    }
    return OPTPARSE_ERROR_DONE;
}

static optparse_help_config_t optparse__resolve_config(const optparse_help_config_t* cfg) {
    optparse_help_config_t r = OPTPARSE_HELP_CONFIG_INIT;
    if (!cfg) { return r; }
    if (cfg->width > 0) { r.width = cfg->width; }
    if (cfg->min_desc > 0) { r.min_desc = cfg->min_desc; }
    if (cfg->max_left > 0) { r.max_left = cfg->max_left; }
    return r;
}

static int optparse__help_width(const optparse_def_t* opt) {
    const int   has_short = (opt->shortname >= 33 && opt->shortname < 127);
    const int   has_long  = (opt->longname && opt->longname[0]);
    const char* metavar   = opt->metavar ? opt->metavar : "ARG";
    int         w         = has_long ? 8 + (int)strlen(opt->longname) : has_short ? 4 : 0;
    switch (opt->argtype) {
        case OPTPARSE_REQUIRED: w += 1 + (int)strlen(metavar); break; /* =ARG   */
        case OPTPARSE_OPTIONAL: w += 3 + (int)strlen(metavar); break; /* [=ARG] */
        default: break;
    }
    return w;
}

static int optparse__help_option(const optparse_def_t* opt, int col, FILE* out) {
    const int   has_short = (opt->shortname >= 33 && opt->shortname < 127);
    const int   has_long  = (opt->longname && opt->longname[0]);
    const char* metavar   = opt->metavar ? opt->metavar : "ARG";
    int         printed;
    if (has_short && has_long) {
        printed = fprintf(out, "  -%c, --%s", (char)opt->shortname, opt->longname);
    } else if (has_short) {
        printed = fprintf(out, "  -%c", (char)opt->shortname);
    } else {
        printed = fprintf(out, "      --%s", has_long ? opt->longname : "");
    }
    switch (opt->argtype) {
        case OPTPARSE_REQUIRED: printed += fprintf(out, "=%s", metavar); break;
        case OPTPARSE_OPTIONAL: printed += fprintf(out, "[=%s]", metavar); break;
        default: break;
    }
    while (printed < col) {
        fputc(' ', out);
        ++printed;
    }
    return printed;
}

static void optparse__help_desc(const char* desc, int col, int width, FILE* out) {
    int avail = width - col;
    int pos   = 0;
    if (avail < 10) avail = 10;
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

void optparse_usage(FILE* out, const char* progname, const optparse_def_t* defs, int count, const char* pos_args) {
    if (!out || !progname) { return; }
    fprintf(out, "Usage: %s", progname);
    if (defs) {
        int has_opts = 0;
        for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&defs[i]); ++i) {
            if (defs[i].longname || (defs[i].shortname >= 33 && defs[i].shortname < 127)) {
                has_opts = 1;
                break;
            }
        }
        if (has_opts) { fprintf(out, " [options]"); }
    }
    if (pos_args && pos_args[0]) { fprintf(out, " %s", pos_args); }
    fputc('\n', out);
}

void optparse_help(FILE* out, const optparse_def_t* defs, int count, const optparse_help_config_t* cfg) {
    if (!out || !defs || !count || optparse__is_end(&defs[0])) { return; }
    optparse_help_config_t c        = optparse__resolve_config(cfg);
    const int              desc_max = c.min_desc >= c.width ? c.width / 2 : c.width - c.min_desc;

    int desc_col = 0, actual_max = 0;
    for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&defs[i]); ++i) {
        if (!defs[i].desc || !defs[i].desc[0]) { continue; }
        int lw = optparse__help_width(&defs[i]) + 2;
        if (lw > actual_max) { actual_max = lw; }
        if (lw <= c.max_left && lw <= desc_max && lw > desc_col) { desc_col = lw; }
    }
    if (desc_col == 0 && actual_max > 0) { desc_col = actual_max < desc_max ? actual_max : desc_max; }
    if (desc_col > desc_max) { desc_col = desc_max; }
    if (desc_col < 2) { desc_col = 2; }

    for (int i = 0; (count < 0 || i < count) && !optparse__is_end(&defs[i]); ++i) {
        const optparse_def_t* opt = &defs[i];
        if (!opt->desc || !opt->desc[0]) { continue; }
        if (optparse__help_width(opt) + 2 > desc_col) {
            optparse__help_option(opt, 0, out);
            fputc('\n', out);
            fprintf(out, "%*s", desc_col, "");
        } else {
            optparse__help_option(opt, desc_col, out);
        }
        optparse__help_desc(opt->desc, desc_col, c.width, out);
    }
}
