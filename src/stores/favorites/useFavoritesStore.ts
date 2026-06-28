import { defineStore } from 'pinia';
import { ref } from 'vue';

/**
 * App-local favorites, persisted by item formId. Fallout: New Vegas has no
 * Skyrim-style "favorite" game state, so favoriting is purely client-side: it
 * pins items to the top of their list. Survives reloads via localStorage.
 */
const KEY = 'nvwm-favorites';

function load(): Set<string> {
  try {
    const raw = localStorage.getItem(KEY);
    if (raw) return new Set(JSON.parse(raw) as string[]);
  } catch {
    /* ignore */
  }
  return new Set();
}

export const useFavoritesStore = defineStore('favorites', () => {
  const ids = ref<Set<string>>(load());

  function persist(): void {
    try {
      localStorage.setItem(KEY, JSON.stringify([...ids.value]));
    } catch {
      /* storage may be unavailable */
    }
  }

  function has(formId: string): boolean {
    return ids.value.has(formId);
  }

  function toggle(formId: string): void {
    // Reassign the Set so dependent computeds re-run.
    const next = new Set(ids.value);
    if (next.has(formId)) next.delete(formId);
    else next.add(formId);
    ids.value = next;
    persist();
  }

  return { ids, has, toggle };
});
