#include <cstdio>
#include <string>
#include <type_traits>

#include "doctest.h"
#include "optparse/optparse.h"

using namespace optparse;

TEST_SUITE_BEGIN("optparse");

TEST_CASE("inheritance and constructors") {
    static_assert(sizeof(Option) == sizeof(optparse_def_t), "Option size mismatch");
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_move_constructible<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<Parser>::value);

    const char* argv[] = {"prog", "-a", "--delay", "500", "foo", nullptr};
    Parser      parser(const_cast<char**>(argv));

    Option defs[] = {
        Option("amend", 'a', ArgType::None, "Amend record"),
        Option("delay", 'd', ArgType::Required, "Delay in ms", "MS"),
        Option(),
    };

    int id = 0;

    REQUIRE(parser.next(defs, &id) == Error::None);
    REQUIRE(id == 'a');
    REQUIRE(parser.optarg() == nullptr);

    REQUIRE(parser.next(defs, &id) == Error::None);
    REQUIRE(id == 'd');
    REQUIRE(parser.optarg() != nullptr);
    REQUIRE(std::string(parser.optarg()) == "500");

    REQUIRE(parser.next(defs, &id) == Error::Done);

    char* pos = parser.arg();
    REQUIRE(pos != nullptr);
    REQUIRE(std::string(pos) == "foo");
    REQUIRE(parser.arg() == nullptr);
}

TEST_CASE("error handling") {
    const char* argv[] = {"prog", "-z", "--delay", nullptr};
    Parser      parser(const_cast<char**>(argv));

    Option defs[] = {
        Option("delay", 'd', ArgType::Required, "Delay"),
        Option(),
    };

    // Invalid option
    REQUIRE(parser.next(defs) == Error::Invalid);
    REQUIRE(parser.optopt() == 'z');
    REQUIRE(std::string(Parser::strerror(Error::Invalid)) == "invalid option");

    // Missing argument
    REQUIRE(parser.next(defs) == Error::Missing);
    REQUIRE(parser.optopt() == 'd');
    REQUIRE(std::string(Parser::strerror(Error::Missing)) == "option requires an argument");

    REQUIRE(parser.next(defs) == Error::Done);
}

TEST_CASE("boundary and edge cases") {
    SUBCASE("empty argv list (NULL pointer)") {
        char*  argv[] = {nullptr};
        Parser parser(argv);
        Option defs[] = {
            Option("test", 't', ArgType::None),
            Option(),
        };
        REQUIRE(parser.next(defs) == Error::Done);
        REQUIRE(parser.arg() == nullptr);
    }

    SUBCASE("argv with only program name") {
        const char* argv[] = {"prog", nullptr};
        Parser      parser(const_cast<char**>(argv));
        Option      defs[] = {
            Option("test", 't', ArgType::None),
            Option(),
        };
        REQUIRE(parser.next(defs) == Error::Done);
        REQUIRE(parser.arg() == nullptr);
    }

    SUBCASE("mixed short clusters and positionals") {
        const char* argv[] = {"prog", "-abc", "pos1", "-d", "val", "pos2", nullptr};
        Parser      parser(const_cast<char**>(argv));

        Option defs[] = {
            Option(nullptr, 'a', ArgType::None),
            Option(nullptr, 'b', ArgType::None),
            Option(nullptr, 'c', ArgType::None),
            Option(nullptr, 'd', ArgType::Required),
            Option(),
        };

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'b');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'c');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'd');
        REQUIRE(std::string(parser.optarg()) == "val");
        REQUIRE(parser.next(defs, &id) == Error::Done);

        REQUIRE(std::string(parser.arg()) == "pos1");
        REQUIRE(std::string(parser.arg()) == "pos2");
    }

    SUBCASE("permutation disabled (POSIX mode)") {
        const char* argv[] = {"prog", "-a", "pos", "-b", nullptr};
        Parser      parser(const_cast<char**>(argv));
        parser.set_permute(false);

        Option defs[] = {
            Option(nullptr, 'a', ArgType::None),
            Option(nullptr, 'b', ArgType::None),
            Option(),
        };

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::Done);  // Stops at "pos"
        REQUIRE(std::string(parser.arg()) == "pos");
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'b');
    }
}

TEST_CASE("narg counting") {
    SUBCASE("counts remaining positionals after parsing") {
        const char* argv[] = {"prog", "-a", "foo", "bar", "baz", nullptr};
        Parser      parser(const_cast<char**>(argv));

        Option defs[] = {
            Option(nullptr, 'a', ArgType::None),
            Option(),
        };

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::Done);
        REQUIRE(parser.narg() == 3);

        REQUIRE(std::string(parser.arg()) == "foo");
        REQUIRE(parser.narg() == 2);
        REQUIRE(std::string(parser.arg()) == "bar");
        REQUIRE(std::string(parser.arg()) == "baz");
        REQUIRE(parser.narg() == 0);
        REQUIRE(parser.arg() == nullptr);
    }

    SUBCASE("zero when argv is empty") {
        char*  argv[] = {nullptr};
        Parser parser(argv);
        Option defs[] = {
            Option("test", 't', ArgType::None),
            Option(),
        };
        REQUIRE(parser.next(defs) == Error::Done);
        REQUIRE(parser.narg() == 0);
    }

    SUBCASE("before DONE may count unpermuted options") {
        const char* argv[] = {"prog", "foo", "-a", "bar", nullptr};
        Parser      parser(const_cast<char**>(argv));

        Option defs[] = {
            Option(nullptr, 'a', ArgType::None),
            Option(),
        };

        REQUIRE(parser.narg() == 3); /* includes the unpermuted -a */

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::Done);
        REQUIRE(parser.narg() == 2); /* foo, bar */
    }
}

TEST_CASE("usage and help formatting") {
    Option defs[] = {
        Option("verbose", 'v', ArgType::None, "Enable verbose logging"),
        Option("output", 'o', ArgType::Required, "Output file path", "FILE"),
        Option(),
    };

#ifdef _WIN32
    FILE* devnull = fopen("NUL", "w");
#else
    FILE* devnull = fopen("/dev/null", "w");
#endif
    if (devnull) {
        // These calls should not crash and should exercise the underlying C implementation
        Parser::usage(devnull, "testprog", defs, -1, "[ARGS]");
        Parser::help(devnull, defs);
        fclose(devnull);
    }
}

TEST_SUITE_END();
