cmake_minimum_required(VERSION 3.5)
project(lua)

set(CORE_FILES lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c
    llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c
    ltm.c lundump.c lvm.c lzio.c)

set(LIB_FILES lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c
    lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c)

set(LUA_SOURCES ${CORE_FILES} ${LIB_FILES})
SET_SOURCE_FILES_PROPERTIES( ${LUA_SOURCES} PROPERTIES LANGUAGE CXX )
add_library(lua ${LUA_SOURCES})
if(NOT ANDROID AND NOT WIN32)
    target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
endif()
#if(WIN32)
#  target_compile_options(lua PRIVATE /TP)
#else()
  target_compile_options(lua PRIVATE -x c++)
#endif()
target_include_directories(lua INTERFACE ${PROJECT_SOURCE_DIR})
