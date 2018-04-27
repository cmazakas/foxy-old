# Sample toolchain file for Windows users

add_compile_options("/await" "/permissive-")
set(BOOST_ROOT "/Users/cmaza/source/boosts/boost_1_67_0")
set(Boost_USE_STATIC_LIBS ON)
include_directories(
  "/vcpkg/installed/x64-windows/include/catch"
  # "/Users/cmaza/source/beast/include"
)