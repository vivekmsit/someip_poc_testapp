cmake_minimum_required(VERSION 3.6)
project(someip_poc_testapp)

set(CMAKE_CXX_STANDARD 11)

include_directories(runtime)

add_executable(someip_poc_testapp main.cpp runtime/Runtime.cpp)