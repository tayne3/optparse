#[=======================================================================[.rst:
Project Configuration
---------------------

Establishes the core compiler environment, standards enforcement, and global
build policies for the project.
#]=======================================================================]

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(OPTPARSE_BUILD_EXAMPLE "build example program" ON)
  option(OPTPARSE_BUILD_TEST "build test program" ON)

  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_C_EXTENSIONS ON)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS ON)

  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT isMultiConfig
    AND NOT CMAKE_BUILD_TYPE
    AND NOT CMAKE_CONFIGURATION_TYPES
  )
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()

  if(NOT DEFINED CMAKE_C_VISIBILITY_PRESET)
    set(CMAKE_C_VISIBILITY_PRESET hidden CACHE STRING "Preset for the export of private symbols")
    set_property(CACHE CMAKE_C_VISIBILITY_PRESET PROPERTY STRINGS hidden default)
  endif()

  if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING "Preset for the export of private symbols")
    set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS hidden default)
  endif()

  if(NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "Whether to add a compile flag to hide symbols of inline functions")
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

  include(GNUInstallDirs)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
  endif()
endif()

add_library(optparse_internal_options INTERFACE)
target_compile_features(optparse_internal_options INTERFACE c_std_99)

if(WIN32)
  target_compile_definitions(optparse_internal_options INTERFACE
    # Suppress secure CRT warnings
    _CRT_SECURE_NO_WARNINGS
    # Suppress POSIX deprecation warnings
    _CRT_NONSTDC_NO_DEPRECATE
    # Suppress Winsock deprecation warnings
    _WINSOCK_DEPRECATED_NO_WARNINGS
    # Prevent min/max macro definitions
    NOMINMAX
  )
  # Force standard C99 printf/scanf over legacy MSVCRT
  if(MINGW)
    target_compile_definitions(optparse_internal_options INTERFACE __USE_MINGW_ANSI_STDIO=1)
  endif()
else()
  # Enable GNU extensions on Linux/Unix systems (covers glibc, musl, etc.)
  if((UNIX AND NOT APPLE) OR CMAKE_SYSTEM_NAME MATCHES "Linux|GNU")
    target_compile_definitions(optparse_internal_options INTERFACE
      _GNU_SOURCE=1
      _TIME_BITS=64
      _FILE_OFFSET_BITS=64
      _LARGEFILE_SOURCE
    )
  endif()
endif()

# In certain edge environments/older CMake versions,
# the MSVC variable alone fails to reliably identify clang-cl.
if(MSVC)
  set(OPTPARSE_IS_MSVC_LIKE TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  set(OPTPARSE_IS_MSVC_LIKE TRUE)
else()
  set(OPTPARSE_IS_MSVC_LIKE FALSE)
endif()

if(OPTPARSE_IS_MSVC_LIKE)
  target_compile_options(optparse_internal_options INTERFACE
    # Fix MSVC __cplusplus always reporting 199711L
    "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>"
    # UTF-8 source and execution character set
    "$<$<COMPILE_LANGUAGE:C>:/utf-8>"
    "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
    # unreferenced label
    "$<$<COMPILE_LANGUAGE:C>:/wd4102>"
    "$<$<COMPILE_LANGUAGE:CXX>:/wd4102>"
    # 64-bit to 32-bit conversion
    "$<$<COMPILE_LANGUAGE:C>:/wd4267>"
    "$<$<COMPILE_LANGUAGE:CXX>:/wd4267>"
    # Unicode character issues
    "$<$<COMPILE_LANGUAGE:C>:/wd4819>"
    "$<$<COMPILE_LANGUAGE:CXX>:/wd4819>"
    # deprecated function warnings
    "$<$<COMPILE_LANGUAGE:C>:/wd4996>"
    "$<$<COMPILE_LANGUAGE:CXX>:/wd4996>"
  )
else()
  target_compile_options(optparse_internal_options INTERFACE
    "$<$<COMPILE_LANGUAGE:C>:-Wall>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
    "$<$<COMPILE_LANGUAGE:C>:-Wextra>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
  )

  # Prevent local developer paths from leaking into compiled binaries.
  check_c_compiler_flag("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/" HAVE_MACRO_PREFIX_MAP)
  if(HAVE_MACRO_PREFIX_MAP)
    target_compile_options(optparse_internal_options INTERFACE
      "$<$<COMPILE_LANGUAGE:C>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/>"
      "$<$<COMPILE_LANGUAGE:CXX>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/>"
    )
  endif()

  # Probe once via the C driver: on any GCC/Clang toolchain the C and CXX
  # drivers share the same -W flag set, so a single check covers both.
  check_c_compiler_flag("-Werror=return-type" HAVE_WERROR_RETURN_TYPE)
  if(HAVE_WERROR_RETURN_TYPE)
    target_compile_options(optparse_internal_options INTERFACE
      "$<$<COMPILE_LANGUAGE:C>:-Werror=return-type>"
      "$<$<COMPILE_LANGUAGE:CXX>:-Werror=return-type>"
    )
  endif()

  # GCC < 6.0 incorrectly fires -Wmissing-field-initializers on valid C99
  # designated / partial initializers. The false positive was fixed in GCC 6.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.0")
    target_compile_options(optparse_internal_options INTERFACE
      "$<$<COMPILE_LANGUAGE:C>:-Wno-missing-field-initializers>"
      "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>"
    )
  endif()
endif()
