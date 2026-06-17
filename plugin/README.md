# FNVWebSocket (NVSE plugin) — spike

A 32-bit Windows NVSE plugin that hosts the WebSocket backend the NewVegasWebMonitor
app connects to. **This is the go/no-go spike**, not the full plugin: it proves the
DLL loads under GameHub Lite, the socket is reachable on the Thor, and the app
connects + renders. It answers `system` + `game.status` + heartbeat with hardcoded
data and acks commands — it does **not** read game memory yet.

Full design / protocol / phases: [../docs/fnv-websocket-plugin.md](../docs/fnv-websocket-plugin.md)

## Build (no local Windows needed)
Built by GitHub Actions with MSVC on a free Windows runner.

1. Commit & push the `plugin/` folder (or run the **build-plugin** workflow manually
   via the Actions tab → "Run workflow").
2. Open the finished run → **Artifacts** → download `FNVWebSocket-dll`.
3. Inside is `FNVWebSocket.dll` (32-bit, static MSVC runtime).

Local MSVC build (if you ever have Windows):
```
cmake -S plugin -B plugin/build -A Win32
cmake --build plugin/build --config Release
# -> plugin/build/Release/FNVWebSocket.dll
```

## Install on the Thor (GameHub Lite)
Copy the DLL into the Wine prefix, next to your other NVSE plugins:
```
.../Fallout New Vegas/Data/NVSE/Plugins/FNVWebSocket.dll
```
(Same folder as JIP LN NVSE / JohnnyGuitar — if those load, this should too.)

## Test (the go/no-go)
1. Launch FNV through GameHub as usual (via NVSE), load a save so you're in the world.
2. Confirm the plugin loaded + is listening: a `FNVWebSocket.log` file appears in the
   FNV working directory containing `[ws] listening on 0.0.0.0:8765`.
3. In the NewVegasWebMonitor app on the Thor:
   - Set the WS address to `127.0.0.1:8765` (connection screen), and make sure
     `VITE_USE_FIXTURES` is **off** (use a real build, not the fixtures dev build).
   - It should connect — the log shows `[ws] client connected (handshake ok)` — and
     the app shows the STATUS / INV / MAP tabs (because `system` reports those
     features and `game.status` reports `canAct:true`).
4. Expected: tabs render, no live numbers yet (STATUS shows `-`, inventory empty).
   **That's success** — transport + load + reachability all proven.

If step 2's log never appears → NVSE isn't loading our DLL (ABI/version issue).
If the log shows `listening` but the app won't connect → socket reachability inside
GameHub's container (see the design doc's risks/fallbacks).

## Next (Phase 2, after this passes)
Swap `nvse_min.h` for the real xNVSE headers, add a main-thread pump, and fill
`fields_for()` in `src/ws_server.cpp` with real reads (JIP LN / JohnnyGuitar make
stats, inventory, position, and map markers far easier). See the design doc.
