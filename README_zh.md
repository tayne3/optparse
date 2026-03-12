# optparse

![CMake](https://img.shields.io/badge/CMake-3.14%2B-brightgreen?logo=cmake&logoColor=white)
[![Release](https://img.shields.io/github/v/release/tayne3/optparse?include_prereleases&label=release&logo=github&logoColor=white)](https://github.com/tayne3/optparse/releases)
[![Tag](https://img.shields.io/github/v/tag/tayne3/optparse?color=%23ff8936&style=flat-square&logo=git&logoColor=white)](https://github.com/tayne3/optparse/tags)

[English](README.md) | **中文**

一个可移植、可重入、可嵌入的命令行选项解析器 —— `getopt()` 本该有的样子。

## 为什么不用 getopt？

POSIX getopt 存在三个致命缺陷：

1. **全局状态。** getopt 的解析状态完全存储在全局变量中，其中一些是静态的、无法访问的。这意味着同一时间只有一个线程能使用 getopt，递归解析嵌套子参数也不可能。Optparse 通过将所有状态存储在调用者提供的结构体中解决了这个问题。

2. **无法正确重置。** POSIX 标准没有提供正确重置解析器的方法。对于可移植代码，getopt 只能对一组 argv 运行一次。Optparse 提供 `optparse_arg()` 用于遍历非选项参数，解析可以随时用不同的选项字符串继续。完全重置只需再调用一次 `optparse_init()`。

3. **错误信息无法获取。** 在 getopt 中，错误信息被打印到 stderr。虽然可以用 opterr 禁用，但信息本身仍然无法获取。Optparse 将错误信息写入 `errmsg` 字段，由你决定是否打印、打印到哪里。

## 使用方法

### 集成

**CMake:**

```cmake
FetchContent_Declare(optparse GIT_REPOSITORY https://github.com/tayne3/optparse.git)
FetchContent_MakeAvailable(optparse)
target_link_libraries(my_app PRIVATE optparse::optparse)
```

**单头文件:**

将 `include/optparse/optparse.h` 复制到项目中。

### 配置

**OPTPARSE_IMPLEMENTATION** — 在一个源文件中定义后包含头文件以引入实现：

```c
#define OPTPARSE_IMPLEMENTATION
#include <optparse/optparse.h>
```

其他文件正常包含头文件即可获得声明。

**OPTPARSE_API** — 控制符号可见性与链接方式。默认为空（外部链接）。常用值：

```c
#define OPTPARSE_API static                  // 内部链接
#define OPTPARSE_API __declspec(dllexport)   // Windows DLL 导出
```

### 示例

```c
#include <stdio.h>
#include <stdlib.h>

#define OPTPARSE_IMPLEMENTATION
#include <optparse/optparse.h>

void out(const char *str, int len, void *userdata) {
    fwrite(str, 1, len, (FILE *)userdata);
}

int main(int argc, char **argv) {
    const optparse_long_t longopts[] = {
        {"help",   'h', OPTPARSE_NONE,     "show this help message"},
        {"amend",  'a', OPTPARSE_NONE,     "amend the commit"},
        {"color",  'c', OPTPARSE_REQUIRED, "set output color", "COLOR"},
        {"delay",  'd', OPTPARSE_OPTIONAL, "delay in seconds", "N"},
        {0},
    };

    optparse_t options;
    optparse_init(&options, argv);

    int option;
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'h':
            optparse_usage(out, stdout, argv[0], longopts, -1, "[OPERANDS]");
            printf("\nOptions:\n");
            optparse_help(out, stdout, longopts, -1, NULL);
            return 0;
        case 'c': printf("color: %s\n", options.optarg); break;
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

## 参数置换

默认情况下，argv 在解析时会被置换，非选项参数被移到末尾。初始化后将 `permute` 设为 0 可禁用此行为：

```c
optparse_t options;
optparse_init(&options, argv);
options.permute = 0;
```

## 直接替换

Optparse 的接口对熟悉 getopt 的人来说应该很亲切。选项字符串格式相同，解析器结构体字段名称也与 getopt 的全局变量一致（`optarg`、`optind`、`optopt`）。

`optparse_long()` API 类似 GNU 的 `getopt_long()`，可作为可移植的嵌入式替代品。

Optparse 不分配内存，也没有依赖 —— 甚至不依赖 libc。

## 子命令解析

解析子命令时，先禁用置换解析选项（子命令之前的"全局"选项），然后用新的选项字符串继续解析剩余部分。

完整示例见 [examples/subcommands.c](examples/subcommands.c)。

## 帮助信息生成

Optparse 可以根据 `optparse_long_t` 数组，直接生成排版好的用法说明和选项列表。

## API

### 函数

| 函数                  | 说明                                      |
| :-------------------- | :---------------------------------------- |
| `optparse_init(...)`  | 初始化解析器状态。                        |
| `optparse(...)`       | 解析下一个短选项（getopt 风格）。         |
| `optparse_long(...)`  | 解析下一个短/长选项（getopt_long 风格）。 |
| `optparse_arg(...)`   | 弹出下一个位置参数并前进。                |
| `optparse_usage(...)` | 通过回调生成 "Usage: ..." 行。            |
| `optparse_help(...)`  | 通过回调生成格式化的选项列表。            |

### 选项字符串

遵循 `getopt()` 约定：无冒号 = 无参数，一个冒号 = 必需参数，两个冒号 = 可选参数。

### 结构体字段

每次调用后可读取：

| 字段     | 说明                              |
| -------- | --------------------------------- |
| `optind` | 下一个 argv 元素的索引            |
| `optopt` | 刚解析的选项字符                  |
| `optarg` | 当前选项的参数（可能为 NULL）     |
| `errmsg` | 错误字符串（仅当返回 `?` 时非空） |

解析前将 `permute` 设为 0 可在第一个非选项处停止（POSIX 模式）。

---

衍生自 [skeeto/optparse](https://github.com/skeeto/optparse)，增加了 CMake 支持和改进。
