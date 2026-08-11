#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "doctest.h"
#include "optparse/optparse.h"

namespace {

struct CaptureFile {
    CaptureFile() : fp_(std::tmpfile()) {
        if (!fp_) throw std::runtime_error("tmpfile() failed");
    }
    ~CaptureFile() {
        if (fp_) std::fclose(fp_);
    }
    CaptureFile(const CaptureFile&)            = delete;
    CaptureFile& operator=(const CaptureFile&) = delete;
    CaptureFile(CaptureFile&&)                 = delete;
    CaptureFile& operator=(CaptureFile&&)      = delete;

    FILE* fp() const { return fp_; }

    std::string read() {
        std::fflush(fp_);
        std::rewind(fp_);
        std::string result;
        char        buf[256];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), fp_)) > 0) result.append(buf, n);
        return result;
    }

private:
    FILE* fp_;
};

std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> lines;

    std::size_t pos = 0;
    while (pos <= s.size()) {
        std::size_t eol = s.find('\n', pos);
        if (eol == std::string::npos) {
            if (pos < s.size()) lines.push_back(s.substr(pos));
            break;
        }
        lines.push_back(s.substr(pos, eol - pos));
        pos = eol + 1;
    }
    return lines;
}

int find_col(const std::string& output, const std::string& needle) {
    for (auto& line : split_lines(output)) {
        auto pos = line.find(needle);
        if (pos != std::string::npos) { return static_cast<int>(pos); }
    }
    return -1;
}

bool all_lines_within(const std::string& output, int max_len) {
    for (auto& line : split_lines(output)) {
        if (static_cast<int>(line.size()) > max_len) { return false; }
    }
    return true;
}

std::string line_containing(const std::string& output, const std::string& needle) {
    for (auto& line : split_lines(output))
        if (line.find(needle) != std::string::npos) return line;
    return "";
}

const optparse_def_t kTypical[] = {
    {"help", 'h', OPTPARSE_NONE, "Show this help message", NULL},
    {"output", 'o', OPTPARSE_REQUIRED, "Write output to FILE", NULL},
    {"verbose", 'v', OPTPARSE_NONE, "Enable verbose mode", NULL},
    {"config", 'c', OPTPARSE_OPTIONAL, "Load config from FILE", NULL},
    OPTPARSE_DEF_NULL,
};

optparse_help_config_t fixed_cfg(int width = 80, int min_desc = 26, int max_left = 36) {
    optparse_help_config_t c;
    c.width    = width;
    c.min_desc = min_desc;
    c.max_left = max_left;
    return c;
}

}  // namespace

TEST_SUITE_BEGIN("opt");

TEST_CASE("help / basic rendering") {
    CaptureFile            cap;
    optparse_help_config_t cfg = fixed_cfg();
    optparse_help(cap.fp(), kTypical, -1, &cfg);
    const std::string out = cap.read();
    INFO(out);

    SUBCASE("long names appear") {
        REQUIRE(out.find("--help") != std::string::npos);
        REQUIRE(out.find("--output") != std::string::npos);
        REQUIRE(out.find("--verbose") != std::string::npos);
        REQUIRE(out.find("--config") != std::string::npos);
    }

    SUBCASE("short names appear") {
        REQUIRE(out.find("-h") != std::string::npos);
        REQUIRE(out.find("-o") != std::string::npos);
        REQUIRE(out.find("-v") != std::string::npos);
        REQUIRE(out.find("-c") != std::string::npos);
    }

    SUBCASE("argtype suffixes are correct") {
        REQUIRE(out.find("--output=ARG") != std::string::npos);
        REQUIRE(out.find("--config[=ARG]") != std::string::npos);
        REQUIRE(out.find("--help=") == std::string::npos);
        REQUIRE(out.find("--verbose=") == std::string::npos);
    }

    SUBCASE("description text appears") {
        REQUIRE(out.find("Show this help message") != std::string::npos);
        REQUIRE(out.find("Write output to FILE") != std::string::npos);
        REQUIRE(out.find("Enable verbose mode") != std::string::npos);
        REQUIRE(out.find("Load config from FILE") != std::string::npos);
    }

    SUBCASE("output is non-empty and multi-line") {
        REQUIRE(!out.empty());
        int newlines = 0;
        for (char c : out) {
            if (c == '\n') ++newlines;
        }
        REQUIRE(newlines >= 4);
    }
}

