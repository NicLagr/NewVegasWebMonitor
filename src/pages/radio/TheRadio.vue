<template>
  <div class="radio-page">
    <div class="radio-panel">
      <div class="radio-label">{{ $t('pages.radio.status.nowPlaying') }}</div>
      <div
        class="radio-station"
        :class="{ 'is-off': !on }"
      >
        {{ on && station ? station : $t('pages.radio.status.off') }}
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
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia';
import { useRadioStore } from '@/stores/radio/useRadioStore';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';

const radioStore = useRadioStore();
const { on, station } = storeToRefs(radioStore);
const wsStore = useWebSocketStore();

function turnOff(): void {
  wsStore.sendCommand({ command: 'radio_off' });
}
</script>

<style scoped lang="scss">
.radio-page {
  display: flex;
  flex: 1 1 auto;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  padding: var(--spacing-lg);
}

.radio-panel {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--spacing-md);
  min-width: 16rem;
  padding: var(--spacing-lg) var(--spacing-xl);
  border: var(--border-thin) solid var(--skyrim-border-accent);
  border-radius: var(--radius-md);
  background-color: var(--skyrim-bg-medium);
  box-shadow: var(--shadow-medium);
}

.radio-label {
  color: var(--skyrim-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.radio-station {
  color: var(--skyrim-text-accent);
  font-family: var(--font-heading);
  font-size: var(--font-size-xl);
  text-align: center;

  &.is-off {
    color: var(--skyrim-text-dim);
  }
}

.radio-off-btn {
  margin-top: var(--spacing-sm);
  padding: var(--spacing-xs) var(--spacing-lg);
  border: var(--border-thin) solid var(--skyrim-border-medium);
  border-radius: var(--radius-sm);
  background-color: var(--skyrim-bg-dark);
  color: var(--skyrim-text-primary);
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
  letter-spacing: 0.08em;
  cursor: pointer;
  transition: border-color var(--transition-fast), color var(--transition-fast);

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
