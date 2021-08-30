cmake_minimum_required(VERSION 3.0)

set(_server_target "kvdb_server")

file(GLOB _src "*.cpp" "*.hpp")

add_executable(${_server_target} ${_src})
