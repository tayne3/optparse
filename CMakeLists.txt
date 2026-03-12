cmake_minimum_required(VERSION 3.14)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported. "
    "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' before rebuilding this project."
  )
endif()

project(optparse VERSION 1.0.0)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(OPTPARSE_BUILD_EXAMPLE "build example program" ON)
  option(OPTPARSE_BUILD_TEST "build test program" ON)

  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT isMultiConfig
     AND NOT CMAKE_BUILD_TYPE
     AND NOT CMAKE_CONFIGURATION_TYPES
  )
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
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

include(GNUInstallDirs)
install(DIRECTORY include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS optparse
  EXPORT optparseTargets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT optparseTargets
  FILE optparseTargets.cmake
  NAMESPACE optparse::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/optparse
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/optparseConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/optparseConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/optparseConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/optparse
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/optparseConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/optparseConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/optparse
)

set(CPACK_PACKAGE_NAME "optparse")
set(CPACK_PACKAGE_VENDOR "tayne3")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A command line parser for C/C++")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
include(CPack)
