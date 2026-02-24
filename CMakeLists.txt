cmake_minimum_required(VERSION 3.14)

project(optparse VERSION 1.0.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# This variable is set by project() in CMake 3.21+
if(NOT DEFINED PROJECT_IS_TOP_LEVEL)
  string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)
endif()

if(PROJECT_IS_TOP_LEVEL)
  option(OPTPARSE_BUILD_EXAMPLE "build example program" ON)
  option(OPTPARSE_BUILD_TEST "build test program" ON)
endif()

add_library(optparse INTERFACE)
add_library(optparse::optparse ALIAS optparse)
target_include_directories(optparse INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(OPTPARSE_BUILD_EXAMPLE)
  add_subdirectory(examples)
endif()

if(OPTPARSE_BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
