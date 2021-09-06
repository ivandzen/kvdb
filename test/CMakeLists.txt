cmake_minimum_required(VERSION 3.0)

set(_test_target "kvdb_test")

file(GLOB _src "*.cpp" "*.hpp")

add_executable(${_test_target} ${_src})

target_link_libraries(${_test_target}
   "${CMAKE_BINARY_DIR}/common/libkvdb.a"
   ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${Boost_LOG_LIBRARY}
   ${Boost_PROGRAM_OPTIONS_LIBRARY}
   ${CMAKE_THREAD_LIBS_INIT})
