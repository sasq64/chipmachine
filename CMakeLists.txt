cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

set(AP1_ROOT apone)
include(${AP1_ROOT}/cmake/Utils.cmake)

add_definitions(-DAPP_NAME=chipmachine)

# set(CMAKE_OSX_SYSROOT /Users/jonasm/MacOSX10.9.sdk)
# set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")

# set(SAN "thread")

if(RASPBERRYPI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funsigned-char")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funsigned-char")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")

if(SAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${SAN}")
endif()

link_directories(/usr/local/lib)
include_directories(${AP1_ROOT}/mods /usr/local/include)


set(SOURCE_FILES
	src/ChipMachine.cpp
	src/MusicBars.cpp
	src/ChipMachine_config.cpp
	src/ChipMachine_keys.cpp
	src/ChipMachine_commands.cpp
	src/GZPlugin.cpp
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

set(TEXTMODE_FILES
	src/MusicDatabase.cpp
	src/GZPlugin.cpp
	src/MusicPlayer.cpp
	src/MusicPlayerList.cpp
	src/RemoteLoader.cpp
	src/SearchIndex.cpp
	src/SongFileIdentifier.cpp
	src/state_machine.cpp
	src/textmode.cpp
	src/main.cpp
)

set(SOURCE_FILES ${SOURCE_FILES} src/version.h src/TextField.h src/TextListView.h src/CueSheet.h
	src/Dialog.h src/LineEdit.h src/Icons.h src/SongInfo.h src/SongInfoField.h src/ChipInterface.h)

file(GLOB DATA_FILES data/*.txt)
file(GLOB LUA_FILES lua/*.lua)
file(GLOB DOC_FILES *.md)

add_custom_target(chipmachine.data_files SOURCES ${DATA_FILES} )
add_custom_target(chipmachine.lua_files SOURCES ${LUA_FILES})
add_custom_target(chipmachine.doc_files SOURCES ${DOC_FILES} )

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
	tedplugin
	ffmpegplugin
)
add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

include_directories(${AP1_ROOT}/mods/grappix ${AP1_ROOT}/mods/musicplayer/plugins)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -mwindows)
endif()

if(SAN)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${SAN})
endif()

if(APPLE)
	add_executable(chipmachine MACOSX_BUNDLE ${SOURCE_FILES})
	add_custom_command(TARGET chipmachine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chipmachine> ${CMAKE_CURRENT_BINARY_DIR})
else()
	add_executable(chipmachine ${SOURCE_FILES})
endif()

target_link_libraries(chipmachine ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})

if(NOT WIN32)
	add_executable(cm ${PLUGIN_EXTRA_FILES} ${TEXTMODE_FILES})
	target_compile_definitions(cm PUBLIC TEXTMODE_ONLY)
	target_link_libraries(cm ${TEXT_MODULES} ${MUSICPLAYER_PLUGINS})
endif()

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

add_executable(mksonglist mksonglist/mksonglist.cpp src/SongFileIdentifier.cpp)
target_link_libraries(mksonglist coreutils archive sc68plugin mp3plugin)
