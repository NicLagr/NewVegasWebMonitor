// Thread-safe handoff between the game's main thread (which reads state) and the
// WebSocket thread (which serializes + sends it). Inventory payloads are built as
// JSON strings on the main thread (where game access is safe) and copied out.
#pragma once
#include <mutex>
#include <string>

struct GameSnapshot {
    bool  inGame   = false;
    int   level    = 0;
    float health   = 0, healthMax = 0;
    float ap       = 0, apMax     = 0;
    float rads     = 0, radsMax   = 1000;
    float xp       = 0;
    float carryWeight = 0, invWeight = 0;
    float karma    = 0;
    float caps     = 0;

    // Player position (for map; logged until calibrated to real coords).
    float posX = 0, posY = 0, posZ = 0, angle = 0;
    bool  isInterior = true;
    std::string worldspace;

    // Inventory payloads — each is a ready-to-send `fields` object, e.g.
    // {"items":[...]} or {"categories":[...]}. Empty string => not built yet.
    std::string cats, weapons, apparel, aid, notes, misc;

    // Map hotspots — ready-to-send {"hot":[...]} (discovered map markers).
    std::string hotspots;

    // Quest journal — ready-to-send {"quests":[...]}.
    std::string quests;
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
