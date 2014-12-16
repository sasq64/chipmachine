cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

add_definitions(-DAPP_NAME=chipmachine)

set(CPP_MODS ../cpp-mods)

include_directories(${CPP_MODS})

add_subdirectory(${CPP_MODS}/coreutils coreutils)
add_subdirectory(${CPP_MODS}/audioplayer audioplayer)
add_subdirectory(${CPP_MODS}/grappix grappix)
add_subdirectory(${CPP_MODS}/bbsutils bbsutils)
add_subdirectory(${CPP_MODS}/fft fft)
add_subdirectory(${CPP_MODS}/archive archive)
add_subdirectory(${CPP_MODS}/webutils webutils)
add_subdirectory(${CPP_MODS}/json json)
add_subdirectory(${CPP_MODS}/lua luainterpreter)
add_subdirectory(${CPP_MODS}/sqlite3 sqlite3)

add_subdirectory(${CPP_MODS}/musicplayer/plugins/ModPlugin modplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/OpenMPTPlugin openmptplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/HTPlugin htplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/HEPlugin heplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/GSFPlugin gsfplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/NDSPlugin ndsplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/USFPlugin usfplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/VicePlugin viceplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/GMEPlugin gmeplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/SexyPSFPlugin sexypsfplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/SC68Plugin sc68plugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/StSoundPlugin stsoundplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/AdPlugin adplugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/MP3Plugin mp3plugin)
add_subdirectory(${CPP_MODS}/musicplayer/plugins/UADEPlugin uadeplugin)

file(GLOB SOURCE_FILES src/*.cpp)
include_directories(${CPP_MODS}/grappix ${CPP_MODS}/musicplayer/plugins)

set(CMAKE_CXX_FLAGS "--std=c++0x -g")

add_executable(chipmachine ${SOURCE_FILES})
target_link_libraries(chipmachine
	coreutils
	audioplayer
	grappix
	sqlite3
	json
	luainterpreter
	archive
	webutils
	bbsutils
	fft
	modplugin
	openmptplugin
	htplugin
	heplugin
	ndsplugin
	gsfplugin
	gmeplugin
	sc68plugin
	usfplugin
	sexypsfplugin
	stsoundplugin
	adplugin
	mp3plugin
	uadeplugin
	viceplugin
)
