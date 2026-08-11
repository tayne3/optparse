if(BUILD_SHARED_LIBS)
  set(SERIAL_LIB_TYPE SHARED)
else()
  set(SERIAL_LIB_TYPE STATIC)
endif()
message(STATUS "optparse v${PROJECT_VERSION} ${SERIAL_LIB_TYPE} library")

add_library(optparse ${OPTPARSE_LIB_TYPE} optparse.c)
add_library(optparse::optparse ALIAS optparse)
target_link_libraries(optparse PRIVATE
  $<BUILD_INTERFACE:optparse_internal_options>
)
target_include_directories(optparse PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(optparse
    PRIVATE
      OPTPARSE_LIB_EXPORT
    INTERFACE
      OPTPARSE_SHARED
  )
endif()
