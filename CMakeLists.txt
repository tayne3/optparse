cmake_minimum_required(VERSION 3.14)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported. "
    "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' before rebuilding this project."
  )
endif()

project(optparse VERSION 0.2.0)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/optparse_compiler_options.cmake)

add_subdirectory(src)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/optparse_package_config.cmake)

if(OPTPARSE_BUILD_EXAMPLE)
  add_subdirectory(examples)
endif()

if(OPTPARSE_BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
