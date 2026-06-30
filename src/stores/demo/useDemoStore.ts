import { defineStore } from 'pinia';
import { reactive, ref } from 'vue';

import { DataRouter } from '@/stores/adapters/dataRouter';
import { useSystemStore } from '@/stores/system/useSystemStore';
import { buildDemoData } from './demoData';

/**
 * True only if the app was opened with a `?demo` query param. Demo activation
 * is intentionally NOT persisted to localStorage — the plain URL must always
 * land on the live connection screen. `enter()` writes `?demo` into the URL so
 * the choice survives reloads, and `exit()` strips it again.
 */
export function isDemoRequested(): boolean {
  try {
    return new URLSearchParams(window.location.search).has('demo');
  } catch {
    return false;
  }
}

/**
 * Demo Mode — lets the app be explored without a running game by feeding an
 * in-memory mock dataset through the same DataRouter path the live socket uses.
 * All demo-only UI (editor, controls) must gate on `isActive` so it never shows
 * during real, connected use.
 */
export const useDemoStore = defineStore('demo', () => {
  const isActive = ref(false);
  /** The live, editable mock dataset (id -> fields). Mutated by the demo editor. */
  const data = reactive<Record<string, Record<string, unknown>>>(buildDemoData());

  /**
   * Push a single dataset key into its store via the normal routing path.
   * The fields are deep-cloned so each apply hands the store a fresh object —
   * otherwise re-applying the same (mutated) reactive reference wouldn't trip
   * Vue's ref reassignment and the UI wouldn't update.
   */
  function routeKey(id: string): void {
    const fields = data[id];
    if (!fields) return;
    const snapshot = JSON.parse(JSON.stringify(fields)) as Record<string, unknown>;
    if (id === 'system') {
      useSystemStore().handleQueryResponse(snapshot);
      return;
    }
    DataRouter.routeDataById(id, snapshot);
  }

  /** Apply the whole dataset (categories first, so subtabs register before content). */
  function apply(): void {
    const ids = Object.keys(data).sort((a, b) => {
      const aCat = a.endsWith('.categories') ? 0 : 1;
      const bCat = b.endsWith('.categories') ? 0 : 1;
      return aCat - bCat;
    });
    for (const id of ids) routeKey(id);
  }

  /** Enter demo mode: mark the URL shareable (so reloads stay in demo) and load mock data. */
  function enter(): void {
    isActive.value = true;
    try {
      const url = new URL(window.location.href);
      if (!url.searchParams.has('demo')) {
        url.searchParams.set('demo', '1');
        window.history.replaceState({}, '', url);
      }
    } catch {
      /* history unavailable — non-fatal */
    }
    apply();
  }

  /** Leave demo mode and return to the live connection flow (full reload for a clean slate). */
  function exit(): void {
    try {
      const url = new URL(window.location.href);
      url.searchParams.delete('demo');
      window.location.href = url.toString();
    } catch {
      window.location.reload();
    }
  }

  /** Restore the original dataset and re-apply it (discards editor changes). */
  function reset(): void {
    const fresh = buildDemoData();
    for (const key of Object.keys(data)) delete data[key];
    Object.assign(data, fresh);
    apply();
  }

  return { isActive, data, enter, exit, apply, reset, routeKey };
});

export default useDemoStore;
