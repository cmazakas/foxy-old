cmake_minimum_required(VERSION 3.9.3)

project(foxy)

find_path(CATCH_INCLUDE_DIR catch.hpp)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

find_package(Boost 1.65 REQUIRED system coroutine)

add_library(
  foxy_core 
  
  STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/make_acceptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/make_v4_endpoint.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/listener.cpp
)

if (MSVC)
  target_compile_definitions(foxy_core PUBLIC _WIN32_WINNT=0x0A00)
endif()

target_compile_definitions(foxy_core PUBLIC BOOST_COROUTINES_NO_DEPRECATION_WARNING=1)
target_compile_features(foxy_core PUBLIC cxx_std_14)

target_include_directories(
  foxy_core

  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/beast/include
)

target_link_libraries(
  foxy_core

  PUBLIC
  Boost::system
  Boost::coroutine
)

add_executable(
  foxy_tests

  ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
)

target_link_libraries(foxy_tests foxy_core)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake.modules/")
enable_testing()
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(foxy_tests)