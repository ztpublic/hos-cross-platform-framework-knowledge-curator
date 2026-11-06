# NAPI wrapper around Kotlin/Native libohoswebview.so.
# ArkTS cannot call @CName symbols; HarmonyOS CAdapter does not export
# business functions. This .so is the actual NAPI surface.
cmake_minimum_required(VERSION 3.5.0)
project(webview_napi)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

if(NOT DEFINED OHOS_ARCH OR OHOS_ARCH STREQUAL "")
    message(FATAL_ERROR "OHOS_ARCH is empty; expected arm64-v8a or x86_64")
endif()

set(KN_SO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH})
set(KN_SO ${KN_SO_DIR}/libohoswebview.so)
if(NOT EXISTS ${KN_SO})
    message(FATAL_ERROR "Missing ${KN_SO}. Run :example:nativeApp:linkDebugSharedOhosArm64 / OhosX64 first.")
endif()

add_library(ohoswebview SHARED IMPORTED)
set_target_properties(ohoswebview PROPERTIES
    IMPORTED_LOCATION ${KN_SO}
    IMPORTED_NO_SONAME TRUE
)

add_library(webview_napi SHARED napi_init.cpp)
target_include_directories(webview_napi PRIVATE
    ${NATIVERENDER_ROOT_PATH}
    ${NATIVERENDER_ROOT_PATH}/include
)
target_link_options(webview_napi PRIVATE
    "-Wl,--no-undefined"
    "-Wl,-rpath,\$ORIGIN"
)
set_target_properties(webview_napi PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE
)

target_link_libraries(webview_napi PUBLIC
    libace_napi.z.so
    libhilog_ndk.z.so
    ohoswebview
)

# Keep KN runtime + libc++ next to the NAPI so $ORIGIN resolves at dlopen.
add_custom_command(TARGET webview_napi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${KN_SO}
        $<TARGET_FILE_DIR:webview_napi>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${KN_SO_DIR}/libc++_shared.so
        $<TARGET_FILE_DIR:webview_napi>
)