TEST_CASE("help / visibility") {
    SUBCASE("NULL description hides option") {
        static const optparse_def_t opts[] = {
            {"visible", 'v', OPTPARSE_NONE, "I am visible", NULL},
            {"hidden", 'x', OPTPARSE_NONE, NULL, NULL},
            OPTPARSE_DEF_NULL,
        };
        CaptureFile            cap;
        optparse_help_config_t cfg = fixed_cfg();
        optparse_help(cap.fp(), opts, -1, &cfg);
        std::string out = cap.read();
        INFO(out);

        REQUIRE(out.find("--visible") != std::string::npos);
        REQUIRE(out.find("--hidden") == std::string::npos);
        REQUIRE(out.find("-x") == std::string::npos);
    }

    SUBCASE("empty description also hides option") {
        static const optparse_def_t opts[] = {
            {"shown", 's', OPTPARSE_NONE, "Shown", NULL},
            {"silent", 'q', OPTPARSE_NONE, "", NULL},
            OPTPARSE_DEF_NULL,
        };
        CaptureFile            cap;
        optparse_help_config_t cfg = fixed_cfg();
        optparse_help(cap.fp(), opts, -1, &cfg);
        std::string out = cap.read();
        INFO(out);

        REQUIRE(out.find("--shown") != std::string::npos);
        REQUIRE(out.find("--silent") == std::string::npos);
    }

    SUBCASE("all-hidden produces empty output") {
        static const optparse_def_t opts[] = {
            {"a", 'a', OPTPARSE_NONE, NULL, NULL},
            {"b", 'b', OPTPARSE_NONE, NULL, NULL},
            OPTPARSE_DEF_NULL,
        };
        CaptureFile            cap;
        optparse_help_config_t cfg = fixed_cfg();
        optparse_help(cap.fp(), opts, -1, &cfg);
        REQUIRE(cap.read().empty());
    }
}

TEST_CASE("help / column alignment") {
    CaptureFile            cap;
    optparse_help_config_t cfg = fixed_cfg();
    optparse_help(cap.fp(), kTypical, -1, &cfg);
    const std::string out = cap.read();
    INFO(out);

    SUBCASE("all description texts start at the same column") {
        int col_h = find_col(out, "Show this help message");
        int col_o = find_col(out, "Write output to FILE");
        int col_v = find_col(out, "Enable verbose mode");
        int col_c = find_col(out, "Load config from FILE");

        REQUIRE(col_h > 0);
        REQUIRE(col_o == col_h);
        REQUIRE(col_v == col_h);
        REQUIRE(col_c == col_h);
    }

    SUBCASE("long-only option does not have a short-name prefix") {
        static const optparse_def_t opts[] = {
            {"long-only", 0, OPTPARSE_NONE, "No short form", NULL},
            {"normal", 'n', OPTPARSE_NONE, "Has short form", NULL},
            OPTPARSE_DEF_NULL,
        };
        CaptureFile            cap2;
        optparse_help_config_t cfg2 = fixed_cfg();
        optparse_help(cap2.fp(), opts, -1, &cfg2);
        std::string out2 = cap2.read();
        INFO(out2);

        std::string lo_line = line_containing(out2, "--long-only");
        REQUIRE(lo_line.find("-, ") == std::string::npos);
        int col_lo = find_col(out2, "No short form");
        int col_n  = find_col(out2, "Has short form");
        REQUIRE(col_lo == col_n);
    }
}

TEST_CASE("help / line width constraint") {
    static const optparse_def_t opts[] = {
        {"output", 'o', OPTPARSE_REQUIRED,
         "Write output to this file; if the path contains spaces it must be "
         "quoted. Supports relative and absolute paths on all platforms.",
         NULL},
        {"verbose", 'v', OPTPARSE_NONE, "Enable verbose mode and diagnostics", NULL},
        OPTPARSE_DEF_NULL,
    };

    const int width = GENERATE(50, 60, 80, 100, 120);

    CaptureFile            cap;
    optparse_help_config_t cfg = fixed_cfg(width);
    optparse_help(cap.fp(), opts, -1, &cfg);
    std::string out = cap.read();
    INFO("width=" << width);
    INFO(out);

    SUBCASE("no line exceeds the configured width") {
        REQUIRE(all_lines_within(out, width));
    }
}

