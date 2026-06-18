import { defineStore } from 'pinia';
import { ref } from 'vue';

export interface RadioStation {
  refId: string;
  name: string;
  /** Whether the Pip-Boy would currently list it (default visibility). */
  receivable?: boolean;
}

/** `radio.status` payload — the Pip-Boy radio's current state + receivable stations. */
export interface RadioState {
  on: boolean;
  currentRefId: string | null;
  stations?: RadioStation[];
}

export const useRadioStore = defineStore('radio', () => {
  /** Whether the Pip-Boy radio is currently playing. */
  const on = ref(false);
  /** refId of the currently-tuned station, or null when off. */
  const currentRefId = ref<string | null>(null);
  /** Receivable (in-range) stations the player can tune to. */
  const stations = ref<RadioStation[]>([]);

  function setRadio(data: RadioState | null | undefined): void {
    if (!data) {
      on.value = false;
      currentRefId.value = null;
      stations.value = [];
      return;
    }
    on.value = !!data.on;
    currentRefId.value = data.currentRefId ?? null;
    stations.value = Array.isArray(data.stations) ? data.stations : [];
  }

  return { on, currentRefId, stations, setRadio };
});
