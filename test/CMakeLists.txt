file(GLOB_RECURSE SRCS_G CONFIGURE_DEPENDS "cases/*.cpp")
foreach(SRC ${SRCS_G})
  get_filename_component(TEST_NAME ${SRC} NAME_WE)
  add_executable(optparse_${TEST_NAME} ${SRC} main.cpp)
  add_test(NAME optparse_${TEST_NAME} COMMAND optparse_${TEST_NAME})
  target_link_libraries(optparse_${TEST_NAME} PRIVATE
    optparse_internal_options
    optparse::optparse
  )
  target_include_directories(optparse_${TEST_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/doctest
  )
endforeach()
