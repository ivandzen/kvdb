cmake_minimum_required(VERSION 3.0)

set(_server_target "kvdb_server")

file(GLOB _src "*.cpp" "*.hpp")

add_executable(${_server_target} ${_src})

add_dependencies(${_server_target}
   kvdb)

target_link_libraries(${_server_target}
   "${CMAKE_BINARY_DIR}/common/libkvdb.a"
   ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${Boost_LOG_LIBRARY}
   ${Boost_PROGRAM_OPTIONS_LIBRARY}
   ${CMAKE_THREAD_LIBS_INIT})
