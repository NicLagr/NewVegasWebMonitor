import { defineStore } from 'pinia';
import { ref } from 'vue';

/**
 * Stations the user has chosen to hide from the radio picker. Keyed by station
 * NAME (stable across load-order/refId changes) and persisted to localStorage,
 * since the engine can't reliably distinguish "real" Pip-Boy stations from
 * ambient/mod broadcasts — the user curates instead.
 */
const STORAGE_KEY = 'nvwm-hidden-stations';

function load(): string[] {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    const arr = raw ? JSON.parse(raw) : [];
    return Array.isArray(arr) ? arr.filter((s) => typeof s === 'string') : [];
  } catch {
    return [];
  }
}

export const useHiddenStationsStore = defineStore('hiddenStations', () => {
  const hidden = ref<Set<string>>(new Set(load()));

  function persist(): void {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify([...hidden.value]));
    } catch {
      /* storage may be unavailable */
    }
  }

  function isHidden(name: string): boolean {
    return hidden.value.has(name);
  }

  function hide(name: string): void {
    hidden.value.add(name);
    hidden.value = new Set(hidden.value); // reassign for reactivity
    persist();
  }

  function unhide(name: string): void {
    hidden.value.delete(name);
    hidden.value = new Set(hidden.value);
    persist();
  }

  return { hidden, isHidden, hide, unhide };
});
