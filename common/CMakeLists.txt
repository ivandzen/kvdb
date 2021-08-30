cmake_minimum_required(VERSION 3.0)

set(_common_target "kvdb")

file(GLOB _src "*.cpp" "*.hpp")

add_library(${_common_target} ${_src})
