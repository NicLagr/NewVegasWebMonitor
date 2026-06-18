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
      v-if="stations.length"
      class="radio-stations"
    >
      <li
        v-for="s in stations"
        :key="s.refId"
        class="radio-station"
        :class="{ 'is-current': on && s.refId === currentRefId }"
        @click="tune(s.refId)"
      >
        <span class="radio-station__dot" />
        <span class="radio-station__name">{{ s.name }}</span>
      </li>
    </ul>
    <div
      v-else
      class="radio-empty"
    >
      {{ $t('pages.radio.status.noStations') }}
    </div>

    <button
      type="button"
      class="radio-off-btn"
      :disabled="!on"
      @click="turnOff"
    >
      {{ $t('pages.radio.status.turnOff') }}
    </button>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { storeToRefs } from 'pinia';
import { useRadioStore } from '@/stores/radio/useRadioStore';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';

const radioStore = useRadioStore();
const { on, currentRefId, stations } = storeToRefs(radioStore);
const wsStore = useWebSocketStore();

const currentName = computed(
  () => stations.value.find((s) => s.refId === currentRefId.value)?.name ?? null
);

function tune(refId: string): void {
  // Tapping the active station turns the radio off; otherwise tune to it.
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
  color: var(--skyrim-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.radio-now {
  color: var(--skyrim-text-accent);
  font-family: var(--font-heading);
  font-size: var(--font-size-xl);

  &.is-off {
    color: var(--skyrim-text-dim);
  }
}

.radio-stations {
  flex: 1 1 auto;
  min-height: 0;
  overflow-y: auto;
  list-style: none;
  margin: 0;
  padding: 0;
  border-top: var(--border-thin) solid var(--skyrim-border-dark);
}

.radio-station {
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
  padding: var(--spacing-sm) var(--spacing-xs);
  border-bottom: var(--border-thin) solid var(--skyrim-border-dark);
  color: var(--skyrim-text-primary);
  font-family: var(--font-heading);
  font-size: var(--font-size-lg);
  cursor: pointer;

  &:hover {
    color: var(--skyrim-text-accent);
  }

  &.is-current {
    color: var(--skyrim-text-accent);

    .radio-station__dot {
      background-color: var(--skyrim-text-accent);
      box-shadow: 0 0 6px var(--skyrim-text-accent);
    }
  }

  &__dot {
    width: 0.5rem;
    height: 0.5rem;
    border-radius: 999px;
    border: var(--border-thin) solid var(--skyrim-border-medium);
    flex: 0 0 auto;
  }
}

.radio-empty {
  flex: 1 1 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--skyrim-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
}

.radio-off-btn {
  flex: 0 0 auto;
  align-self: flex-start;
  padding: var(--spacing-xs) var(--spacing-lg);
  border: var(--border-thin) solid var(--skyrim-border-medium);
  border-radius: var(--radius-sm);
  background-color: var(--skyrim-bg-dark);
  color: var(--skyrim-text-primary);
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
  letter-spacing: 0.08em;
  cursor: pointer;

  &:hover:not(:disabled) {
    border-color: var(--skyrim-border-accent);
    color: var(--skyrim-text-accent);
  }

  &:disabled {
    opacity: 0.4;
    cursor: default;
  }
}
</style>
