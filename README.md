# optparse

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/optparse?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/optparse/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/optparse?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/optparse/tags)

**English** | [中文](README_zh.md)

A portable, reentrant, embeddable command-line option parser — the library `getopt()` should have been.

---

## Why optparse instead of getopt?

POSIX `getopt()` has three fundamental design flaws that make it unsuitable for modern code:

| Flaw                                        | getopt | optparse                                |
| ------------------------------------------- | ------ | --------------------------------------- |
| Global state — not thread-safe, no re-entry | ✗      | ✓ All state in a caller-supplied struct |
| No reliable reset / sub-command parsing     | ✗      | ✓ `optparse_init()` + `optparse_arg()`  |
| Errors printed to stderr, not accessible    | ✗      | ✓ Error stored in `options.errmsg`      |

If you've ever struggled with getopt's hidden globals, mysterious optind resets, or the inability to parse nested subcommands — optparse was built for you.

---

## Usage

### Basic Example

```c
#define OPTPARSE_IMPLEMENTATION
#include <optparse/optparse.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    struct optparse_long longopts[] = {
        {"verbose", 'v', OPTPARSE_NONE},
        {"output",  'o', OPTPARSE_REQUIRED},
        {"count",   'n', OPTPARSE_OPTIONAL},
        {NULL, 0, OPTPARSE_NONE},
    };

    struct optparse opts;
    optparse_init(&opts, argv);

    int verbose = 0, count = 1;
    const char *output = "out.txt";
    int c;

    while ((c = optparse_long(&opts, longopts, NULL)) != -1) {
        switch (c) {
        case 'v': verbose = 1; break;
        case 'o': output = opts.optarg; break;
        case 'n': count = opts.optarg ? atoi(opts.optarg) : 5; break;
        case '?':
            fprintf(stderr, "error: %s\n", opts.errmsg);
            return 1;
        }
    }

    char *file;
    while ((file = optparse_arg(&opts))) {
        printf("processing: %s\n", file);
    }
    return 0;
}
```

---

## API Reference

### Core Functions

| Function                                      | Description                                                                   |
| --------------------------------------------- | ----------------------------------------------------------------------------- |
| `optparse_init(options, argv)`                | Initialize parser state. Call once, or any time you need a full reset.        |
| `optparse(options, optstring)`                | Parse next short option. Returns option char, `-1` when done, `'?'` on error. |
| `optparse_long(options, longopts, longindex)` | Parse next option (short or long). `longindex` receives matched entry index.  |
| `optparse_arg(options)`                       | Return next non-option argument, or `NULL` when exhausted.                    |

### Option String Format

Follows `getopt()` conventions:

| Suffix   | Meaning           |
| -------- | ----------------- |
| _(none)_ | No argument       |
| `:`      | Required argument |
| `::`     | Optional argument |
