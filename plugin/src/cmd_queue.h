// Thread-safe command queue: the WebSocket thread enqueues game actions
// (equip/use/...) and the main game-loop thread drains + executes them, since
// game mutations must happen on the main thread.
#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <cstdint>

struct PluginCmd {
    std::string type;   // "equip" | "unequip" | "use" | "drop" | ...
    uint32_t    formId; // hex form id of the target item
    int         count;  // for drop
};

inline std::mutex&                cmd_mutex() { static std::mutex m; return m; }
inline std::vector<PluginCmd>&    cmd_store() { static std::vector<PluginCmd> q; return q; }

inline void cmd_push(const std::string& type, uint32_t formId, int count) {
    std::lock_guard<std::mutex> lock(cmd_mutex());
    cmd_store().push_back({ type, formId, count });
}
inline std::vector<PluginCmd> cmd_drain() {
    std::lock_guard<std::mutex> lock(cmd_mutex());
    std::vector<PluginCmd> out;
    out.swap(cmd_store());
    return out;
}
