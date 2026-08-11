add_executable(optparse_basic basic.c)
target_link_libraries(optparse_basic PRIVATE
  optparse_internal_options
  optparse::optparse
)

add_executable(optparse_usage usage.c)
target_link_libraries(optparse_usage PRIVATE
optparse_internal_options
optparse::optparse
)

add_executable(optparse_subcommands subcommands.c)
target_link_libraries(optparse_subcommands PRIVATE
optparse_internal_options
optparse::optparse
)
