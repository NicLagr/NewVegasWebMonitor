#!/usr/bin/env node

/**
 * Generates a placeholder Mojave map at public/mojave-map.png so the MAP tab
 * renders before you drop in the real game map. Desert-toned background with a
 * grid + center crosshair. Size defaults to 4096×4096 to match the example
 * calibration in useMapCoordinates.ts / MOJAVE_MAP_IMAGE_WIDTH in useMapProjection.ts.
 *
 * Run: node scripts/make-placeholder-map.js   (or: npm run map:placeholder)
 * Replace public/mojave-map.png with the real Mojave world map when ready.
 */

import { deflateSync } from 'node:zlib';
import { writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const SIZE = parseInt(process.env.MAP_SIZE || '4096', 10);
const GRID = 256;

const BG = [120, 100, 78];     // desert tan
const LINE = [92, 77, 60];     // darker grid
const CROSS = [0, 255, 65];    // Pip-Boy green crosshair

// --- CRC32 (PNG chunk checksums) ---
const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();
function crc32(buf) {
  let c = 0xffffffff;
  for (let i = 0; i < buf.length; i++) c = CRC_TABLE[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}
function chunk(type, data) {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length, 0);
  const typeBuf = Buffer.from(type, 'ascii');
  const body = Buffer.concat([typeBuf, data]);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(body), 0);
  return Buffer.concat([len, body, crc]);
}

// --- Raw RGBA scanlines (each prefixed with filter byte 0) ---
const rowLen = 1 + SIZE * 3; // 8-bit RGB
const raw = Buffer.alloc(rowLen * SIZE);
const near = (v, m) => Math.abs(v) <= m;
const cx = SIZE / 2;
const cy = SIZE / 2;

for (let y = 0; y < SIZE; y++) {
  const rowStart = y * rowLen;
  raw[rowStart] = 0; // filter: none
  const isGridRow = y % GRID === 0;
  for (let x = 0; x < SIZE; x++) {
    let col = BG;
    if (isGridRow || x % GRID === 0) col = LINE;
    // center crosshair lines
    if (near(x - cx, 2) || near(y - cy, 2)) col = CROSS;
    const p = rowStart + 1 + x * 3;
    raw[p] = col[0];
    raw[p + 1] = col[1];
    raw[p + 2] = col[2];
  }
}

const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(SIZE, 0);
ihdr.writeUInt32BE(SIZE, 4);
ihdr[8] = 8;  // bit depth
ihdr[9] = 2;  // color type: RGB
ihdr[10] = 0; // compression
ihdr[11] = 0; // filter
ihdr[12] = 0; // interlace

const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
  chunk('IHDR', ihdr),
  chunk('IDAT', deflateSync(raw, { level: 6 })),
  chunk('IEND', Buffer.alloc(0)),
]);

const out = resolve(__dirname, '../public/mojave-map.png');
writeFileSync(out, png);
console.log(`[make-placeholder-map] wrote ${out} (${SIZE}x${SIZE}, ${(png.length / 1024).toFixed(0)} KB)`);
