cmake_minimum_required(VERSION 3.1.0)
project(chipmachine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

link_directories(/usr/local/lib /usr/local/Cellar/llvm/6.0.1/lib)
include_directories(/usr/local/include)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


set(AP1_ROOT ../apone CACHE PATH "Where apone is checked out")
set(MUSICPLAYER ../musicplayer CACHE PATH "Where musicplayer is checked out")

include(${AP1_ROOT}/cmake/Utils.cmake)


if(APPLE)
    #set(CMAKE_OSX_SYSROOT "/Users/jonasminnberg/MacOSX10.10.sdk")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11")
endif()

if(RASPBERRYPI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
endif()

# GCC/Clang common warnings
set(GCC_CLANG_WARNINGS -Wall -Wextra -Wnon-virtual-dtor -pedantic)

# GCC specific warnings
set(GCC_WARNINGS
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wnull-dereference
    -Wcast-align
    -Wno-missing-field-initializers)

# MSVC warnings
set(MSVC_WARNINGS /W4)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(WARNINGS ${WARNINGS} ${GCC_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(WARNINGS ${WARNINGS} ${CLANG_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(WARNINGS ${WARNINGS} ${MSVC_WARNINGS})
endif()

add_library(Warnings INTERFACE)
target_compile_options(Warnings INTERFACE ${WARNINGS})

# Workaround for gcc10 linking errors
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
endif()

if(WINDOWS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funsigned-char")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funsigned-char")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
endif()

if(SAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${SAN}")
endif()

add_subdirectory(external/lua)
add_subdirectory(external/sol2)


# Main app files
set(MAIN_FILES
    src/MusicDatabase.cpp
    src/GZPlugin.cpp
    src/MusicPlayer.cpp
    src/MusicPlayerList.cpp
    src/RemoteLoader.cpp
    src/SearchIndex.cpp
    src/SongFileIdentifier.cpp
    src/state_machine.cpp
    src/youtube.cpp
    src/textmode.cpp
)

# GL GUI files
set(GUI_FILES
    src/ChipMachine.cpp
    src/MusicBars.cpp
    src/ChipMachine_config.cpp
    src/ChipMachine_keys.cpp
    src/ChipMachine_commands.cpp
    src/textmode.cpp
)

# Add include files
set(SOURCE_FILES ${SOURCE_FILES} src/version.h src/TextField.h src/TextListView.h src/CueSheet.h
    src/Dialog.h src/LineEdit.h src/Icons.h src/SongInfo.h src/SongInfoField.h src/ChipInterface.h)

file(GLOB DATA_FILES data/*.txt)
file(GLOB LUA_FILES lua/*.lua)
file(GLOB DOC_FILES *.md)
file(GLOB SCRIPT_FILES scripts/*)

add_custom_target(chipmachine.data_files SOURCES ${DATA_FILES} )
add_custom_target(chipmachine.lua_files SOURCES ${LUA_FILES})
add_custom_target(chipmachine.doc_files SOURCES ${DOC_FILES} )
add_custom_target(chipmachine.script_files SOURCES ${SCRIPT_FILES} )

set(CORE_MODULES
    coreutils
    audioplayer
    archive
    webutils
    sqlite3
    xml
    bbsutils
    crypto
)

set(GUI_MODULES
    grappix
    fft
)

set(APONE_MODULES ${GUI_MODULES} ${CORE_MODULES})
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
    v2plugin
)
add_subdirectory(${MUSICPLAYER}/plugins plugins)
add_subdirectory(${MUSICPLAYER}/psf psf)

include_directories(${AP1_ROOT}/mods/grappix ${MUSICPLAYER}/plugins)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -mwindows)
endif()

if(SAN)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${SAN})
endif()


add_library(musicplayer INTERFACE)
target_link_libraries(musicplayer INTERFACE psf plugin_register)
target_include_directories(musicplayer INTERFACE ${MUSICPLAYER}/..)

if(APPLE)
    add_executable(chipmachine MACOSX_BUNDLE ${MAIN_FILES} ${GUI_FILES} src/main.cpp)
    add_custom_command(TARGET chipmachine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chipmachine> ${CMAKE_CURRENT_BINARY_DIR})
else()
    add_executable(chipmachine ${MAIN_FILES} ${GUI_FILES} src/main.cpp)
endif()

target_link_libraries(chipmachine PRIVATE musicplayer plugin_register
                      ${CORE_MODULES} ${GUI_MODULES} ${MUSICPLAYER_PLUGINS} pthread sol z)
target_compile_definitions(chipmachine PRIVATE -DAPP_NAME=chipmachine)

if(NOT WIN32)
    add_executable(cm ${MAIN_FILES} src/textmode.cpp src/main.cpp)
    target_compile_definitions(cm PUBLIC TEXTMODE_ONLY)
    target_link_libraries(cm musicplayer ${CORE_MODULES} ${MUSICPLAYER_PLUGINS} sol z)
endif()

if(APPLE)
    set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
    set(DIRS "")
    install(CODE "
       file(COPY \"${CMAKE_SOURCE_DIR}/lua\" DESTINATION Chipmachine.app/Contents/Resources/)
       file(COPY \"${CMAKE_SOURCE_DIR}/data\" DESTINATION Chipmachine.app/Contents/Resources/)
       file(MAKE_DIRECTORY Chipmachine.app/Contents/Resources/bin) 
       file(COPY \"${CMAKE_SOURCE_DIR}/note.png\" DESTINATION Chipmachine.app/Contents/)
       file(COPY \"${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)
       include(BundleUtilities)
       fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
       file(COPY \"${CMAKE_SOURCE_DIR}/bin/ffmpeg\" DESTINATION Chipmachine.app/Contents/Resources/bin/)
       file(COPY \"${CMAKE_SOURCE_DIR}/bin/youtube-dl\" DESTINATION Chipmachine.app/Contents/Resources/bin/)
       " COMPONENT Runtime)
endif()

add_executable(mksonglist mksonglist/mksonglist.cpp src/SongFileIdentifier.cpp)
target_link_libraries(mksonglist coreutils archive sc68plugin mp3plugin)

add_executable(cmtest testmain.cpp test.cpp ${MAIN_FILES})
target_link_libraries(cmtest PRIVATE plugin_register ${CORE_MODULES} ${MUSICPLAYER_PLUGINS} musicplayer z pthread sol)
target_include_directories(cmtest PRIVATE ${APONE}/mods)
target_compile_definitions(cmtest PRIVATE -DTESTING)

#${TEXTMODE_FILES} add_executable(ditest ${PLUGIN_EXTRA_FILES} testmain.cpp ditest.cpp ${APP_FILES} ${SOURCE_FILES})
# target_link_libraries(ditest ${APONE_MODULES} ${MUSICPLAYER_PLUGINS} pthread sol)
# target_include_directories(ditest PRIVATE ${APONE}/mods)
# target_compile_definitions(ditest PRIVATE -DTESTING)

