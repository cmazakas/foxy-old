# Sample toolchain file for Windows users

set(BOOST_ROOT "/Users/cmaza/source/boosts/boost_1_67_0_b1")
set(Boost_USE_STATIC_LIBS ON)
include_directories(
  "/vcpkg/installed/x64-windows/include/catch"
  "/Users/cmaza/source/beast/include"
)