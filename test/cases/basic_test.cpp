#include <string>
#include <vector>

#include "doctest.h"
#include "optparse/optparse.h"

namespace {

class Argv {
public:
    explicit Argv(std::initializer_list<const char*> args) {
        _ss.push_back(const_cast<char*>("prog"));
        for (auto s : args) { _ss.push_back(const_cast<char*>(s)); }
        _ss.push_back(nullptr);
    }

    char** data() { return _ss.data(); }

    optparse_t to_opts() {
        optparse_t o;
        optparse_init(&o, data());
        return o;
    }

private:
    std::vector<char*> _ss;
};

std::vector<std::string> unconsumed_args(optparse_t* o) {
    std::vector<std::string> v;
    for (char* a = optparse_shift(o); a != nullptr; a = optparse_shift(o)) { v.push_back(a); }
    return v;
}

/** Standard definition table used by most short-option tests. */
const optparse_def_t kShortDefs[] = {
    {nullptr, 'a', OPTPARSE_NONE, NULL, NULL},     {nullptr, 'b', OPTPARSE_NONE, NULL, NULL},
    {nullptr, 'c', OPTPARSE_REQUIRED, NULL, NULL}, {nullptr, 'd', OPTPARSE_OPTIONAL, NULL, NULL},
    {nullptr, 'e', OPTPARSE_NONE, NULL, NULL},     OPTPARSE_DEF_NULL,
};

/** Standard definition table used by most long-option tests. */
const optparse_def_t kDefs[] = {
    {"amend", 'a', OPTPARSE_NONE, NULL, NULL},
    {"brief", 'b', OPTPARSE_NONE, NULL, NULL},
    {"color", 'c', OPTPARSE_OPTIONAL, NULL, NULL},
    {"delay", 'd', OPTPARSE_REQUIRED, NULL, NULL},
    {"erase", 'e', OPTPARSE_NONE, NULL, NULL},
    {"file", 'f', OPTPARSE_REQUIRED, NULL, NULL},
    OPTPARSE_DEF_NULL,
};

}  // namespace

TEST_SUITE_BEGIN("opt");

