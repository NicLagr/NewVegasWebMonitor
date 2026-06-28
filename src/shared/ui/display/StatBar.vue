<template>
  <div class="stat-bar">
    <div class="stat-header">
      <span
        v-if="label"
        class="stat-label"
      >
        {{ label }}
      </span>
    </div>
    <div class="stat-track">
      <div
        class="stat-fill"
        :class="`stat-fill--${color}`"
        :style="{ width: `${pct}%` }"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';

const props = defineProps<{
  label?: string;
  value: number;
  max: number;
  color: 'hp' | 'ap' | 'rads';
}>();

const pct = computed(() => (props.value / props.max) * 100);
</script>

<style scoped lang="scss">
.stat-bar {
  display: flex;
  flex-direction: column;
  gap: var(--spacing-xs);
}

.stat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  color: var(--pip-text-accent);
  letter-spacing: 0.05em;
}

.stat-value {
  font-size: var(--font-size-xs);
  color: var(--pip-text-secondary);
}

.stat-track {
  height: 1rem;
  background-color: var(--pip-bg-dark);
  border: 1px solid var(--pip-border-dark);
  border-radius: var(--radius-sm);
  overflow: hidden;
}

.stat-fill {
  height: 100%;
  transition: width var(--transition-normal);

  &--hp {
    background: linear-gradient(90deg, #0a5a22, #00ff41);
    box-shadow: 0 0 8px rgb(0 255 65 / 40%);
  }

  &--ap {
    background: linear-gradient(90deg, #0a5a4a, #00e0c0);
    box-shadow: 0 0 8px rgb(0 224 192 / 40%);
  }

  &--rads {
    background: linear-gradient(90deg, #6b6b1f, #d4d432);
    box-shadow: 0 0 8px rgb(212 212 50 / 40%);
  }
}
</style>
