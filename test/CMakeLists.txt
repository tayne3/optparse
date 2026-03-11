if(MSVC)
  add_compile_options(/utf-8)
endif()

add_executable(optparse_test main.cpp)
add_test(NAME optparse_test COMMAND optparse_test)
target_include_directories(optparse_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/catch)
target_link_libraries(optparse_test PRIVATE optparse::optparse)
