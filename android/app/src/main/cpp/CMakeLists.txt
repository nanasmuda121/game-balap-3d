cmake_minimum_required(VERSION 3.16)
project(Apex3DRacing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLATFORM "Android" CACHE STRING "" FORCE)
set(GRAPHICS "GRAPHICS_API_OPENGL_ES3" CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

set(GAME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/city.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/car.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/bot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/ui.cpp
)

add_library(main SHARED ${GAME_SOURCES})

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src
    ${raylib_SOURCE_DIR}/src
)

target_link_libraries(main PRIVATE
    raylib
    android
    log
    EGL
    GLESv3
    OpenSLES
    m
)

target_link_options(main PRIVATE -u ANativeActivity_onCreate)
