#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

#include "doctest.h"
#include "optparse/optparse.h"

using namespace optparse;

TEST_SUITE_BEGIN("opt");

TEST_CASE("C++: inheritance and constructors") {
    // Memory layout safety: size must be identical for array traversal
    static_assert(sizeof(Option) == sizeof(optparse_def_t), "Option size mismatch");

    const char* argv_raw[] = {"prog", "-a", "--delay", "500", "foo", nullptr};
    char**      argv       = const_cast<char**>(argv_raw);

    Parser parser(argv);

    // Using constructors for cleaner and safer initialization
    Option defs[] = {
        Option("amend", 'a', ArgType::None, "Amend record"),
        Option("delay", 'd', ArgType::Required, "Delay in ms", "MS"),
        Option()  // Sentinel (all zeros)
    };

    int id = 0;

    REQUIRE(parser.next(defs, &id) == Error::None);
    REQUIRE(id == 'a');
    REQUIRE(parser.arg() == nullptr);

    REQUIRE(parser.next(defs, &id) == Error::None);
    REQUIRE(id == 'd');
    REQUIRE(parser.arg() != nullptr);
    REQUIRE(std::string(parser.arg()) == "500");

    REQUIRE(parser.next(defs, &id) == Error::Done);

    char* pos = parser.shift();
    REQUIRE(pos != nullptr);
    REQUIRE(std::string(pos) == "foo");
    REQUIRE(parser.shift() == nullptr);
}

TEST_CASE("C++: Parser copy control") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_move_constructible<Parser>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<Parser>::value);
}

TEST_CASE("C++: error handling") {
    const char* argv_raw[] = {"prog", "-z", "--delay", nullptr};
    char**      argv       = const_cast<char**>(argv_raw);

    Parser parser(argv);
    Option defs[] = {Option("delay", 'd', ArgType::Required, "Delay"), Option()};

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

TEST_CASE("C++: boundary and edge cases") {
    SUBCASE("empty argv list (NULL pointer)") {
        char*  argv[] = {nullptr};
        Parser parser(argv);
        Option defs[] = {Option("test", 't', ArgType::None), Option()};
        REQUIRE(parser.next(defs) == Error::Done);
        REQUIRE(parser.shift() == nullptr);
    }

    SUBCASE("argv with only program name") {
        char*  argv[] = {const_cast<char*>("prog"), nullptr};
        Parser parser(argv);
        Option defs[] = {Option("test", 't', ArgType::None), Option()};
        REQUIRE(parser.next(defs) == Error::Done);
        REQUIRE(parser.shift() == nullptr);
    }

    SUBCASE("mixed short clusters and positionals") {
        const char* argv_raw[] = {"prog", "-abc", "pos1", "-d", "val", "pos2", nullptr};
        char**      argv       = const_cast<char**>(argv_raw);
        Parser      parser(argv);

        Option defs[] = {Option(nullptr, 'a', ArgType::None), Option(nullptr, 'b', ArgType::None),
                         Option(nullptr, 'c', ArgType::None), Option(nullptr, 'd', ArgType::Required), Option()};

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'b');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'c');
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'd');
        REQUIRE(std::string(parser.arg()) == "val");
        REQUIRE(parser.next(defs, &id) == Error::Done);

        REQUIRE(std::string(parser.shift()) == "pos1");
        REQUIRE(std::string(parser.shift()) == "pos2");
    }

    SUBCASE("permutation disabled (POSIX mode)") {
        const char* argv_raw[] = {"prog", "-a", "pos", "-b", nullptr};
        char**      argv       = const_cast<char**>(argv_raw);
        Parser      parser(argv);
        parser.set_permute(false);

        Option defs[] = {Option(nullptr, 'a', ArgType::None), Option(nullptr, 'b', ArgType::None), Option()};

        int id;
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'a');
        REQUIRE(parser.next(defs, &id) == Error::Done);  // Stops at "pos"
        REQUIRE(std::string(parser.shift()) == "pos");
        REQUIRE(parser.next(defs, &id) == Error::None);
        REQUIRE(id == 'b');
    }
}

TEST_CASE("C++: usage and help formatting") {
    Option defs[] = {Option("verbose", 'v', ArgType::None, "Enable verbose logging"),
                     Option("output", 'o', ArgType::Required, "Output file path", "FILE"), Option()};

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
