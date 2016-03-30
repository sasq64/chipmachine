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
#	src/RemoteLists.cpp
	src/RemoteLoader.cpp
	src/SearchIndex.cpp
	src/SongFileIdentifier.cpp
#	src/TelnetInterface.cpp
	src/state_machine.cpp
	src/textmode.cpp
	src/main.cpp
)

set(TEXTMODE_FILES
	src/MusicDatabase.cpp
	src/MusicPlayer.cpp
	src/MusicPlayerList.cpp
	src/RemoteLoader.cpp
	src/SearchIndex.cpp
	src/SongFileIdentifier.cpp
	src/state_machine.cpp
	src/textmode.cpp
	src/main.cpp
)

add_custom_target(chipmachine.include_files SOURCES src/version.h src/TextField.h src/TextListView.h src/CueSheet.h
	src/Dialog.h src/LineEdit.h src/Icons.h src/SongInfo.h src/SongInfoField.h src/ChipInterface.h)

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
	bbsutils
)
set(TEXT_MODULES
	coreutils
	audioplayer
	fft
	archive
	webutils
	luainterpreter
	sqlite3
	xml
	bbsutils
)
add_subdirectory(${AP1_ROOT}/mods ap1mods)

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
	uadeplugin
)
add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

include_directories(${AP1_ROOT}/mods/grappix ${AP1_ROOT}/mods/musicplayer/plugins)

add_subdirectory(docopt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -mwindows)
endif()

if(APPLE)
	add_executable(chipmachine MACOSX_BUNDLE ${SOURCE_FILES})
	add_custom_command(TARGET chipmachine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chipmachine> ${CMAKE_CURRENT_BINARY_DIR})
else()
	add_executable(chipmachine ${SOURCE_FILES})
endif()

add_executable(cm ${PLUGIN_EXTRA_FILES} ${TEXTMODE_FILES})

target_compile_definitions(cm PUBLIC TEXTMODE_ONLY)

target_link_libraries(chipmachine ${APONE_MODULES} ${MUSICPLAYER_PLUGINS} docopt_s)
target_link_libraries(cm ${TEXT_MODULES} ${MUSICPLAYER_PLUGINS} docopt_s)

if(APPLE)
	set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
	set(DIRS "")
	install(CODE "
	   file(COPY \"${CMAKE_SOURCE_DIR}/lua\" DESTINATION Chipmachine.app/Contents/Resources/)
	   file(COPY \"${CMAKE_SOURCE_DIR}/data\" DESTINATION Chipmachine.app/Contents/Resources/)
	   file(COPY \"${CMAKE_SOURCE_DIR}/note.png\" DESTINATION Chipmachine.app/Contents/)
	   file(COPY \"${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)
	   include(BundleUtilities)
       fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
       " COMPONENT Runtime)
endif()
