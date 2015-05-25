cmake_minimum_required(VERSION 3.2)
project(Accounting)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES Accounting.c)
add_executable(Accounting ${SOURCE_FILES} Accounting.h)