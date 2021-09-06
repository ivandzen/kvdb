cmake_minimum_required(VERSION 3.0)

project(kvdb)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.71 COMPONENTS system log program_options REQUIRED)
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(test)
