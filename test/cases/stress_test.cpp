#include <iostream>
#include <string>
#include <vector>

#include "catch.hpp"

#define OPTPARSE_API static
#define OPTPARSE_IMPLEMENTATION
#include "optparse/optparse.h"

TEST_CASE("stress: recursion depth with permutation", "[stress]") {
    const int num_non_options = 50000;

    std::vector<std::string> args;
    args.push_back("prog");
    for (int i = 0; i < num_non_options; ++i) { args.push_back("arg" + std::to_string(i)); }
    args.push_back("-a");

    std::vector<char*> argv;
    for (const auto& s : args) { argv.push_back(const_cast<char*>(s.c_str())); }
    argv.push_back(nullptr);

    struct optparse options;
    optparse_init(&options, argv.data());
    int result = optparse(&options, "a");

    SECTION("check result") {
        REQUIRE(result == 'a');
        REQUIRE(options.optind == 2);  // 'prog' + '-a'
    }
}

TEST_CASE("stress: recursion depth with optparse_long", "[stress]") {
    const int num_non_options = 10000;

    std::vector<std::string> args;
    args.push_back("prog");
    for (int i = 0; i < num_non_options; ++i) { args.push_back("arg" + std::to_string(i)); }
    args.push_back("--test");

    std::vector<char*> argv;
    for (const auto& s : args) { argv.push_back(const_cast<char*>(s.c_str())); }
    argv.push_back(nullptr);

    const struct optparse_long longopts[] = {
        {"test", 't', OPTPARSE_NONE},
        {nullptr, 0, OPTPARSE_NONE},
    };

    struct optparse options;
    optparse_init(&options, argv.data());

    int longindex = -1;
    int result    = optparse_long(&options, longopts, &longindex);

    SECTION("check result") {
        REQUIRE(result == 't');
        REQUIRE(longindex == 0);
        REQUIRE(options.optind == 2);
    }
}
