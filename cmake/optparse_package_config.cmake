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
