// FNVWebSocket — NVSE plugin entry points (spike).
//
// On load, starts the WebSocket server on a detached background thread. The
// spike intentionally reads NO game memory, so the background thread is safe.
// Phase 2 adds a main-thread pump (NVSE hook) for actual game-state reads.
#include "nvse_min.h"
#include "ws_server.h"

#include <thread>

extern "C" {

__declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name        = "FNVWebSocket";
    info->version     = 1;
    // Don't load in the GECK/editor.
    if (nvse && nvse->isEditor) return false;
    return true;
}

__declspec(dllexport) bool NVSEPlugin_Load(const NVSEInterface* /*nvse*/) {
    static std::thread server([]() { ws::run_server(8765); });
    server.detach();
    return true;
}

} // extern "C"
