cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

add_definitions(-DAPP_NAME=chipmachine)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DCM_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_RELEASE} -DCM_DEBUG")

set(CPP_MODS ../cpp-mods/mods)

include_directories(${CPP_MODS} /usr/local/include)

file(GLOB SOURCE_FILES src/*.cpp)

set(CPP_MODS_MODULES
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
	hivelyplugin
	rsnplugin
)
add_subdirectory(${CPP_MODS}/musicplayer/plugins plugins)

if(RASPBERRY)
  include_directories(${PI_SYS_ROOT}/vc/include)
endif()

include_directories(${CPP_MODS}/grappix ${CPP_MODS}/musicplayer/plugins)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")

add_executable(chipmachine ${SOURCE_FILES})
target_link_libraries(chipmachine ${CPP_MODS_MODULES} ${MUSICPLAYER_PLUGINS})
