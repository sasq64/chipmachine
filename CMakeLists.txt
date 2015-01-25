cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

add_definitions(-DAPP_NAME=chipmachine)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DCM_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DCM_DEBUG")

set(AP1_ROOT ../apone)

include_directories(${AP1_ROOT}/mods)

file(GLOB SOURCE_FILES src/*.cpp)

set(APONE_MODULES
	coreutils
	audioplayer
	grappix
	bbsutils
	fft
	archive
	webutils
	json
	luainterpreter
	sqlite3
)
add_subdirectory(${AP1_ROOT}/mods ap1mods)

set(MUSICPLAYER_PLUGINS
	openmptplugin
	htplugin
	heplugin
	ndsplugin
#	gsfplugin
	gmeplugin
	sc68plugin
	usfplugin
	stsoundplugin
	adplugin
	mp3plugin
	viceplugin
	hivelyplugin
	rsnplugin
)

if(NOT WINDOWS)
	set(MUSICPLAYER_PLUGINS ${MUSICPLAYER_PLUGINS} uadeplugin)
endif()

add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

include_directories(${AP1_ROOT}/mods/grappix ${AP1_ROOT}/mods/musicplayer/plugins)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")

include(${AP1_ROOT}/cmake/Utils.cmake)
SET_DATA_FILES(data lua)

#if(APPLE)
#	add_executable(chipmachine MACOSX_BUNDLE ${SOURCE_FILES})
#else()
	add_executable(chipmachine ${SOURCE_FILES})
#endif()

target_link_libraries(chipmachine ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})

add_custom_command(TARGET chipmachine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plugins/uadeplugin/uade/uadecore ${CMAKE_CURRENT_BINARY_DIR}/data/uade)

#include(BundleUtilities)
#fixup_bundle(${CMAKE_CURRENT_BINARY_DIR}/chipmachine.app "" "dfssds")
#verify_app(${CMAKE_CURRENT_BINARY_DIR}/chipmachine.app)
