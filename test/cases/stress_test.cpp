#include <string>
#include <vector>

#include "doctest.h"
#include "optparse/optparse.h"

TEST_SUITE_BEGIN("optparse");

TEST_CASE("stress: recursion depth with permutation") {
    const int num_non_options = 50000;

    std::vector<std::string> args;
    args.push_back("prog");
    for (int i = 0; i < num_non_options; ++i) { args.push_back("arg" + std::to_string(i)); }
    args.push_back("-a");

    std::vector<char*> argv;
    for (const auto& s : args) { argv.push_back(const_cast<char*>(s.c_str())); }
    argv.push_back(nullptr);

    static const optparse_def_t defs[] = {
        {NULL, 'a', OPTPARSE_NONE, NULL, NULL},
        OPTPARSE_DEF_NULL,
        OPTPARSE_DEF_NULL,
    };

    optparse_t options;
    optparse_init(&options, argv.data());

    int              id;
    optparse_error_t result = optparse_next(&options, defs, &id);

    SUBCASE("check result") {
        REQUIRE(result == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 'a');
        REQUIRE(options.optind == 2);  // 'prog' + '-a'
    }
}

TEST_CASE("stress: recursion depth with next") {
    const int num_non_options = 10000;

    std::vector<std::string> args;
    args.push_back("prog");
    for (int i = 0; i < num_non_options; ++i) { args.push_back("arg" + std::to_string(i)); }
    args.push_back("--test");

    std::vector<char*> argv;
    for (const auto& s : args) { argv.push_back(const_cast<char*>(s.c_str())); }
    argv.push_back(nullptr);

    const optparse_def_t defs[] = {
        {"test", 't', OPTPARSE_NONE, NULL, NULL},
        OPTPARSE_DEF_NULL,
    };

    optparse_t options;
    optparse_init(&options, argv.data());

    int              id     = -1;
    optparse_error_t result = optparse_next(&options, defs, &id);

    SUBCASE("check result") {
        REQUIRE(result == OPTPARSE_ERROR_NONE);
        REQUIRE(id == 't');
        REQUIRE(options.optind == 2);
    }
}

TEST_SUITE_END();
