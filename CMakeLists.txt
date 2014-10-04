cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

include_directories("../cpp-mods")
include_directories("../cpp-mods/grappix")
include_directories("../cpp-mods/netlink")

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB SOURCE_FILES ../cpp-mods/json/json-c/*.c)
file(GLOB SOURCE_FILES ../cpp-mods/grappix/grappix/*.cpp)

add_definitions(-std=c++0x -O2)



add_executable(chipmachine ${SOURCE_FILES})