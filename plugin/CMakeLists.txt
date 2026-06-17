cmake_minimum_required(VERSION 3.20)

# Static MSVC runtime so the DLL has no VC-redist dependency in the Wine prefix.
cmake_policy(SET CMP0091 NEW)

project(FNVWebSocket CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# xNVSE source (cloned by CI into plugin/extern/xnvse). We use it HEADER-ONLY:
# g_thePlayer is defined in main.cpp at its runtime address, and actor-value reads
# go through virtual calls — so we don't compile the whole xNVSE tree.
set(XNVSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/xnvse" CACHE PATH "Path to xNVSE checkout")

add_library(FNVWebSocket SHARED
    src/main.cpp
    src/ws_server.cpp
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
        /EHsc /W3
        /FI"nvse/prefix.h"                  # xNVSE headers assume prefix.h is force-included
        /bigobj
    )
    target_link_options(FNVWebSocket PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/exports.def")
endif()

target_link_libraries(FNVWebSocket PRIVATE ws2_32)

# IMPORTANT: configure for 32-bit (FNV is x86):  cmake -S plugin -B build -A Win32
