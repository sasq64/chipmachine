cmake_minimum_required(VERSION 2.8.4)
project(miniplay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2")

add_definitions(-DAPP_NAME=miniplay)


set(CPP_MODS ../../cpp-mods)

set(SOURCE_FILES miniplay.cpp)

include_directories(${CPP_MODS} ${CPP_MODS}/musicplayer/plugins)

set(CPP_MODS_MODULES
	coreutils
	audioplayer
)
add_subdirectory(${CPP_MODS} cppmods)

set(MUSICPLAYER_PLUGINS
	openmptplugin
	htplugin
	heplugin
	ndsplugin
	gsfplugin
	gmeplugin
	sc68plugin
	usfplugin
	stsoundplugin
	adplugin
	mp3plugin
	uadeplugin
	viceplugin
)
add_subdirectory(${CPP_MODS}/musicplayer/plugins plugins)

message("SOURCE FILES " ${SOURCE_FILES})


set(CMAKE_CXX_FLAGS "--std=c++0x")

add_executable(miniplay ${SOURCE_FILES})
target_link_libraries(miniplay ${CPP_MODS_MODULES} ${MUSICPLAYER_PLUGINS})
