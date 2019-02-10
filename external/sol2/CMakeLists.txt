cmake_minimum_required(VERSION 3.5)
project(sol2)

add_library(sol INTERFACE)
target_include_directories(sol INTERFACE ${PROJECT_SOURCE_DIR})
target_compile_definitions(sol INTERFACE SOL_USING_CXX_LUA)
target_link_libraries(sol INTERFACE lua)
