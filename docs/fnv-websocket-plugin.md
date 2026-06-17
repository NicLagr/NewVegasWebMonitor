# FNVWebSocket — NVSE plugin design & protocol spec

Backend for **NewVegasWebMonitor**. A 32-bit Windows NVSE plugin that runs inside
Fallout: New Vegas and exposes the player's live state over the WebSocket protocol
the app already speaks ([../src/api/websocket/protocol.ts](../src/api/websocket/protocol.ts)).
The frontend is finished and game-agnostic; this plugin is the missing half.

> Status: **design only**. No code yet. Build after the feasibility spike (§2) passes.

---

## 1. Target environment (AYN Thor + GameHub Lite)

- **Device:** AYN Thor — Snapdragon (ARM64) Android handheld.
- **Game:** FNV (32-bit Windows x86) running via **GameHub Lite** = Wine + an
  x86→ARM translation layer (box64/box86-style).
- **Plugin:** **32-bit Windows DLL** (`FNVWebSocket.dll`). The ARM translation is
  transparent to it — it's a normal Windows PE that NVSE loads from
  `…/Fallout New Vegas/Data/NVSE/Plugins/`. We do **not** build an ARM/Android binary.
- **App ↔ game are on the SAME device.** The app connects to
  **`ws://127.0.0.1:8765`** (localhost). No PC, no LAN. Wine sockets are backed by
  the host (Android) network stack, so a listener on `0.0.0.0:8765` inside Wine
  should be reachable on Android localhost.
- **Build host:** macOS, cross-compiling with **mingw-w64 (i686)** → a Windows x86
  DLL. No Windows/Visual Studio required (§8).

### App-side changes when the plugin is live
- Set the WS endpoint to `ws://127.0.0.1:8765` (the in-app connection screen, or
  `VITE_WS_URL`), and **unset `VITE_USE_FIXTURES`** so it stops using mock data.
- The MAP tab appears only when the plugin's `system` query advertises the `map`
  feature (§5).
- Worldspace EditorID must match `WORLD_MAP_WORLDSPACE` in
  [../src/stores/map/types.ts](../src/stores/map/types.ts) (currently `WastelandNV`
  — **confirm in-game** and change in one place if different).

---

## 2. Feasibility spike (DO THIS FIRST)

NVSE under GameHub Lite's Wine+translation stack is **unverified**. Validate before
investing in the full plugin. Three independent checks, each a day or less:

