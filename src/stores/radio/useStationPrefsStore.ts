import { defineStore } from 'pinia';
import { ref } from 'vue';

/**
 * Per-station visibility overrides, keyed by station NAME (stable across
 * load-order/refId changes) and persisted to localStorage.
 *
 * By default a station's visibility follows the plugin's `receivable` flag
 * (whether the Pip-Boy would currently list it). An override lets the user
 * force a station visible (`true`) or hidden (`false`) regardless — so they can
 * surface a station the auto-filter dropped, or hide one it kept.
 */
const STORAGE_KEY = 'nvwm-station-prefs';

function load(): Record<string, boolean> {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    const obj = raw ? JSON.parse(raw) : {};
    return obj && typeof obj === 'object' ? obj : {};
  } catch {
    return {};
  }
}

export const useStationPrefsStore = defineStore('stationPrefs', () => {
  const overrides = ref<Record<string, boolean>>(load());

  function persist(): void {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(overrides.value));
    } catch {
      /* storage may be unavailable */
    }
  }

  /** Effective visibility: explicit override if set, else the default. */
  function isVisible(name: string, fallback: boolean): boolean {
    return name in overrides.value ? overrides.value[name] : fallback;
  }

  function show(name: string): void {
    overrides.value = { ...overrides.value, [name]: true };
    persist();
  }

  function hide(name: string): void {
    overrides.value = { ...overrides.value, [name]: false };
    persist();
  }

  return { overrides, isVisible, show, hide };
});
