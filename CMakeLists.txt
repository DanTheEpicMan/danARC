cmake_minimum_required(VERSION 4.1)
project(danARC)

set(CMAKE_CXX_STANDARD 20)

add_executable(danARC main.cpp
        memory/memory.cpp
        memory/memory.h
        utils/localUtil.h
        offsets.h
        utils/gameUtil.h)