TEST_CASE("help / custom metavar") {
    static const optparse_def_t opts[] = {
        {"output", 'o', OPTPARSE_REQUIRED, "Write to FILE", "FILE"},
        {"config", 'c', OPTPARSE_OPTIONAL, "Use CONFIG", "CONFIG"},
        OPTPARSE_DEF_NULL,
    };
    CaptureFile            cap;
    optparse_help_config_t cfg = fixed_cfg();
    optparse_help(cap.fp(), opts, -1, &cfg);
    std::string out = cap.read();
    INFO(out);

    REQUIRE(out.find("--output=FILE") != std::string::npos);
    REQUIRE(out.find("--config[=CONFIG]") != std::string::npos);
    REQUIRE(out.find("=ARG") == std::string::npos);
}

TEST_CASE("usage / basic usage") {
    CaptureFile cap;
    optparse_usage(cap.fp(), "testapp", NULL, -1, NULL);
    std::string out = cap.read();
    INFO(out);
    REQUIRE(out == "Usage: testapp\n");
}

TEST_CASE("usage / with options") {
    static const optparse_def_t opts[] = {
        {"help", 'h', OPTPARSE_NONE, "Help", NULL},
        OPTPARSE_DEF_NULL,
    };
    CaptureFile cap;
    optparse_usage(cap.fp(), "testapp", opts, -1, NULL);
    std::string out = cap.read();
    INFO(out);
    REQUIRE(out == "Usage: testapp [options]\n");
}

TEST_CASE("long / short option parsing without optstring") {
    static const optparse_def_t opts[] = {
        {"verbose", 'v', OPTPARSE_NONE, "Verbose", NULL},
        {"output", 'o', OPTPARSE_REQUIRED, "Output", NULL},
        OPTPARSE_DEF_NULL,
    };

    SUBCASE("simple short option") {
        char*      argv[] = {(char*)"app", (char*)"-v", NULL};
        optparse_t options;
        optparse_init(&options, argv);
        int              id;
        optparse_error_t r = optparse_next(&options, opts, &id);
        REQUIRE(r == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'v');
    }

    SUBCASE("short option with required argument") {
        char*      argv[] = {(char*)"app", (char*)"-o", (char*)"file.txt", NULL};
        optparse_t options;
        optparse_init(&options, argv);
        int              id;
        optparse_error_t r = optparse_next(&options, opts, &id);
        REQUIRE(r == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'o');
        REQUIRE(std::string(options.optarg) == "file.txt");
    }

    SUBCASE("invalid short option") {
        char*      argv[] = {(char*)"app", (char*)"-x", NULL};
        optparse_t options;
        optparse_init(&options, argv);
        int              id;
        optparse_error_t r = optparse_next(&options, opts, &id);
        REQUIRE(r == OPTPARSE_ERROR_INVALID);
        REQUIRE(std::string(optparse_strerror(r)).find("invalid") != std::string::npos);
        REQUIRE(options.optopt == 'x');
    }
}

TEST_CASE("help / line wrap threshold") {
    static const optparse_def_t opts[] = {
        {"abc", 'a', OPTPARSE_NONE, "Desc A", NULL},    // help_width=11, lw=13
        {"abcd", 'b', OPTPARSE_NONE, "Desc B", NULL},   // help_width=12, lw=14
        {"abcde", 'c', OPTPARSE_NONE, "Desc C", NULL},  // help_width=13, lw=15
        OPTPARSE_DEF_NULL,
    };

    CaptureFile cap;
    // width=80, min_desc=26, max_left=14
    optparse_help_config_t cfg = fixed_cfg(80, 26, 14);
    optparse_help(cap.fp(), opts, -1, &cfg);
    std::string out = cap.read();
    INFO(out);

    REQUIRE(out.find("  -a, --abc   Desc A") != std::string::npos);                 // gap is 3
    REQUIRE(out.find("  -b, --abcd  Desc B") != std::string::npos);                 // gap is 2
    REQUIRE(out.find("  -c, --abcde\n              Desc C") != std::string::npos);  // wrapped with 14 spaces
}

TEST_SUITE_END();
