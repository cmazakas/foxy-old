cmake_minimum_required(VERSION 3.10)

project(foxy)

find_path(CATCH_INCLUDE_DIR catch.hpp)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

find_package(Boost 1.66.0 REQUIRED COMPONENTS system coroutine)

add_library(
  foxy_core

  STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/connection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/listener.cpp
)

if (MSVC)
  # Win10
  target_compile_definitions(foxy_core PUBLIC _WIN32_WINNT=0x0A00)
endif()

target_compile_definitions(
  foxy_core

  PUBLIC
  BOOST_COROUTINES_NO_DEPRECATION_WARNING=1
  BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1
)

target_compile_features(foxy_core PUBLIC cxx_std_14)

target_include_directories(
  foxy_core

  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  foxy_core

  PUBLIC
  debug ${Boost_SYSTEM_LIBRARY_DEBUG}
  debug ${Boost_COROUTINE_LIBRARY_DEBUG}

  optimized ${Boost_SYSTEM_LIBRARY_RELEASE}
  optimized ${Boost_COROUTINE_LIBRARY_RELEASE}
)

add_executable(
  foxy_tests

  ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/server_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/mp11_test.cpp
)

target_link_libraries(foxy_tests foxy_core)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake.modules/")
enable_testing()
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(foxy_tests)