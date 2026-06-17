// Minimal WebSocket server for the FNVWebSocket spike.
//
// SPIKE SCOPE: proves the DLL loads, the socket is reachable from the app, and
// the NewVegasWebMonitor app connects + renders. It answers `system` + `game.status`
// + heartbeat with hardcoded data and replies success to commands. It does NOT
// read game memory yet — that is Phase 2, and MUST happen on the game's main
// thread (this server runs on a background thread, which is only safe precisely
// because it touches no game state).
#pragma once

namespace ws {
// Blocks: binds 0.0.0.0:<port>, accepts one client at a time, serves until the
// client disconnects, then re-accepts. Call on a detached background thread.
void run_server(int port);
}
