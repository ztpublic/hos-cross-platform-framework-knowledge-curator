# ============================================================================
# Letos (formerly SQLiteStudio) — HarmonyOS OHOS cross-compile wrapper
# ============================================================================
# Strategy: Qt6.12 prebuilt SDK (策略 B'), qt_add_executable for OHOS .so+main.
# We add_subdirectory the original Letos core+gui source, skip the original
# letos executable target, and create our own qt_add_executable instead.
#
# Key fixes for OHOS:
# 1. CMAKE_SHARED_LIBRARY_SONAME_FLAG="" — prevents SONAME in .so binaries
# 2. QPA plugin linked by bare name (-lqohos), not target (avoids abs path NEEDED)
# 3. Post-build patch_needed.py — fixes .so.1 → .so in DT_NEEDED entries
# 4. sqlite3.h shim — redirects sqlite3_* → mc_sqlite3_* for bundled amalgamation
# ============================================================================

cmake_minimum_required(VERSION 3.20)

# ── OHOS cross-compile: must be BEFORE find_package ──────────────────────────
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

# Disable SONAME in shared library binaries — OHOS HAP expects bare .so names.
# This prevents -Wl,-soname,libxxx.so.1 from being passed to the linker.
# The original CMakeLists still set VERSION/SOVERSION properties which create
# symlinks (libxxx.so → libxxx.so.1 → libxxx.so.1.0.0), but the .so binary
# itself won't have a SONAME, so NEEDED entries reference bare .so names.
# Post-build patch_needed.py fixes any remaining .so.1 → .so references.
set(CMAKE_SHARED_LIBRARY_SONAME_FLAG "")
set(CMAKE_SKIP_RPATH ON)

project(letos LANGUAGES C CXX VERSION 4.0.3)

# ── Qt6 setup ────────────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Network
    Widgets
    Svg
    UiTools
    Xml
    Sql
    Qml
    LinguistTools
)

qt_standard_project_setup()

# ── Letos build options (set BEFORE add_subdirectory) ────────────────────────
set(WITH_GUI ON CACHE BOOL "" FORCE)
set(WITH_PORTABLE ON CACHE BOOL "" FORCE)
set(WITH_UPDATER OFF CACHE BOOL "" FORCE)
set(WITH_SUPPORT_REMINDERS OFF CACHE BOOL "" FORCE)
set(WITH_SHARED_RES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CUSTOM_SQLITE3 "1" CACHE PATH "" FORCE)

# ── SQLite3: build from bundled amalgamation ────────────────────────────────
set(SQLITE3MC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../PluginsSrc/DbSQLite3MC")
if(NOT TARGET Letos::SQLite3)
    add_library(leto_sqlite3 STATIC
        ${SQLITE3MC_DIR}/sqlite3mc_amalgamation.c
    )
    target_compile_definitions(leto_sqlite3 PRIVATE
        SQLITE_ENABLE_COLUMN_METADATA
        SQLITE_ENABLE_FTS5
        SQLITE_ENABLE_JSON1
        SQLITE_ENABLE_RTREE
        SQLITE_ENABLE_DBSTAT_VTAB
        SQLITE_ENABLE_SESSION
    )
    target_include_directories(leto_sqlite3 PUBLIC ${SQLITE3MC_DIR})
    add_library(Letos::SQLite3 ALIAS leto_sqlite3)
    message(STATUS "Building SQLite3 from bundled amalgamation")
endif()

# ── Include Letos CMake framework ────────────────────────────────────────────
set(LETOSSRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../LetosSrc")
list(APPEND CMAKE_MODULE_PATH "${LETOSSRC_DIR}/cmake")
include(version)
letos_find_version()

include(dirs)
include(common)

add_custom_target(MiscFiles)

# ── Build coreLetos and guiLetos ─────────────────────────────────────────────
add_subdirectory(${LETOSSRC_DIR}/core ${CMAKE_CURRENT_BINARY_DIR}/LetosBuild/core)
add_subdirectory(${LETOSSRC_DIR}/gui ${CMAKE_CURRENT_BINARY_DIR}/LetosBuild/gui)

# ── Patch targets for OHOS ───────────────────────────────────────────────────
get_target_property(_QOHOS_LIB Qt6::QOhosPlatformIntegrationPlugin IMPORTED_LOCATION_RELEASE)
if(NOT _QOHOS_LIB)
    get_target_property(_QOHOS_LIB Qt6::QOhosPlatformIntegrationPlugin IMPORTED_LOCATION)
endif()
get_filename_component(_QOHOS_DIR "${_QOHOS_LIB}" DIRECTORY)

if(TARGET coreLetos)
    target_link_directories(coreLetos PRIVATE ${_QOHOS_DIR})
    target_link_libraries(coreLetos PRIVATE -lqohos Qt6::Sql)
    target_include_directories(coreLetos PRIVATE ${SQLITE3MC_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
endif()
if(TARGET guiLetos)
    target_link_directories(guiLetos PRIVATE ${_QOHOS_DIR})
    target_link_libraries(guiLetos PRIVATE -lqohos)
    target_include_directories(guiLetos PRIVATE ${SQLITE3MC_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# ── OHOS entry: qt_add_executable ───────────────────────────────────────────
qt_add_executable(letos_ohos
    ${LETOSSRC_DIR}/letos/main.cpp
    ${LETOSSRC_DIR}/letos/singleapplication/singleapplication.cpp
)

qt_add_resources(letos_ohos "${LETOSSRC_DIR}/letos/letos.qrc")

target_compile_definitions(letos_ohos PRIVATE
    QAPPLICATION_CLASS=QApplication
    PORTABLE_CONFIG
)

target_link_directories(letos_ohos PRIVATE ${_QOHOS_DIR})
target_link_libraries(letos_ohos PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    -lqohos
    coreLetos
    guiLetos
)

target_include_directories(letos_ohos PRIVATE
    ${LETOSSRC_DIR}/letos
    ${LETOSSRC_DIR}/gui
    ${LETOSSRC_DIR}/core
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/LetosBuild/gui
)

set_target_properties(letos_ohos PROPERTIES
    CXX_VISIBILITY_PRESET default
)

# ── Post-build: patch DT_NEEDED to bare .so names ───────────────────────────
# Fixes:
# 1. .so.1 → .so (version suffix from SOVERSION)
# 2. absolute/relative paths → bare .so (from imported targets)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Executable)
    add_custom_command(TARGET letos_ohos POST_BUILD
        COMMAND ${Python3_Executable}
                ${CMAKE_CURRENT_SOURCE_DIR}/../../../../patch_needed.py
                "$<TARGET_FILE:coreLetos>"
                "$<TARGET_FILE:guiLetos>"
                "$<TARGET_FILE:letos_ohos>"
                "$<TARGET_SONAME_FILE:coreLetos>"
                "$<TARGET_SONAME_FILE:guiLetos>"
        COMMENT "Patching DT_NEEDED entries to bare .so names"
    )
endif()
