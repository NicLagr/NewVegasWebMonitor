<template>
  <div class="radio-page">
    <div class="radio-header">
      <span class="radio-label">{{ $t('pages.radio.status.nowPlaying') }}</span>
      <span
        class="radio-now"
        :class="{ 'is-off': !on }"
      >{{ on && currentName ? currentName : $t('pages.radio.status.off') }}</span>
    </div>

    <ul
      v-if="visibleStations.length"
      class="radio-stations"
    >
      <li
        v-for="s in visibleStations"
        :key="s.refId"
        class="radio-station"
        :class="{ 'is-current': on && s.refId === currentRefId }"
        @click="tune(s.refId)"
      >
        <span class="radio-station__dot" />
        <span class="radio-station__name">{{ s.name }}</span>
        <button
          type="button"
          class="radio-station__action"
          :title="$t('pages.radio.status.hide')"
          @click.stop="prefs.hide(s.name)"
        >✕</button>
      </li>
    </ul>
    <div
      v-else
      class="radio-empty"
    >
      {{ $t('pages.radio.status.noStations') }}
    </div>

    <div class="radio-footer">
      <button
        type="button"
        class="radio-text-btn"
        :disabled="!on"
        @click="turnOff"
      >
        {{ $t('pages.radio.status.turnOff') }}
      </button>
      <button
        v-if="hiddenStations.length"
        type="button"
        class="radio-text-btn"
        @click="showHidden = !showHidden"
      >
        {{ showHidden ? $t('pages.radio.status.hideHidden') : $t('pages.radio.status.showHidden', { n: hiddenStations.length }) }}
      </button>
    </div>

    <ul
      v-if="showHidden && hiddenStations.length"
      class="radio-stations radio-stations--hidden"
    >
      <li
        v-for="s in hiddenStations"
        :key="s.refId"
        class="radio-station is-hidden"
      >
        <span class="radio-station__name">{{ s.name }}</span>
        <button
          type="button"
          class="radio-station__action"
          :title="$t('pages.radio.status.unhide')"
          @click.stop="prefs.show(s.name)"
        >↺</button>
      </li>
    </ul>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { useRadioStore } from '@/stores/radio/useRadioStore';
import { useStationPrefsStore } from '@/stores/radio/useStationPrefsStore';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';

const radioStore = useRadioStore();
const { on, currentRefId, stations } = storeToRefs(radioStore);
const prefs = useStationPrefsStore();
const { overrides } = storeToRefs(prefs);
const wsStore = useWebSocketStore();

const showHidden = ref(false);

// Default visibility follows the plugin's `receivable` flag; a user override wins.
// `overrides` is referenced so the lists recompute when prefs change.
function stationVisible(s: { name: string; receivable?: boolean }): boolean {
  void overrides.value;
  return prefs.isVisible(s.name, s.receivable !== false);
}
const visibleStations = computed(() => stations.value.filter(stationVisible));
const hiddenStations = computed(() => stations.value.filter((s) => !stationVisible(s)));

const currentName = computed(
  () => stations.value.find((s) => s.refId === currentRefId.value)?.name ?? null
);

function tune(refId: string): void {
  if (on.value && refId === currentRefId.value) {
    wsStore.sendCommand({ command: 'radio_off' });
  } else {
    wsStore.sendCommand({ command: 'radio_tune', formId: refId });
  }
}

function turnOff(): void {
  wsStore.sendCommand({ command: 'radio_off' });
}
</script>

<style scoped lang="scss">
.radio-page {
  display: flex;
  flex-direction: column;
  gap: var(--spacing-md);
  width: 100%;
  height: 100%;
  min-height: 0;
  padding: var(--spacing-lg);
}

.radio-header {
  display: flex;
  flex-direction: column;
  gap: 0.15rem;
  flex: 0 0 auto;
}

.radio-label {
  color: var(--pip-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.radio-now {
  color: var(--pip-text-accent);
  font-family: var(--font-heading);
  font-size: var(--font-size-xl);

  &.is-off {
    color: var(--pip-text-dim);
  }
}

.radio-stations {
  flex: 1 1 auto;
  min-height: 0;
  overflow-y: auto;
  list-style: none;
  margin: 0;
  padding: 0;
  border-top: var(--border-thin) solid var(--pip-border-dark);

  &--hidden {
    flex: 0 1 auto;
    max-height: 40%;
    opacity: 0.6;
  }
}

.radio-station {
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
  padding: var(--spacing-sm) var(--spacing-xs);
  border-bottom: var(--border-thin) solid var(--pip-border-dark);
  color: var(--pip-text-primary);
  font-family: var(--font-heading);
  font-size: var(--font-size-lg);
  cursor: pointer;

  &:hover {
    color: var(--pip-text-accent);
  }

  &.is-current {
    color: var(--pip-text-accent);

    .radio-station__dot {
      background-color: var(--pip-text-accent);
      box-shadow: 0 0 6px var(--pip-text-accent);
    }
  }

  &.is-hidden {
    cursor: default;
  }

  &__dot {
    width: 0.5rem;
    height: 0.5rem;
    border-radius: 999px;
    border: var(--border-thin) solid var(--pip-border-medium);
    flex: 0 0 auto;
  }

  &__name {
    flex: 1 1 auto;
  }

  &__action {
    flex: 0 0 auto;
    width: 1.5rem;
    height: 1.5rem;
    padding: 0;
    border: var(--border-thin) solid var(--pip-border-dark);
    border-radius: var(--radius-sm);
    background-color: transparent;
    color: var(--pip-text-dim);
    font-size: var(--font-size-sm);
    cursor: pointer;

    &:hover {
      border-color: var(--pip-border-accent);
      color: var(--pip-text-accent);
    }
  }
}

.radio-empty {
  flex: 1 1 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--pip-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
}

.radio-footer {
  flex: 0 0 auto;
  display: flex;
  gap: var(--spacing-md);
  align-items: center;
}

.radio-text-btn {
  padding: var(--spacing-xs) var(--spacing-md);
  border: var(--border-thin) solid var(--pip-border-medium);
  border-radius: var(--radius-sm);
  background-color: var(--pip-bg-dark);
  color: var(--pip-text-primary);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.06em;
  cursor: pointer;

  &:hover:not(:disabled) {
    border-color: var(--pip-border-accent);
    color: var(--pip-text-accent);
  }

  &:disabled {
    opacity: 0.4;
    cursor: default;
  }
}
</style>
