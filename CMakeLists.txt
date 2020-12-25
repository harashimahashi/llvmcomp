cmake_minimum_required(VERSION 3.16)
project(llvmc VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

add_executable(llvmc src/llvmc.cpp src/lex.cpp)
