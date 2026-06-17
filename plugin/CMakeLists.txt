cmake_minimum_required(VERSION 3.20)

# Static MSVC runtime so the DLL has no VC-redist dependency in the Wine prefix.
cmake_policy(SET CMP0091 NEW)

project(FNVWebSocket CXX)

# xNVSE headers use std::span / std::ranges → C++20 required.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# xNVSE source (cloned by CI into plugin/extern/xnvse).
set(XNVSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/xnvse" CACHE PATH "Path to xNVSE checkout")
set(NVSE_SRC "${XNVSE_DIR}/nvse/nvse")

# The canonical plugin source set (mirrors nvse_plugin_example.vcxproj): the
# Game*/utility/container helpers + RTTI, NO hooks/commands (no runtime side effects).
set(NVSE_SOURCES
    ${NVSE_SRC}/containers.cpp
    ${NVSE_SRC}/GameAPI.cpp
    ${NVSE_SRC}/GameBSExtraData.cpp
    ${NVSE_SRC}/GameData.cpp
    ${NVSE_SRC}/GameExtraData.cpp
    ${NVSE_SRC}/GameForms.cpp
    ${NVSE_SRC}/GameObjects.cpp
    ${NVSE_SRC}/GameOSDepend.cpp
    ${NVSE_SRC}/GameProcess.cpp
    ${NVSE_SRC}/GameRTTI.cpp
    ${NVSE_SRC}/GameScript.cpp
    ${NVSE_SRC}/GameSettings.cpp
    ${NVSE_SRC}/GameTasks.cpp
    ${NVSE_SRC}/GameTiles.cpp
    ${NVSE_SRC}/GameTypes.cpp
    ${NVSE_SRC}/NiTypes.cpp
    ${NVSE_SRC}/printf.cpp
    ${NVSE_SRC}/SafeWrite.cpp
    ${NVSE_SRC}/Utilities.cpp
    ${NVSE_SRC}/utility.cpp
)
file(GLOB COMMON_SOURCES "${XNVSE_DIR}/common/*.cpp")

add_library(FNVWebSocket SHARED
    src/main.cpp
    src/ws_server.cpp
    ${NVSE_SOURCES}
    ${COMMON_SOURCES}
)
target_include_directories(FNVWebSocket PRIVATE
    src
    "${XNVSE_DIR}"        # for "common/..."
    "${XNVSE_DIR}/nvse"   # for "nvse/..."
)

set_target_properties(FNVWebSocket PROPERTIES OUTPUT_NAME "FNVWebSocket" PREFIX "")

if (MSVC)
    set_property(TARGET FNVWebSocket PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_compile_definitions(FNVWebSocket PRIVATE
        WIN32 _WINDOWS _USRDLL RUNTIME
        RUNTIME_VERSION=0x040020D0          # FNV 1.4.0.525
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(FNVWebSocket PRIVATE
        /EHsc /W0           # xNVSE source is not warning-clean; silence to focus on errors
        /FI"nvse/prefix.h"  # xNVSE headers assume prefix.h is force-included
        /bigobj
    )
    target_link_options(FNVWebSocket PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/exports.def")
endif()

target_link_libraries(FNVWebSocket PRIVATE ws2_32)

# IMPORTANT: configure for 32-bit (FNV is x86):  cmake -S plugin -B build -A Win32
