cmake_minimum_required(VERSION 2.8.4)
project(miniplay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2")

add_definitions(-DAPP_NAME=miniplay)

set(AP1_ROOT ../../apone)

include_directories(${AP1_ROOT}/mods)

set(SOURCE_FILES miniplay.cpp)

set(APONE_MODULES
	coreutils
	audioplayer
)
add_subdirectory(${AP1_ROOT}/mods ap1mods)

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
add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

include_directories(${AP1_ROOT}/mods/musicplayer/plugins)


set(CMAKE_CXX_FLAGS "-std=c++0x")

add_executable(miniplay ${SOURCE_FILES})
target_link_libraries(miniplay ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})
