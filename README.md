# NewVegasWebMonitor

A Vue 3 + Vite + Capacitor **companion app for *Fallout: New Vegas*** — a Pip-Boy-style
second screen that shows your character in real time: **status, inventory, and the
Mojave map with fast travel**, streamed from the game over WebSocket.

> **Live PWA:** https://niclagr.github.io/NewVegasWebMonitor/
>
> Built for the **[AYN Thor](https://www.ayntec.com/)** (running FNV via GameHub Lite),
> but it runs in any modern browser — desktop, phone, tablet.

Adapted from [andreyvelsk/SkyrimWebMonitor](https://github.com/andreyvelsk/SkyrimWebMonitor)
(MIT). The WebSocket protocol is unchanged; the UI, data models, theme, and map were
re-targeted to New Vegas.

---

## How it works

The app is a **WebSocket client**. The game state is served by an **NVSE plugin**
(`FNVWebSocket`, in [`plugin/`](plugin/)) that runs inside FNV and exposes player data
on `ws://127.0.0.1:8765`. The app connects and renders it live.

```
[ FNV + FNVWebSocket NVSE plugin ]  ──ws://…:8765──▶  [ NewVegasWebMonitor app ]
        (the server)                                        (this repo)
```

- **Same device (AYN Thor / GameHub Lite):** game and app both on the Thor → `127.0.0.1`.
- **PC + handheld:** FNV on a Windows PC, app on the Thor over WiFi → `<pc-ip>:8765`.

The plugin is currently at the **spike stage** (transport proven; live game-data reads
are the next phase). Full design & protocol: [`docs/fnv-websocket-plugin.md`](docs/fnv-websocket-plugin.md).

## Features

- **STATUS** — HP / AP / Rads bars, Level, XP, Caps, Karma, carry weight
- **INV** — Weapons (condition %), Apparel (Damage Threshold), Aid/Chems, Notes/Holotapes, Misc
- **MAP** — Mojave Wasteland map with location + quest markers and tap-to-**fast-travel**
- Pip-Boy green-on-black terminal theme with CRT scanlines, monospace UI
- Installable PWA (home-screen, offline shell) and Android APK
- Game-agnostic WebSocket layer — the app doesn't care which build of the plugin it talks to

> SKILLS (S.P.E.C.I.A.L.) and DATA (quests) tabs are scaffolded and feature-gated off
> for now.

## Download & try it

The current builds are a **self-contained demo** (bundled mock data) — open and you get
the full UI + Mojave map with markers, **no game or plugin required**.

- **PWA (no install):** open **https://niclagr.github.io/NewVegasWebMonitor/** and
  "Add to Home Screen".
- **Android APK:** GitHub **Actions → "Build Android APK" → Artifacts →
  `newvegaswebmonitor-debug-apk`**. Unzip, install `app-debug.apk` (enable "install from
  unknown sources"). Debug-signed.
- **NVSE plugin DLL:** Actions → **"build-plugin" → `FNVWebSocket-dll`** (32-bit Windows).

To make a build that connects to the **live plugin** instead of mock data, remove the
`VITE_USE_FIXTURES: 'true'` env from the build step in
[`.github/workflows/android-apk.yml`](.github/workflows/android-apk.yml).

## Run locally / develop

Requires Node.js 20+.

```bash
npm install

# A) Offline demo with mock fixtures (no backend):
echo "VITE_USE_FIXTURES=true" > .env.local
npm run dev            # http://localhost:5173/NewVegasWebMonitor/

# B) Live against a protocol-accurate mock server (real WebSocket path):
npm run mock:server                       # ws://localhost:8765, streams Mojave data
#   then in .env.local: VITE_WS_SERVER_URL=http://localhost:8765 (and remove VITE_USE_FIXTURES)
npm run dev
```

Other scripts:

```bash
npm run build            # production web build → dist/
npm run tsc              # type-check
npm run lint             # ESLint --fix
npm run map:placeholder  # regenerate a placeholder public/mojave-map.png
npm run android:apk      # local APK build (needs Android SDK + JDK 17)
```

### Swapping the map image

The world map is a single image at `public/mojave-map.png`. Replace it with your own
Mojave render, set `MOJAVE_MAP_IMAGE_WIDTH/HEIGHT` in
[`src/pages/map/composables/useMapProjection.ts`](src/pages/map/composables/useMapProjection.ts),
then tap ~5 landmarks on the MAP tab (each logs `[map] image px: …`) and paste those into
`REFERENCE_POINTS` in
[`src/pages/map/composables/useMapCoordinates.ts`](src/pages/map/composables/useMapCoordinates.ts).

## The FNVWebSocket plugin

See [`plugin/README.md`](plugin/README.md) for build (free GitHub Actions MSVC),
install (`Data/NVSE/Plugins/`), and the go/no-go test, and
[`docs/fnv-websocket-plugin.md`](docs/fnv-websocket-plugin.md) for the full design,
protocol contract, and phased plan.

## Tech stack

Vue 3 (Composition API) + TypeScript · Vite · Pinia · vue-i18n · vite-plugin-pwa
(Workbox) · OpenSeadragon (map) · Capacitor (Android) · C++ NVSE plugin (backend).

## Credits

- Adapted from **[SkyrimWebMonitor](https://github.com/andreyvelsk/SkyrimWebMonitor)** by
  andreyvelsk (MIT) — original app + WebSocket protocol.
- Generic UI icons from [game-icons.net](https://game-icons.net/) — [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/); per-author attribution preserved in `public/icons/` folder names.
- Per-item Pip-Boy icons in `public/icons/pipboy/` are from **[Consistent Pip-Boy Icons](https://www.nexusmods.com/profile/ItsMeJesusHChrist)** by **ItsMeJesusHChrist**, used with permission (credit required, non-commercial). The original DDS textures were converted to WebP for web use; the raw source set is not redistributed.
- *Fallout: New Vegas* is © Bethesda Softworks / Obsidian Entertainment. This is an
  unofficial fan-made companion app, not affiliated with or endorsed by either. Game map
  imagery is property of its respective owners and used here for personal/non-commercial use.

## License

[MIT](LICENSE).
