# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)
project(OHKI)
add_definitions(-DOHOS_PLATFORM)

set(OHKI_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${OHKI_ROOT_PATH}
                    ${OHKI_ROOT_PATH}/include
)

aux_source_directory(. SRC_ROOT)

add_library(native SHARED ${SRC_ROOT})

find_library(
    # Sets the name of the path variable.
    hilog-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    hilog_ndk.z
)

target_link_libraries(native PUBLIC libace_napi.z.so libhilog_ndk.z.so ${OHKI_ROOT_PATH}/../../../libs/arm64-v8a/libkn.so)