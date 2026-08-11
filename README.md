# optparse

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/optparse?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/optparse/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/optparse?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/optparse/tags)

A portable, reentrant, embeddable command-line option parser — the library `getopt()` should have been.

## Why not getopt?

The POSIX getopt option parser has three fatal flaws:

1. **Global state.** The getopt parser state is stored entirely in global variables, some of which are static and inaccessible. This means only one thread can use getopt at a time, and recursive parsing of nested sub-arguments is impossible. Optparse fixes this by storing all state in a caller-supplied struct.

2. **No proper reset.** The POSIX standard provides no way to properly reset the parser. For portable code, getopt is only good for one run over one argv. Optparse provides `optparse_shift()` for stepping through non-option arguments, and parsing can continue at any time. A full reset is just another call to `optparse_init()`.

3. **Inaccessible errors.** In getopt, error messages are printed to stderr. This can be disabled with opterr, but the messages themselves are still inaccessible. Optparse returns structured error codes from `optparse_next()`, and `optparse_strerror()` turns them into human-readable messages, so you decide where (or if) to print them.

## Usage

### Integration

**CMake:**

```cmake
FetchContent_Declare(optparse GIT_REPOSITORY https://github.com/tayne3/optparse.git)
FetchContent_MakeAvailable(optparse)
target_link_libraries(my_app PRIVATE optparse::optparse)
```

**Manual Integration:**

Copy `include/optparse/optparse.h` and `src/optparse.c` into your project source tree and compile `optparse.c` alongside your project files.

### Example

```c
#include <stdio.h>
#include <stdlib.h>

#include "optparse/optparse.h"

int main(int argc, char **argv) {
    (void)argc;

    const optparse_def_t defs[] = {
        {"color",   'c', OPTPARSE_REQUIRED, "set output color",          "COLOR"},
        {"version", 'v', OPTPARSE_NONE,     "print version and exit",    NULL},
        {"help",    'h', OPTPARSE_NONE,     "show this help message",    NULL},
        OPTPARSE_DEF_NULL, /* terminator */
    };

    optparse_t options;
    optparse_init(&options, argv);

    optparse_error_t status;
    int              id;
    while ((status = optparse_next(&options, defs, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
        case 'h':
            optparse_usage(stderr, argv[0], defs, -1, "[OPERANDS]");
            fprintf(stderr, "\nOptions:\n");
            optparse_help(stderr, defs, -1, NULL);
            return 0;
        case 'v': printf("version 1.0\n"); return 0;
        case 'c': printf("color: %s\n", options.optarg); break;
        }
    }

    if (status != OPTPARSE_ERROR_DONE) {
        fprintf(stderr, "%s: %s\n", argv[0], optparse_strerror(status));
        return 1;
    }

    char *arg;
    while ((arg = optparse_shift(&options))) {
        printf("%s\n", arg);
    }
    return 0;
}
```

## Permutation

By default, argv is permuted as it is parsed, moving non-option arguments to the end. This can be disabled by setting `permute` to 0 after initialization:

```c
optparse_t options;
optparse_init(&options, argv);
options.permute = 0;
```

## Drop-in Replacement

Optparse's interface should be familiar to anyone accustomed to getopt. The parser struct fields have the same names as the getopt global variables (`optarg`, `optind`, `optopt`), and `optparse_next()` unifies short and long options.

The `optparse_next()` API is similar to GNU's `getopt_long()` and can serve as a portable, embedded replacement.

Optparse does not allocate memory and has no dependencies — not even libc.

## Subcommand Parsing

To parse subcommands, first parse options with permutation disabled (these are the "global" options before the subcommand), then parse the remainder with a fresh `optparse_def_t` array.

See [examples/subcommands.c](examples/subcommands.c) for a complete example.

## Help Message Generation

Optparse can generate formatted usage and option lists directly from your `optparse_def_t` array.

## API

### Functions

| Function | Description |
| --- | --- |
| `optparse_init(opts, argv)` | Initialize parser state. |
| `optparse_next(opts, defs, out_id)` | Parse the next short/long option; returns an error code (`OPTPARSE_ERROR_DONE` when finished). |
| `optparse_shift(opts)` | Pop the next positional argument and advance. |
| `optparse_usage(out, progname, defs, count, pos_args)` | Print a "Usage: ..." line. |
| `optparse_help(out, defs, count, cfg)` | Print a formatted options list. |
| `optparse_strerror(code)` | Return a human-readable message for an error code. |

### Option Descriptors

| Field | Description |
| --- | --- |
| `longname` | Long option name (without `--`); `NULL` for short-only. |
| `shortname` | Printable ASCII short option character; use a non-printable value (e.g. `128+`) for long-only options. |
| `argtype` | `OPTPARSE_NONE`, `OPTPARSE_REQUIRED`, or `OPTPARSE_OPTIONAL`. |
| `desc` | Help text; `NULL` hides the entry from help output. |
| `metavar` | Argument placeholder shown in help; defaults to `"ARG"`. |

### Struct Fields

After each call, you can read:

| Field | Description |
| --- | --- |
| `optind` | Index of next argv element |
| `optopt` | Shortname of the option just matched (0 for unknown long options) |
| `optarg` | Argument for current option (may be NULL) |
| `argv` | The original argument vector (useful for error reporting) |
| `subind` | Byte offset within a short-option cluster |

Set `permute` to 0 before parsing to stop at the first non-option (POSIX mode).

---

Forked from [skeeto/optparse](https://github.com/skeeto/optparse) with CMake support and additional improvements.