1. **Does xNVSE load at all?** Install xNVSE into the GameHub prefix, launch FNV via
   `nvse_loader.exe` (or GameHub's custom-exe option), open console and run
   `GetNVSEVersion`. If NVSE doesn't initialize under box64/Wine, the whole approach
   is blocked — stop and reconsider (fallback options in §11).
2. **Does a trivial plugin load + run?** Build the xNVSE `plugin_example` (or a
   10-line plugin that writes "loaded" to a log file in `My Games`) with mingw and
   confirm it loads. Proves our toolchain produces a DLL NVSE accepts under Wine.
3. **Is a Wine socket reachable from Android localhost?** Tiny plugin that opens a
   winsock listener on `0.0.0.0:8765` and echoes. From an Android terminal / the
   app, connect to `127.0.0.1:8765`. Proves the transport works on-device.

Only after all three pass do we build the real plugin.

---

## 3. Architecture

```
┌────────────────────────── FNV process (Wine, x86) ──────────────────────────┐
│  FNVWebSocket.dll (NVSE plugin)                                              │
│                                                                             │
│   NVSEPlugin_Query / NVSEPlugin_Load   ← NVSE entry points                  │
│                                                                             │
│   ┌─ WS server thread (winsock) ─┐      ┌─ Main-thread pump (NVSE hook) ─┐   │
│   │ • TCP accept on :8765        │      │ • runs each frame / on timer   │   │
│   │ • WebSocket handshake+frames │◄────►│ • reads game state SAFELY      │   │
│   │ • parse client JSON          │ queue│ • builds data payloads         │   │
│   │ • send data/heartbeat/etc.   │      │ • applies commands             │   │
│   └──────────────────────────────┘      └────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │ ws://127.0.0.1:8765
                  ┌────────────▼─────────────┐
                  │  Pip-Boy Companion (app)  │  (PWA/APK on the same Thor)
                  └───────────────────────────┘
```

**Threading rule (critical):** the game mutates its state on its **main thread**.
Reading actor values / inventory from the WS background thread will crash or read
garbage. So:
- WS thread only does socket I/O + JSON (parse incoming, send outgoing strings).
- A **main-thread pump** (an NVSE main-loop hook, or a per-frame event via
  JIP/JohnnyGuitar, or at minimum a low-rate timer marshaled onto the main thread)
  reads game state, builds payloads, and hands serialized strings to the WS thread
  through a mutex-guarded queue.
- Subscriptions are a table of `{id, fields, frequencyMs, lastSentTick}`; the pump
  emits each when due.

Single client is fine (one companion app). Keep it simple.

---

## 4. WebSocket transport

Minimal hand-rolled server (deps are painful under mingw/x86; the protocol is small):
- **Handshake:** read HTTP upgrade, take `Sec-WebSocket-Key`, append the magic GUID
  `258EAFA5-E914-47DA-95CA-C5AB0DC85B11`, SHA-1, base64 → `Sec-WebSocket-Accept`.
- **Framing:** RFC 6455 text frames (opcode 0x1). Client→server frames are masked
  (unmask with the 4-byte key); server→client unmasked. Handle ping/pong (0x9/0xA)
  and close (0x8). Messages are small JSON; fragmentation unlikely but handle
  continuation defensively.
- Keepalive: the app sends `{"type":"heartbeat"}`; reply `{"type":"heartbeat","ts":<ms>}`.

A small JSON lib that compiles clean on mingw-x86 is fine (e.g. a single-header
parser). Avoid anything pulling Boost/OpenSSL.

---

## 5. Protocol contract (authoritative: protocol.ts)

### Client → server
| type | fields | meaning |
|------|--------|---------|
| `subscribe` | `id`, `fields` (alias→registryKey), `settings.frequency?`, `settings.sendOnChange?` | start a periodic data stream |
| `unsubscribe` | `id?` | stop one (or all if omitted) |
| `unsubscribe_all` | — | stop all |
| `query` | `id`, `fields` | one-shot read |
| `heartbeat` | — | keepalive |
| `command` | `command`, `formId?`, `hand?`, `count?`, `slot?`, `active?`, `x?`, `y?`, `z?` | perform a game action |

### Server → client
| type | shape |
|------|-------|
| `data` | `{ type:'data', id, ts, fields:{…} }` — `id` echoes the sub/query id; `fields` keyed by the **alias** the client asked for |
| `heartbeat` | `{ type:'heartbeat', ts }` |
| `error` | `{ type:'error', message }` |
| `commandResult` | `{ type:'commandResult', id, success, error? }` |

**Key point:** the client sends `fields` as `{ alias: "Registry::Key" }`. The plugin
resolves each registry key and returns `fields` keyed by the **alias**. The app's
stores read aliases, so the plugin must echo aliases, not registry keys.

### Subscriptions & field map the app sends
(From [../src/app/config/pageRegistry.ts](../src/app/config/pageRegistry.ts) and the system query.)

- **`system`** (query on connect): `language: Game::Language`, `features: App::Features`
  → return `{ language: "en", features: ["player","inventory","map"] }`. Add
  `"player.quests"` only once the DATA tab is built; add `"player.hotkeys"` for SKILLS.
- **`game.status`** (global, ~100ms, sendOnChange): `status: Game::Status` →
  `{ status: { canAct, controlsEnabled, dead, inCombat, inDialogue, inMainMenu, loading, paused } }`.
  `canAct` gates all gameplay subscriptions app-side — must be `true` when the player
  is in the world.
- **`character.stats`**: `health=ActorValue::kHealth`, `ap=ActorValue::kActionPoints`,
  `rads=ActorValue::kRadiationRads`, `healthBase/apBase/radsMax` (the max for each bar),
  `level=Player::Level`, `xp=Player::XP::Current`, `xpNext=Player::XP::Next`,
  `inventoryWeight=Player::InventoryWeight`, `carryWeight=Player::CarryWeight`,
  `caps=Inventory::Caps`, `karma=Player::Karma`.
- **`inventory.categories`**: `categories=Inventory::Categories` →
  `{ categories: [{ categoryId, count, name }] }`. `categoryId.toLowerCase()` becomes
  the subtab id; the app currently expects `weapons, apparel, potions(label "Aid"),
  books(label "Notes"), misc` (see `subTabsOrderMap`).
- **`inventory.weapons`**: `items=Inventory::Items::Weapons`, `ammo=Inventory::Items::Ammo`.
- **`inventory.apparel`**: `items=Inventory::Items::Apparel`.
- **`inventory.potions`** (AID), **`inventory.books`** (NOTES), **`inventory.misc`**: `items=…`.
- **`map.player`** (~50ms): `position=Player::Position`.
- **`map.hotspots`** (~1s): `hot=Map::Markers::Locations`.
- **`map.questMarkers`** (~1s): `marker=Map::Markers::Quests`.

### Data payload shapes (what the app's stores expect)
These mirror the fixtures ([../public/fixtures.json](../public/fixtures.json)) — the
authoritative example of every shape. Highlights:

- **WeaponItem**: `{ formId, name, categoryType:"Weapon", count, value, weight,
  isFavorite, isStolen, damage, baseDamage, condition (0–100), isEquipped,
  equippedHand, isTwoHanded, weaponType, equipSlots, enchantment }`.
  `weaponType` reuses the engine enum slots (`OneHandSword`=pistol, `TwoHandSword`=rifle,
  etc. — see [../src/shared/lib/constants/weaponIcons.ts](../src/shared/lib/constants/weaponIcons.ts)).
- **ApparelItem**: `{ …base…, damageThreshold (DT), damageResistance?, condition,
  armorType:"Light"|"Heavy"|"Clothing", bodySlots:[…], isEquipped }`.
- **Aid (PotionItem)** / **Notes (BookItem)** / **MiscItem**: see fixtures.
- **CharacterStats**: the numeric fields above (null/undefined allowed).
- **PlayerPosition**: `{ x, y, z, angle (radians, 0=N, CW+), isInterior,
  worldspace, worldspaceFormId, parentWorldspace, parentWorldspaceFormId, cell, cellFormId }`.
  Only plotted when `worldspace===parentWorldspace===WastelandNV` and not interior.
- **MapHotspot**: `{ type, typeId, refId (hex form id), name, x, y, isVisible,
  canFastTravel }`. `type` ∈ Vault/City/Town/Settlement/Cave/Factory/MilitaryBase/
  Camp/Building/Ruins/Mine/GasStation/Monument/Casino (icon map in
  [../src/pages/map/composables/useMapMarkerIcons.ts](../src/pages/map/composables/useMapMarkerIcons.ts)).

### Commands (server applies, replies commandResult)
`equip`/`unequip` (formId, hand?), `use` (formId), `drop` (formId, count?),
`favorite` (formId), `hotkey_set` (formId, slot 1–8), `hotkey_clear`/`hotkey_trigger`
(slot), `quest_set_active` (formId, active), `player_marker_set` (x,y,z?),
`player_marker_clear`, `fast_travel` (formId = the map marker's refId).

---

## 6. Reading game state (NVSE / game API)

Use the player singleton and game forms. Most needs community game-struct headers
(xNVSE `GameAPI`, or JIP LN / JohnnyGuitar references). Difficulty noted.

| Need | Approach | Difficulty |
|------|----------|------------|
| Player ref | `PlayerCharacter::GetSingleton()` (`g_thePlayer`) | easy |
| HP / AP / Rads (+max) | `player->avOwner.GetActorValue(code)` / `GetBaseActorValue`; AV codes: Health, ActionPoints, RadiationRads | easy |
| Level / XP | player level field; XP + XP-to-next via game globals | easy |
| Caps | count of **Caps001** (`0x0000000F`) in player inventory | easy |
| Karma | player karma actor value/global | medium |
| Carry/inventory weight | `GetActorValue` carry weight + computed inventory weight | medium |
| Inventory items | walk `ExtraContainerChanges` on the player; per item read base form + extra data (count, equipped, **health/condition** via `ExtraHealth`, favorite, etc.) | hard |
| Weapon DAM / type | base `TESObjectWEAP` fields | medium |
| Apparel DT/DR | base `TESObjectARMO` fields | medium |
| Position / worldspace | `player->posX/Y/Z`, `player->rotZ` (→radians), `player->GetWorldSpace()->GetEditorID()`, interior check via current cell | medium |
| Map markers | iterate refs with `ExtraMapMarker`; read name, marker type, `canFastTravel`, world x/y, refId | hard |
| Game status (canAct…) | menu mode (`MenuMode`), `IsInCombat`, dialogue/loading/dead flags | medium |
| fast_travel | move player to marker ref (replicate the map's travel), or `CenterOnCell`/translate | hard |

> Inventory + map-marker enumeration are the heavy lifts. Stats/position/status are
> quick wins — build those first (§10).

### Map coordinates — remove app-side calibration
The app currently uses a hand-calibrated affine (you clicked landmarks). Two ways the
plugin makes that automatic (the in-game world map is orthographic = a clean affine):
- **Send worldspace bounds**: include the Mojave worldspace's usable min/max world
  X/Y once; the app derives the exact transform from bounds + image size. ~2 numbers,
  no clicking.
- **Send normalized coords**: project positions to 0–1 map space in the plugin (the
  game already computes this for its Pip-Boy map) and the app just scales to pixels.
  Zero calibration.
Either is a small frontend follow-up; pick when we get there.

---

## 7. Message-handling flow (pseudo)

```
on WS message (WS thread):
  parse JSON → enqueue {kind, payload}     // never touch game state here

main-thread pump (each frame / timer):
  drain queue:
    subscribe   → add to sub table
    unsubscribe → remove
    query       → read now, enqueue outgoing data
    command     → apply now, enqueue commandResult
  for each active sub due this tick:
    read fields, build {type:'data', id, ts, fields}, enqueue outgoing

WS thread:
  drain outgoing queue → frame + send
```

---

## 8. Toolchain & build (macOS → Windows x86 DLL)

- `brew install mingw-w64` (provides `i686-w64-mingw32-g++`).
- CMake toolchain targeting `i686-w64-mingw32`; build a **32-bit** shared lib.
- Link `ws2_32` (winsock). Static-link libstdc++/libgcc/winpthread
  (`-static -static-libgcc -static-libstdc++`) so the DLL has no runtime deps in the
  Wine prefix.
- NVSE plugin headers from the **xNVSE** repo (`nvse/` + `PluginAPI.h`); export
  `NVSEPlugin_Query`, `NVSEPlugin_Load` with C linkage and the correct calling
  convention; match the plugin opcode/version block.
- Output `FNVWebSocket.dll` → copy into GameHub's Wine prefix:
  `…/Fallout New Vegas/Data/NVSE/Plugins/FNVWebSocket.dll`.
- Proposed repo layout: `plugin/` (CMakeLists, `src/`, vendored `nvse/` headers,
  README with the exact build + install commands).

> Risk: NVSE’s headers/ABI assume MSVC in places. If mingw can’t satisfy the ABI,
> fallback is building the DLL with MSVC in a Windows VM/CI and only *running* it on
> the Thor. The spike (§2.2) settles this.

---

## 9. App-side follow-ups (small)
- Point WS endpoint at `ws://127.0.0.1:8765`; unset `VITE_USE_FIXTURES`.
- Confirm/adjust `WORLD_MAP_WORLDSPACE`.
- Optional: switch map projection to bounds-based or normalized coords (§6) to drop
  the manual calibration table.
- Everything else (stores, pages, markers, fast-travel dispatch) already matches the
  contract — no changes.

---

## 10. Phased build plan (each phase verifiable against the live app)
0. **Spike** (§2) — NVSE loads, plugin loads, socket reachable.
1. **Transport + handshake** — WS server, heartbeat, `system` query (return features),
   `game.status` (return `canAct:true` in-world). App connects, shows tabs.
2. **STATUS** — `character.stats`. HP/AP/Rads/Caps/Level/XP go live. (Quick win.)
3. **Position + MAP** — `map.player` (+ worldspace bounds for auto-calibration), then
   `map.hotspots` / `map.questMarkers`. Player dot + markers live; fast_travel command.
4. **Inventory** — categories + weapons/apparel/aid/notes/misc, with condition/DT.
5. **Commands** — equip/use/drop/favorite/hotkeys; commandResult.
6. **Polish** — sendOnChange throttling, reconnect robustness, real weaponType/marker-type mapping.

---

## 11. Risks & fallbacks
- **NVSE under GameHub/box64 may not work.** Primary risk. Mitigation: spike first.
  Fallbacks: (a) run FNV on a real Windows PC and the app on the Thor over WiFi (the
  original architecture — fully supported, just not single-device); (b) if NVSE is OK
  but plugins aren’t, explore an NVSE *script*-based bridge (limited).
- **mingw vs MSVC ABI** for NVSE headers (§8) — fallback: MSVC build, Thor run.
- **Main-thread hook under translation** — per-frame hooks may behave differently
  under box64; the low-rate timer fallback trades latency for safety.
- **Socket binding/permissions** inside GameHub’s container — settled by spike §2.3.
- **Performance** — x86→ARM translation makes per-frame reads costlier; keep
  frequencies modest (stats 200ms, position 50–100ms) and prefer sendOnChange.
```
