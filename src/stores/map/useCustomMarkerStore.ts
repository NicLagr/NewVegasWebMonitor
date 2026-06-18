import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { Point } from '@/pages/map/composables/useMapProjection';

/**
 * A single user-placed marker on the map, in raw game-world coordinates (so it
 * projects with the same affine transform as discovered hotspots and the player
 * dot). Persisted to localStorage so it survives reloads. Purely a companion-app
 * annotation — it is not pushed into the in-game Pip-Boy map.
 */
const STORAGE_KEY = 'nvwm-custom-marker';

function load(): Point | null {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return null;
    const p = JSON.parse(raw);
    if (p && Number.isFinite(p.x) && Number.isFinite(p.y)) return { x: p.x, y: p.y };
  } catch {
    /* ignore corrupt storage */
  }
  return null;
}

export const useCustomMarkerStore = defineStore('customMarker', () => {
  /** World-coord position of the custom marker, or null when none is placed. */
  const marker = ref<Point | null>(load());

  function persist(): void {
    try {
      if (marker.value) localStorage.setItem(STORAGE_KEY, JSON.stringify(marker.value));
      else localStorage.removeItem(STORAGE_KEY);
    } catch {
      /* storage may be unavailable (private mode); marker still works in-session */
    }
  }

  /** Place (or move) the custom marker to a world-coordinate point. */
  function setMarker(point: Point): void {
    marker.value = { x: point.x, y: point.y };
    persist();
  }

  /** Remove the custom marker. */
  function clearMarker(): void {
    marker.value = null;
    persist();
  }

  return { marker, setMarker, clearMarker };
});
