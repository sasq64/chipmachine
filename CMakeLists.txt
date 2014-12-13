cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

set(CMAKE_CXX_FLAGS "--std=c++0x")

file(GLOB SOURCE_FILES src/*.cpp)
#set(SOURCE_FILES ${SOURCE_FILES} grappix/specific/window_pc.cpp)
#set(SOURCE_FILES src/chipmachine.cpp src/chipmachine_config.cpp)

add_executable(chipmachine ${SOURCE_FILES})
include_directories(../cpp-mods)
include_directories(../cpp-mods/grappix)

add_subdirectory(../cpp-mods/grappix grappix)
add_subdirectory(../cpp-mods/coreutils coreutils)
add_subdirectory(../cpp-mods/sqlite3 sqlite3)

target_link_libraries(chipmachine coreutils grappix sqlite3)