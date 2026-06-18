import { defineStore } from 'pinia';
import { ref } from 'vue';

/** `radio.status` payload — the Pip-Boy radio's current state. */
export interface RadioState {
  on: boolean;
  station: string | null;
}

export const useRadioStore = defineStore('radio', () => {
  /** Whether the Pip-Boy radio is currently playing. */
  const on = ref(false);
  /** Name of the currently-tuned station, or null when off. */
  const station = ref<string | null>(null);

  function setRadio(data: RadioState | null | undefined): void {
    if (!data) {
      on.value = false;
      station.value = null;
      return;
    }
    on.value = !!data.on;
    station.value = data.station ?? null;
  }

  return { on, station, setRadio };
});
