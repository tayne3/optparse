if(MSVC)
  add_compile_options(Catch2 INTERFACE /utf-8)
endif()

add_executable(long long.c)
target_link_libraries(long PRIVATE optparse::optparse)

add_executable(short short.c)
target_link_libraries(short PRIVATE optparse::optparse)

add_executable(subcommands subcommands.c)
target_link_libraries(subcommands PRIVATE optparse::optparse)
