cmake_minimum_required(VERSION 2.8.4)
project(mksonglist)

set(AP1_ROOT ../../apone)

add_definitions(-DAPP_NAME=mksonglist)

include_directories(${AP1_ROOT}/mods /usr/local/include)

set(SOURCE_FILES
	mksonglist.cpp
	../src/SongFileIdentifier.cpp
)

set(APONE_MODULES
	coreutils
	archive
)

add_subdirectory(${AP1_ROOT}/mods ap1mods)

set(MUSICPLAYER_PLUGINS
	sc68plugin
	mp3plugin
)

add_subdirectory(${AP1_ROOT}/mods/musicplayer/plugins plugins)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(mksonglist ${SOURCE_FILES})
target_link_libraries(mksonglist ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})

