cmake_minimum_required(VERSION 3.22)
project(native)
add_library(native SHARED native.c)
target_link_libraries(native log)
