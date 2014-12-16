cmake_minimum_required(VERSION 2.8.4)
project(chipmachine)

set(CMAKE_CXX_FLAGS "--std=c++0x")

set(CPP_MODS ../../cpp-mods)

include_directories(${CPP_MODS} ${CPP_MODS}/musicplayer/plugins)
#include_directories(${CPP_MODS}/g#rappix)

add_subdirectory(${CPP_MODS}/coreutils coreutils)

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


add_executable(miniplay miniplay.cpp)
target_link_libraries(miniplay coreutils modplugin openmptplugin htplugin heplugin ndsplugin gsfplugin gmeplugin sc68plugin usfplugin sexypsfplugin stsoundplugin adplugin mp3plugin uadeplugin)