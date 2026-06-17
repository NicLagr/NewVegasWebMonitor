// Minimal NVSE plugin interface — just enough to be loaded by NVSE.
//
// This avoids vendoring the full xNVSE source for the spike. We only need the
// PluginInfo struct and the leading fields of NVSEInterface (we don't call any
// NVSE functions yet). When we add real game-state reads (Phase 2+), replace
// this with the real xNVSE headers (github.com/xNVSE/NVSE) so we get the game
// struct definitions + JIP/JohnnyGuitar helpers.
#pragma once
#include <cstdint>

typedef uint32_t UInt32;

struct PluginInfo {
    enum { kInfoVersion = 2 };
    UInt32       infoVersion;
    const char*  name;
    UInt32       version;
};

// Only the leading version/data fields are guaranteed by this minimal header.
// The real struct continues with many function pointers we don't use yet.
struct NVSEInterface {
    UInt32 nvseVersion;
    UInt32 runtimeVersion;
    UInt32 editorVersion;
    UInt32 isEditor;
    // ... function pointers follow in the real interface (unused here) ...
};
