// FNVWebSocket — NVSE plugin entry points (Phase 2a: live character stats).
//
// Reads player actor values on the GAME MAIN THREAD via NVSE's kMessage_MainGameLoop
// hook, stashes them in a thread-safe snapshot, and the WebSocket server thread
// serializes that snapshot to the app. Reading game memory off the main thread is
// unsafe — hence the message hook.
//
// Built against the real xNVSE headers (fetched in CI). g_thePlayer is defined here
// at its runtime address so we don't have to compile the whole xNVSE source tree.
#include "nvse/PluginAPI.h"
#include "nvse/GameAPI.h"
#include "nvse/GameObjects.h"

#include "snapshot.h"
#include "ws_server.h"

#include <thread>

// Runtime address of the player pointer (from xNVSE GameObjects.cpp).
PlayerCharacter** g_thePlayer = (PlayerCharacter**)0x011DEA3C;

static PluginHandle            g_pluginHandle = kPluginHandle_Invalid;
static NVSEMessagingInterface* g_messaging    = nullptr;

// Read player stats. MUST run on the main thread (called from the game-loop message).
static void ReadPlayerStats() {
    GameSnapshot s;
    PlayerCharacter* p = g_thePlayer ? *g_thePlayer : nullptr;
    if (!p || !p->parentCell) { snapshot_set(s); return; } // not in-world yet

    s.inGame = true;
    ActorValueOwner& av = p->avOwner;
    s.level     = (int)av.Fn_0A();           // GetLevel
    // Fn_03 = current actor value, Fn_08 = permanent/base (used for the bar max).
    s.health    = av.Fn_03(eActorVal_Health);
    s.healthMax = av.Fn_08(eActorVal_Health);
    s.ap        = av.Fn_03(eActorVal_ActionPoints);
    s.apMax     = av.Fn_08(eActorVal_ActionPoints);
    s.rads      = av.Fn_03(eActorVal_RadLevel);
    s.radsMax   = 1000.0f; // FNV rads cap
    s.xp        = av.Fn_03(eActorVal_XP);
    s.carryWeight = av.Fn_08(eActorVal_CarryWeight);
    s.invWeight   = av.Fn_03(eActorVal_InventoryWeight);
    s.karma       = av.Fn_03(eActorVal_Karma);
    // TODO Phase 2b: level (base form), caps (inventory count of 0x0000000F).
    snapshot_set(s);
}

static void MessageHandler(NVSEMessagingInterface::Message* msg) {
    if (msg->type == NVSEMessagingInterface::kMessage_MainGameLoop) {
        ReadPlayerStats();
    }
}

extern "C" {

__declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name        = "FNVWebSocket";
    info->version     = 1;
    if (nvse->isEditor) return false;
    return true;
}

__declspec(dllexport) bool NVSEPlugin_Load(NVSEInterface* nvse) {
    g_pluginHandle = nvse->GetPluginHandle();

    g_messaging = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
    if (g_messaging)
        g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);

    // WebSocket server on a detached background thread (socket I/O + JSON only;
    // it reads the snapshot, never game memory directly).
    static std::thread server([]() { ws::run_server(8765); });
    server.detach();
    return true;
}

} // extern "C"
