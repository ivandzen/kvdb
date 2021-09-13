cmake_minimum_required(VERSION 3.0)

set(_client_target "kvdb_cli")

file(GLOB _src "*.cpp" "*.hpp")

add_executable(${_client_target} ${_src})

add_dependencies(${_client_target}
   kvdb)

target_link_libraries(${_client_target}
   "${CMAKE_BINARY_DIR}/lib/libkvdb.a"
   ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${Boost_LOG_LIBRARY}
   ${Boost_PROGRAM_OPTIONS_LIBRARY}
   ${CMAKE_THREAD_LIBS_INIT})
