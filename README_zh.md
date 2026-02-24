# optparse

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/optparse?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/optparse/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/optparse?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/optparse/tags)

[English](README.md) | **中文**

一个可移植、可重入、可嵌入的命令行选项解析器 —— `getopt()` 本该有的样子。

---

## 为什么选择 optparse 而不是 getopt？

POSIX `getopt()` 存在三个根本性的设计缺陷，使其不适合现代代码：

| 缺陷                             | getopt | optparse                               |
| -------------------------------- | ------ | -------------------------------------- |
| 全局状态 —— 非线程安全，不可重入 | ✗      | ✓ 所有状态存储在用户提供的结构体中     |
| 无法可靠重置 / 不支持子命令解析  | ✗      | ✓ `optparse_init()` + `optparse_arg()` |
| 错误输出到 stderr，无法获取      | ✗      | ✓ 错误信息存储在 `options.errmsg` 中   |

如果你曾被 getopt 的隐藏全局变量、莫名其妙的 optind 重置、或无法解析嵌套子命令所困扰 —— optparse 就是为解决这些问题而生的。

---

## 使用方法

### 基本示例

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
            fprintf(stderr, "错误: %s\n", opts.errmsg);
            return 1;
        }
    }

    char *file;
    while ((file = optparse_arg(&opts))) {
        printf("处理文件: %s\n", file);
    }
    return 0;
}
```

---

## API 参考

### 核心函数

| 函数                                          | 说明                                                                |
| --------------------------------------------- | ------------------------------------------------------------------- |
| `optparse_init(options, argv)`                | 初始化解析器状态。调用一次，或需要完全重置时调用。                  |
| `optparse(options, optstring)`                | 解析下一个短选项。返回选项字符，完成时返回 `-1`，错误时返回 `'?'`。 |
| `optparse_long(options, longopts, longindex)` | 解析下一个选项（短选项或长选项）。`longindex` 接收匹配项的索引。    |
| `optparse_arg(options)`                       | 返回下一个非选项参数，耗尽时返回 `NULL`。                           |

### 选项字符串格式

遵循 `getopt()` 约定：

| 后缀   | 含义     |
| ------ | -------- |
| _(无)_ | 无参数   |
| `:`    | 必需参数 |
| `::`   | 可选参数 |
