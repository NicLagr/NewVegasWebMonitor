#!/usr/bin/env node

/**
 * NewVegasWebMonitor — mock WebSocket backend.
 *
 * Speaks the exact protocol the real FNVWebSocket NVSE plugin will implement
 * (see src/api/websocket/protocol.ts), so you can test the app over a real
 * network — including the AYN Thor APK — before the game-side plugin exists.
 *
 * It serves the data in public/fixtures.json, jitters the STATUS bars so the
 * UI visibly updates, and answers the `system` + `inventory.categories`
 * queries the app sends on connect.
 *
 * Run:   npm run mock:server
 * Then point the app at  ws://<this-machine-LAN-IP>:8765
 * (in dev: set VITE_WS_SERVER_URL=http://localhost:8765 and unset VITE_USE_FIXTURES;
 *  on the Thor APK: type the PC's IP:8765 into the connection screen).
 */

import { WebSocketServer } from 'ws';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PORT = parseInt(process.env.MOCK_PORT || '8765', 10);

const fixtures = JSON.parse(
  readFileSync(resolve(__dirname, '../public/fixtures.json'), 'utf-8')
);

// Features the app should believe the backend provides. STATUS + inventory are
// always-on; 'map' reveals the MAP tab. Omitting 'player.quests' keeps DATA
// hidden (that tab isn't built out yet).
const FEATURES = ['player', 'inventory', 'map'];

// Deep-clone helper so per-connection jitter never mutates the shared fixtures.
const clone = (v) => JSON.parse(JSON.stringify(v));

/** Return the `fields` payload for a given subscription/query id. */
function fieldsFor(id, tick = 0) {
  if (id === 'system') {
    return { language: 'en', features: FEATURES };
  }
  if (id === 'map.player') {
    // Orbit the player around the base position so the marker visibly moves.
    const base = clone(fixtures['map.player'] ?? {});
    const pos = base.position ?? {};
    const t = tick * 0.15;
    const R = 12000;
    return {
      position: {
        ...pos,
        x: (pos.x ?? 6000) + R * Math.cos(t),
        y: (pos.y ?? 6000) + R * Math.sin(t),
        angle: t % (Math.PI * 2),
      },
    };
  }
  if (id === 'character.stats') {
    // Jitter HP/AP/Rads a little each push so the bars move like a live game.
    const s = clone(fixtures['character.stats'] ?? {});
    const jitter = (val, min, max) => {
      if (typeof val !== 'number') return val;
      const next = val + Math.round((Math.random() - 0.5) * 12);
      return Math.max(min, Math.min(max, next));
    };
    s.health = jitter(s.health, 0, s.healthBase ?? 220);
    s.ap = jitter(s.ap, 0, s.apBase ?? 105);
    s.rads = jitter(s.rads, 0, s.radsMax ?? 1000);
    return s;
  }
  return fixtures[id] ?? null;
}

const send = (ws, obj) => {
  if (ws.readyState === ws.OPEN) ws.send(JSON.stringify(obj));
};
const now = () => Date.now();

const wss = new WebSocketServer({ port: PORT });

wss.on('listening', () => {
  console.log(`[mock-server] listening on ws://0.0.0.0:${PORT}`);
  console.log('[mock-server] point the app here. Ctrl+C to stop.');
});

wss.on('connection', (ws, req) => {
  console.log(`[mock-server] client connected (${req.socket.remoteAddress})`);
  const timers = new Map(); // subscriptionId -> interval handle
  const ticks = new Map(); // subscriptionId -> push counter (for animation)

  const pushData = (id) => {
    const tick = (ticks.get(id) ?? 0) + 1;
    ticks.set(id, tick);
    const fields = fieldsFor(id, tick);
    if (fields === null) {
      console.warn(`[mock-server] no fixture for "${id}" — sending empty`);
    }
    send(ws, { type: 'data', id, ts: now(), fields: fields ?? {} });
  };

  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      send(ws, { type: 'error', message: 'invalid JSON' });
      return;
    }

    switch (msg.type) {
      case 'heartbeat':
        send(ws, { type: 'heartbeat', ts: now() });
        break;

      case 'query':
        // One-shot: answer immediately with a data message keyed by the query id.
        pushData(msg.id);
        break;

      case 'subscribe': {
        // Push once now, then on the requested interval (clamped to >=200ms).
        const freq = Math.max(200, Number(msg.settings?.frequency) || 500);
        pushData(msg.id);
        clearInterval(timers.get(msg.id));
        timers.set(msg.id, setInterval(() => pushData(msg.id), freq));
        console.log(`[mock-server] subscribed "${msg.id}" @ ${freq}ms`);
        break;
      }

      case 'unsubscribe':
        if (msg.id) {
          clearInterval(timers.get(msg.id));
          timers.delete(msg.id);
        }
        break;

      case 'unsubscribe_all':
        for (const t of timers.values()) clearInterval(t);
        timers.clear();
        break;

      case 'command':
        // Pretend every action succeeded.
        console.log(`[mock-server] command "${msg.command}" (formId=${msg.formId ?? '-'})`);
        send(ws, { type: 'commandResult', id: msg.id, success: true });
        break;

      default:
        send(ws, { type: 'error', message: `unknown message type: ${msg.type}` });
    }
  });

  ws.on('close', () => {
    for (const t of timers.values()) clearInterval(t);
    timers.clear();
    console.log('[mock-server] client disconnected');
  });
});
