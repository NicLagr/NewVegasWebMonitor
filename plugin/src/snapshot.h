// Thread-safe handoff between the game's main thread (which reads state) and the
// WebSocket thread (which serializes + sends it). Plain types only — no NVSE deps —
// so ws_server.cpp can include it without pulling in game headers.
#pragma once
#include <mutex>

struct GameSnapshot {
    bool  inGame   = false;
    float health   = 0, healthMax = 0;
    float ap       = 0, apMax     = 0;
    float rads     = 0, radsMax   = 1000;
    float xp       = 0;
    float carryWeight = 0, invWeight = 0;
    float karma    = 0;
    float caps     = 0;
    int   level    = 0;
};

// inline → one shared instance across translation units.
inline std::mutex&   snap_mutex() { static std::mutex m; return m; }
inline GameSnapshot& snap_store() { static GameSnapshot s; return s; }

inline void snapshot_set(const GameSnapshot& s) {
    std::lock_guard<std::mutex> lock(snap_mutex());
    snap_store() = s;
}
inline GameSnapshot snapshot_get() {
    std::lock_guard<std::mutex> lock(snap_mutex());
    return snap_store();
}
