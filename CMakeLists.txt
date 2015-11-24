cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

set(AP1_ROOT ../apone)
include(${AP1_ROOT}/cmake/Utils.cmake)

add_definitions(-DAPP_NAME=chipmachine)
#(-DUSE_REMOTELISTS -DENABLE_TELNET)

if(RASPBERRYPI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funsigned-char")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funsigned-char")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DCM_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DCM_DEBUG")


#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")


include_directories(${AP1_ROOT}/mods /usr/local/include)


set(SOURCE_FILES
	src/ChipMachine.cpp
	src/MusicBars.cpp
	src/ChipMachine_config.cpp
	src/ChipMachine_keys.cpp
	src/MusicDatabase.cpp
	src/MusicPlayer.cpp
	src/MusicPlayerList.cpp
	src/PlaylistDatabase.cpp
#	src/RemoteLists.cpp
	src/RemoteLoader.cpp
	src/SearchIndex.cpp
#	src/SongFileIdentifier.cpp
#	src/TelnetInterface.cpp
	src/renderable.cpp
	src/renderset.cpp
	src/state_machine.cpp
	src/main.cpp
#	src/tinyxml2.cpp
)

#if(RASPBERRY)
#	set(SOURCE_FILES ${SOURCE_FILES} src/PiTFT.cpp)
#endif()

set(APONE_MODULES
	coreutils
	audioplayer
	grappix
	fft
	archive
	webutils
#	json
	luainterpreter
	sqlite3
#	net
	xml
)

if(NOT WIN32)
  set(APONE_MODULES ${APONE_MODULES} bbsutils)
endif()

add_subdirectory(${AP1_ROOT}/mods ap1mods)

if(APPLE)
	set(DATA_TARGET ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app/Contents/Resources/)
	set(UADECORE_TARGET "${DATA_TARGET}data/uade/")
else()
	set(UADECORE_TARGET "${CMAKE_CURRENT_BINARY_DIR}/data/uade/")
endif()

set(MUSICPLAYER_PLUGINS
	openmptplugin
	htplugin
	heplugin
	ndsplugin
	gmeplugin
	sc68plugin
	usfplugin
	stsoundplugin
	adplugin
	mp3plugin
	viceplugin
	hivelyplugin
	rsnplugin
	ayflyplugin
	mdxplugin
	s98plugin
	aoplugin
	gsfplugin
)

#if(NOT WIN32)
	set(MUSICPLAYER_PLUGINS ${MUSICPLAYER_PLUGINS} uadeplugin)
#endif()

add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

include_directories(${AP1_ROOT}/mods/grappix ${AP1_ROOT}/mods/musicplayer/plugins)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#include_directories(${AP1_ROOT}/mods/glew/glew-1.11.0/include ${AP1_ROOT}mods/musicplayer/plugins/mp3plugin/mpg123-1.21.0/src/libmpg123)

if(APPLE)
	add_executable(chipmachine MACOSX_BUNDLE ${SOURCE_FILES})
else()
	add_executable(chipmachine ${SOURCE_FILES})
endif()

#add_executable(modland modland.cpp)
#target_link_libraries(modland coreutils)

if(WIN32)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -mwindows)
endif()

target_link_libraries(chipmachine ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})
SET_DATA_FILES(data lua)

if(APPLE)
	set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
	set(DIRS "")
	install(CODE "
	   file(COPY \"${CMAKE_SOURCE_DIR}/note.png\" DESTINATION Chipmachine.app/Contents/)
	   file(COPY \"${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)
	   include(BundleUtilities)
       fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
       " COMPONENT Runtime)
endif()
