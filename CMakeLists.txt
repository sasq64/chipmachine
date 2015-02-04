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

set(SOURCE_FILES
	src/ChipMachine.cpp
	src/ChipMachine_config.cpp
	src/ChipMachine_keys.cpp
	src/MusicDatabase.cpp
	src/MusicPlayer.cpp
	src/MusicPlayerList.cpp
	src/PlaylistDatabase.cpp
	src/RemoteLists.cpp
	src/RemoteLoader.cpp
	src/SearchIndex.cpp
	src/SongFileIdentifier.cpp
	src/TelnetInterface.cpp
	src/renderable.cpp
	src/renderset.cpp
	src/state_machine.cpp
	src/main.cpp
)

if(RASPBERRY)
	set(SOURCE_FILES ${SOURCE_FILES} src/PiTFT.cpp)
endif()

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

if(APPLE)
	add_executable(chipmachine MACOSX_BUNDLE ${SOURCE_FILES})
else()
	add_executable(chipmachine ${SOURCE_FILES})
endif()

#add_custom_command(OUTPUT data/uade/uadecore DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/plugins/uadeplugin/uade/uadecore
#	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plugins/uadeplugin/uade/uadecore ${CMAKE_CURRENT_BINARY_DIR}/data/uade)

target_link_libraries(chipmachine ${APONE_MODULES} ${MUSICPLAYER_PLUGINS})
#add_dependencies(chipmachine data/uade/uadecore)

#include(BundleUtilities)
#fixup_bundle(${CMAKE_CURRENT_BINARY_DIR}/chipmachine.app "" "dfssds")
#verify_app(${CMAKE_CURRENT_BINARY_DIR}/chipmachine.app)

#install(TARGETS chipmachine DESTINATION Chipmachine.app/Contents/MacOS)
#install(DIRECTORY data lua DESTINATION Chipmachine.app/Contents/Resources)
#install(FILES ${CMAKE_BINARY_DIR}/data/uade/uadecore DESTINATION Chipmachine.app/Contents/Resources/data/uade/)

set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
set(DIRS "")
# install(CODE [=[
#    file(MAKE_DIRECTORY Chipmachine.app/Contents/MacOS)
#    file(MAKE_DIRECTORY Chipmachine.app/Contents/Frameworks)
#    file(COPY chipmachine DESTINATION Chipmachine.app/Contents/MacOS)
# ]=] COMPONENT Runtime)
install(CODE " 
   file(COPY data lua DESTINATION Chipmachine.app/Contents/Resources/)
   file(COPY \"${CMAKE_SOURCE_DIR}/note.png\" DESTINATION Chipmachine.app/Contents/)
   file(COPY /Library/Frameworks/SDL.framework DESTINATION Chipmachine.app/Contents/Frameworks/)
   file(COPY \"${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)
   include(BundleUtilities)
   fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
   " COMPONENT Runtime)


# set(CPACK_PACKAGE_NAME "Chipmachine")
# set(CPACK_PACKAGE_FILE_NAME "chipmachine")
# set(CPACK_PACKAGE_ICON "note.png")

# set(CPACK_BUNDLE_NAME "Chipmachine")
# set(CPACK_BUNDLE_ICON "note.png")
# set(CPACK_BUNDLE_PLIST "Info.plist")
# set(CPACK_BUNDLE_STARTUP_COMMAND "chipmachine")

# set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jonas Minnberg")

# set(CPACK_GENERATOR "Bundle")
# include(CPack)
