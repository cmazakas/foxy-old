cmake_minimum_required(VERSION 3.11)

project(foxy VERSION 1.0.0)

find_path(CATCH_INCLUDE_DIR "catch.hpp")

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

find_package(
  Boost 1.67
  REQUIRED
  COMPONENTS system coroutine context thread container
)

add_library(
  libfoxy

  STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session.cpp
)

set_target_properties(
  libfoxy

  PROPERTIES
    OUTPUT_NAME "foxy"
)

if (MSVC)
  # Win10
  target_compile_definitions(libfoxy PUBLIC _WIN32_WINNT=0x0A00)
endif()

target_compile_definitions(
  libfoxy

  PUBLIC
  BOOST_COROUTINES_NO_DEPRECATION_WARNING=1
  BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1
  _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
)

target_compile_features(libfoxy PUBLIC cxx_std_17)

target_include_directories(
  libfoxy

  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  libfoxy

  PUBLIC
  Boost::boost
  Boost::coroutine
  Boost::system
  Boost::container
)

if (MSVC)
  target_compile_options(libfoxy PUBLIC "/await")

  # keep this around, just in case
  target_link_libraries(libfoxy PUBLIC Boost::disable_autolinking)
endif()

add_library(test_utils INTERFACE)
target_include_directories(test_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test/include)

add_executable(
  foxy_tests

  ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/async_read_body_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/listener_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/route_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/client_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/allocator_client_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/proto_client_test.cpp
)

target_link_libraries(
  foxy_tests

  PRIVATE
  libfoxy
  test_utils
  Catch
  Boost::context
  Boost::thread
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake.modules/")
enable_testing()
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(foxy_tests)

# Installation

include(GNUInstallDirs)

install(
  TARGETS
    libfoxy
    foxy_tests

  EXPORT
    FoxyTargets

  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  EXPORT
    FoxyTargets

  FILE
    FoxyTargets.cmake

  NAMESPACE
    Foxy::

  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/foxy
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include

  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/foxy

  FILES_MATCHING PATTERN
    "*.h*"
)

#####################
# ConfigVersion file
##
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/FoxyConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FoxyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/FoxyConfig.cmake
    INSTALL_DESTINATION ${CMAKE_CURRENT_LIST_DIR}/cmake
)

## Install all the helper files
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FoxyConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FoxyConfigVersion.cmake

  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/cmake
)

add_library(Foxy::libfoxy ALIAS libfoxy)