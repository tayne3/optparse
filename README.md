# optparse

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/optparse?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/optparse/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/optparse?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/optparse/tags)

**English** | [中文](README_zh.md)

A portable, reentrant, embeddable command-line option parser — the library `getopt()` should have been.

## Why not getopt?

The POSIX getopt option parser has three fatal flaws:

1. **Global state.** The getopt parser state is stored entirely in global variables, some of which are static and inaccessible. This means only one thread can use getopt at a time, and recursive parsing of nested sub-arguments is impossible. Optparse fixes this by storing all state in a caller-supplied struct.

2. **No proper reset.** The POSIX standard provides no way to properly reset the parser. For portable code, getopt is only good for one run over one argv. Optparse provides `optparse_arg()` for stepping through non-option arguments, and parsing can continue at any time with a different option string. A full reset is just another call to `optparse_init()`.

3. **Inaccessible errors.** In getopt, error messages are printed to stderr. This can be disabled with opterr, but the messages themselves are still inaccessible. Optparse writes error messages to the `errmsg` field, so you decide where (or if) to print them.

## Usage

### Integration

**CMake:**

```cmake
FetchContent_Declare(optparse GIT_REPOSITORY https://github.com/tayne3/optparse.git)
FetchContent_MakeAvailable(optparse)
target_link_libraries(my_app PRIVATE optparse::optparse)
```

**Single Header:**

Copy `include/optparse/optparse.h` to your project.

### Configuration

**OPTPARSE_IMPLEMENTATION** — Define in exactly one source file before inclusion to pull in the implementation:

```c
#define OPTPARSE_IMPLEMENTATION
#include <optparse/optparse.h>
```

Other files include the header normally to get declarations only.

**OPTPARSE_API** — Controls symbol visibility and linkage. Default is empty (external linkage). Common values:

```c
#define OPTPARSE_API static                  // internal linkage
#define OPTPARSE_API __declspec(dllexport)   // Windows DLL export
```

### Example

```c
#define OPTPARSE_IMPLEMENTATION
#include <optparse/optparse.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    optparse_long_t longopts[] = {
        {"amend",  'a', OPTPARSE_NONE},
        {"brief",  'b', OPTPARSE_NONE},
        {"color",  'c', OPTPARSE_REQUIRED},
        {"delay",  'd', OPTPARSE_OPTIONAL},
        {0, 0, OPTPARSE_NONE}
    };

    optparse_t options;
    optparse_init(&options, argv);

    int amend = 0, brief = 0, delay = 0;
    const char *color = "white";
    int option;

    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'a': amend = 1; break;
        case 'b': brief = 1; break;
        case 'c': color = options.optarg; break;
        case 'd': delay = options.optarg ? atoi(options.optarg) : 1; break;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
            return 1;
        }
    }

    char *arg;
    while ((arg = optparse_arg(&options))) {
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

Optparse's interface should be familiar to anyone accustomed to getopt. The option string has the same format, and the parser struct fields have the same names as the getopt global variables (`optarg`, `optind`, `optopt`).

The `optparse_long()` API is similar to GNU's `getopt_long()` and can serve as a portable, embedded replacement.

Optparse does not allocate memory and has no dependencies — not even libc.

## Subcommand Parsing

To parse subcommands, first parse options with permutation disabled (these are the "global" options before the subcommand), then parse the remainder with a fresh option string.

See [examples/subcommands.c](examples/subcommands.c) for a complete example.

## API

### Functions

| Function                                      | Description                                                                  |
| --------------------------------------------- | ---------------------------------------------------------------------------- |
| `optparse_init(options, argv)`                | Initialize parser state. Call once, or any time you need a full reset.       |
| `optparse(options, optstring)`                | Parse next short option. Returns option char, `-1` when done, `?` on error.  |
| `optparse_long(options, longopts, longindex)` | Parse next option (short or long). `longindex` receives matched entry index. |
| `optparse_arg(options)`                       | Return next non-option argument, or `NULL` when exhausted.                   |

### Option String

Follows `getopt()` conventions: no colon = no argument, one colon = required, two colons = optional.

### Struct Fields

After each call, you can read:

| Field    | Description                                     |
| -------- | ----------------------------------------------- |
| `optind` | Index of next argv element                      |
| `optopt` | The option character just parsed                |
| `optarg` | Argument for current option (may be NULL)       |
| `errmsg` | Error string (non-empty only when `?` returned) |

Set `permute` to 0 before parsing to stop at the first non-option (POSIX mode).

---

Forked from [skeeto/optparse](https://github.com/skeeto/optparse) with CMake support and additional improvements.