TEST_CASE("short options") {
    int id = -1;

    SUBCASE("no arguments") {
        Argv av{};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("single flag") {
        Argv av{"-a"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("multiple flags") {
        Argv av{"-a", "-b", "-e"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'e');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("combined cluster -abe") {
        Argv av{"-abe"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'e');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("required argument — separate token") {
        Argv av{"-c", "red"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'c');
        REQUIRE(o.optarg != nullptr);
        REQUIRE(std::string(o.optarg) == "red");
    }

    SUBCASE("required argument — inline (no space)") {
        Argv av{"-cred"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'c');
        REQUIRE(std::string(o.optarg) == "red");
    }

    SUBCASE("required argument — combined cluster with arg") {
        Argv                        av{"-abeblue"};
        auto                        o      = av.to_opts();
        static const optparse_def_t defs[] = {
            {nullptr, 'a', OPTPARSE_NONE, NULL, NULL},
            {nullptr, 'b', OPTPARSE_NONE, NULL, NULL},
            {nullptr, 'e', OPTPARSE_REQUIRED, NULL, NULL},
            OPTPARSE_DEF_NULL,
        };
        REQUIRE(optparse_next(&o, defs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, defs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, defs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'e');
        REQUIRE(std::string(o.optarg) == "blue");
    }

    SUBCASE("optional argument — present inline") {
        Argv av{"-d10"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'd');
        REQUIRE(o.optarg != nullptr);
        REQUIRE(std::string(o.optarg) == "10");
    }

    SUBCASE("optional argument — absent") {
        Argv av{"-d", "10"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'd');
        REQUIRE(o.optarg == nullptr);
        /* "10" becomes a positional argument */
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"10"};
        REQUIRE(args == expected);
    }

    SUBCASE("unknown option returns ERR_UNKNOWN") {
        Argv av{"-z"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_INVALID);
        REQUIRE(o.optopt == 'z');
    }

    SUBCASE("invalid option in cluster discards the rest") {
        Argv av{"-azb"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_INVALID);
        REQUIRE(o.optopt == 'z');
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("missing required argument returns ERR_MISSING") {
        Argv av{"-c"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_MISSING);
        REQUIRE(o.optopt == 'c');
    }

    SUBCASE("repeated flag increments count") {
        Argv av{"-eeeeee"};
        auto o     = av.to_opts();
        int  count = 0;
        while (optparse_next(&o, kShortDefs, &id) == OPTPARSE_ERROR_NONE) {
            REQUIRE(id == 'e');
            ++count;
        }
        REQUIRE(count == 6);
    }
}

TEST_CASE("long options") {
    int id = -1;

    SUBCASE("single flag --amend") {
        Argv av{"--amend"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
    }

    SUBCASE("multiple flags") {
        Argv av{"--amend", "--brief"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("required argument — separate token") {
        Argv av{"--delay", "500"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'd');
        REQUIRE(std::string(o.optarg) == "500");
    }

    SUBCASE("required argument — inline with '='") {
        Argv av{"--color=red"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'c');
        REQUIRE(std::string(o.optarg) == "red");
    }

    SUBCASE("optional argument — present inline") {
        Argv av{"--color=blue"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'c');
        REQUIRE(std::string(o.optarg) == "blue");
    }

    SUBCASE("optional argument — absent") {
        Argv av{"--color"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'c');
        REQUIRE(o.optarg == nullptr);
    }

    SUBCASE("required argument missing returns ERR_MISSING") {
        Argv av{"--delay"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_MISSING);
        REQUIRE(o.optopt == 'd');
    }

    SUBCASE("unknown option returns ERR_UNKNOWN") {
        Argv av{"--foo"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_INVALID);
        REQUIRE(o.optopt == 0);  // Unknown long opts set optopt to 0
    }

    SUBCASE("TOOMANY when flag given an argument") {
        Argv av{"--amend=yes"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_TOOMANY);
        REQUIRE(o.optopt == 'a');
    }

    SUBCASE("long-only option (shortname > 127)") {
        static const optparse_def_t lo[] = {
            {"verbose", 256, OPTPARSE_NONE, NULL, NULL},
            {"output", 257, OPTPARSE_REQUIRED, NULL, NULL},
            {nullptr, 0, OPTPARSE_NONE, NULL, NULL},
        };
        Argv av{"--verbose", "--output", "file.txt"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, lo, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 256);

        REQUIRE(optparse_next(&o, lo, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 257);
        REQUIRE(std::string(o.optarg) == "file.txt");
    }

    SUBCASE("mix of short and long options") {
        Argv        av{"-a", "--brief", "--color=green", "--delay", "42"};
        auto        o     = av.to_opts();
        bool        amend = false, brief = false;
        std::string color;
        int         delay = 0;
        while (optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE) {
            switch (id) {
                case 'a': amend = true; break;
                case 'b': brief = true; break;
                case 'c': color = o.optarg ? o.optarg : ""; break;
                case 'd': delay = std::atoi(o.optarg); break;
                default: FAIL("unexpected option");
            }
        }
        REQUIRE(amend);
        REQUIRE(brief);
        REQUIRE(color == "green");
        REQUIRE(delay == 42);
    }
}

TEST_CASE("permute options") {
    int id = -1;

    SUBCASE("non-option before option") {
        Argv av{"foo", "--amend", "bar"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foo", "bar"};
        REQUIRE(args == expected);
    }

    SUBCASE("options interspersed with positionals") {
        Argv        av{"foo", "--delay", "1234", "bar", "-cred"};
        auto        o = av.to_opts();
        std::string color;
        int         delay = 0;
        while (optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE) {
            switch (id) {
                case 'c': color = o.optarg ? o.optarg : ""; break;
                case 'd': delay = std::atoi(o.optarg); break;
            }
        }
        REQUIRE(color == "red");
        REQUIRE(delay == 1234);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foo", "bar"};
        REQUIRE(args == expected);
    }

    SUBCASE("all positionals, no options") {
        Argv av{"foo", "bar", "baz"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foo", "bar", "baz"};
        REQUIRE(args == expected);
    }
}

TEST_CASE("posix: stop at first non-option") {
    Argv av{"-a", "stop", "-b"};
    auto o    = av.to_opts();
    o.permute = 0;
    int id;
    REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
    REQUIRE(id == 'a');
    REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
    auto args = unconsumed_args(&o);
    REQUIRE(args.size() >= 1);
    REQUIRE(args[0] == "stop");
}

TEST_CASE("arg options") {
    int id = -1;

    SUBCASE("basic positional collection") {
        Argv av{"-a", "foo", "bar"};
        auto o = av.to_opts();
        optparse_next(&o, kDefs, &id); /* consume -a */
        optparse_next(&o, kDefs, &id); /* returns DONE */
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foo", "bar"};
        REQUIRE(args == expected);
    }

    SUBCASE("step over subcommand and re-parse") {
        Argv av{"-a", "subcmd", "-b"};
        auto o    = av.to_opts();
        o.permute = 0;
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);

        char* subcmd = optparse_shift(&o);
        REQUIRE(subcmd != nullptr);
        REQUIRE(std::string(subcmd) == "subcmd");

        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("returns NULL when exhausted") {
        Argv av{};
        auto o = av.to_opts();
        REQUIRE(optparse_shift(&o) == nullptr);
    }
}

TEST_CASE("edge options") {
    int id = -1;

    SUBCASE("double-dash '--' terminates option parsing") {
        Argv av{"--", "foobar"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foobar"};
        REQUIRE(args == expected);
    }

    SUBCASE("single dash '-' is treated as positional") {
        Argv av{"-"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"-"};
        REQUIRE(args == expected);
    }

    SUBCASE("re-initialise resets state") {
        Argv av{"-a", "-b"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        optparse_init(&o, av.data());
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'b');
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
    }

    SUBCASE("id pointer nullptr does not crash") {
        Argv av{"--amend"};
        auto o = av.to_opts();
        REQUIRE(optparse_next(&o, kDefs, nullptr) == OPTPARSE_ERROR_NONE);
        REQUIRE(o.optopt == 'a');  // State still records it
    }
}

struct Config {
    bool             amend = false;
    bool             brief = false;
    std::string      color;
    bool             set_color = false;
    int              delay     = 0;
    int              erase     = 0;
    optparse_error_t err       = OPTPARSE_ERROR_NONE;
};

static Config run_long(char** argv_raw, const optparse_def_t* lo = kDefs) {
    Config     cfg;
    optparse_t o;
    optparse_init(&o, argv_raw);

    int              id;
    optparse_error_t st;
    while ((st = optparse_next(&o, lo, &id)) == OPTPARSE_ERROR_NONE) {
        switch (id) {
            case 'a': cfg.amend = true; break;
            case 'b': cfg.brief = true; break;
            case 'c':
                cfg.set_color = true;
                cfg.color     = o.optarg ? o.optarg : "";
                break;
            case 'd': cfg.delay = std::atoi(o.optarg); break;
            case 'e': cfg.erase++; break;
        }
    }
    if (st != OPTPARSE_ERROR_DONE) cfg.err = st;
    return cfg;
}

TEST_CASE("regression options") {
    int id = -1;

    SUBCASE("-- foobar") {
        Argv av{"--", "foobar"};
        auto cfg = run_long(av.data());
        auto o   = av.to_opts();
        while (optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE) {}
        auto args = unconsumed_args(&o);
        REQUIRE_FALSE(cfg.amend);
        REQUIRE_FALSE(cfg.brief);
        auto expected = std::vector<std::string>{"foobar"};
        REQUIRE(args == expected);
    }

    SUBCASE("-a -b -c -d 10 -e") {
        Argv av{"-a", "-b", "-c", "-d", "10", "-e"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.amend);
        REQUIRE(cfg.brief);
        REQUIRE(cfg.set_color);
        REQUIRE(cfg.color.empty());
        REQUIRE(cfg.delay == 10);
        REQUIRE(cfg.erase == 1);
        REQUIRE(cfg.err == OPTPARSE_ERROR_NONE);
    }

    SUBCASE("--amend --brief --color --delay 10 --erase") {
        Argv av{"--amend", "--brief", "--color", "--delay", "10", "--erase"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.amend);
        REQUIRE(cfg.brief);
        REQUIRE(cfg.set_color);
        REQUIRE(cfg.color.empty());
        REQUIRE(cfg.delay == 10);
        REQUIRE(cfg.erase == 1);
    }

    SUBCASE("-a -b -cred -d 10 -e") {
        Argv av{"-a", "-b", "-cred", "-d", "10", "-e"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.amend);
        REQUIRE(cfg.brief);
        REQUIRE(cfg.color == "red");
        REQUIRE(cfg.delay == 10);
        REQUIRE(cfg.erase == 1);
    }

    SUBCASE("-eeeeee increments to 6") {
        Argv av{"-eeeeee"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.erase == 6);
    }

    SUBCASE("--delay (missing arg) gives MISSING error") {
        Argv av{"--delay"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.err == OPTPARSE_ERROR_MISSING);
    }

    SUBCASE("--foo bar leaves foo and bar as positionals") {
        Argv av{"--foo", "bar"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.err == OPTPARSE_ERROR_INVALID);
    }

    SUBCASE("-x leaves -x as positional") {
        Argv av{"-x"};
        auto cfg = run_long(av.data());
        REQUIRE(cfg.err == OPTPARSE_ERROR_INVALID);
    }

    SUBCASE("- is positional") {
        Argv       av{"-"};
        optparse_t o;
        optparse_init(&o, av.data());
        REQUIRE(optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_DONE);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"-"};
        REQUIRE(args == expected);
    }

    SUBCASE("-e foo bar baz -a quux") {
        Argv       av{"-e", "foo", "bar", "baz", "-a", "quux"};
        optparse_t o;
        optparse_init(&o, av.data());
        Config cfg;
        while (optparse_next(&o, kDefs, &id) == OPTPARSE_ERROR_NONE) {
            switch (id) {
                case 'a': cfg.amend = true; break;
                case 'e': cfg.erase++; break;
            }
        }
        REQUIRE(cfg.amend);
        REQUIRE(cfg.erase == 1);
        auto args     = unconsumed_args(&o);
        auto expected = std::vector<std::string>{"foo", "bar", "baz", "quux"};
        REQUIRE(args == expected);
    }
}

TEST_SUITE_END();
