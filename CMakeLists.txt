cmake_minimum_required(VERSION 4.1)
project(RedScript)

set(CMAKE_CXX_STANDARD 20)

add_executable(RedScript src/main.cpp
        src/tokenization.h
        src/parser.h
        src/generation.h
        src/arena.h)
