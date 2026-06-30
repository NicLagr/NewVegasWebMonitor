<template>
  <div class="demo-editor">
    <p class="demo-hint">{{ $t('settings.demo.hint') }}</p>

    <div class="field-grid">
      <label
        v-for="f in coreFields"
        :key="f.key"
        class="field"
      >
        <span class="field-label">{{ $t(`settings.demo.${f.i18n}`) }}</span>
        <input
          v-model.number="stats[f.key]"
          class="input"
          type="number"
          :min="f.min"
          :max="f.max"
          @input="push"
        >
      </label>
    </div>

    <p class="sub-label">{{ $t('settings.demo.special') }}</p>
    <div class="special-grid">
      <label
        v-for="s in specialFields"
        :key="s.key"
        class="special"
      >
        <span class="special-letter">{{ s.letter }}</span>
        <input
          v-model.number="special[s.key]"
          class="input"
          type="number"
          min="1"
          max="10"
          @input="push"
        >
      </label>
    </div>

    <p class="sub-label">{{ $t('settings.demo.limbs') }}</p>
    <div class="field-grid">
      <label
        v-for="l in limbFields"
        :key="l.key"
        class="field"
      >
        <span class="field-label">{{ l.label }}</span>
        <input
          v-model.number="limbs[l.key]"
          class="input"
          type="number"
          min="0"
          max="100"
          @input="push"
        >
      </label>
    </div>

    <div class="demo-actions">
      <button
        class="btn btn-secondary"
        @click="demo.reset()"
      >
        {{ $t('settings.demo.reset') }}
      </button>
      <button
        class="btn btn-ghost"
        @click="demo.exit()"
      >
        {{ $t('settings.demo.exit') }}
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useDemoStore } from '@/stores/demo/useDemoStore';

const demo = useDemoStore();

// Live, editable views into the demo dataset's character.stats block.
const stats = computed(
  () => (demo.data['character.stats'] ?? {}) as Record<string, number>
);
const special = computed(
  () => ((demo.data['character.stats']?.special ?? {}) as Record<string, number>)
);
const limbs = computed(
  () => ((demo.data['character.stats']?.limbs ?? {}) as Record<string, number>)
);

const coreFields = [
  { key: 'level', i18n: 'level', min: 1, max: 50 },
  { key: 'caps', i18n: 'caps', min: 0, max: 999999 },
  { key: 'karma', i18n: 'karma', min: -1000, max: 1000 },
  { key: 'health', i18n: 'hp', min: 0, max: 9999 },
  { key: 'ap', i18n: 'ap', min: 0, max: 9999 },
  { key: 'rads', i18n: 'rads', min: 0, max: 1000 },
] as const;

const specialFields = [
  { key: 'strength', letter: 'S' },
  { key: 'perception', letter: 'P' },
  { key: 'endurance', letter: 'E' },
  { key: 'charisma', letter: 'C' },
  { key: 'intelligence', letter: 'I' },
  { key: 'agility', letter: 'A' },
  { key: 'luck', letter: 'L' },
] as const;

const limbFields = [
  { key: 'head', label: 'Head' },
  { key: 'torso', label: 'Torso' },
  { key: 'leftArm', label: 'L. Arm' },
  { key: 'rightArm', label: 'R. Arm' },
  { key: 'leftLeg', label: 'L. Leg' },
  { key: 'rightLeg', label: 'R. Leg' },
] as const;

function push(): void {
  demo.routeKey('character.stats');
}
</script>

<style scoped lang="scss">
.demo-editor {
  display: flex;
  flex-direction: column;
  gap: var(--spacing-sm);
}

.demo-hint {
  margin: 0 0 var(--spacing-xs);
  font-size: var(--font-size-xs);
  color: var(--pip-text-dim);
  line-height: 1.4;
}

.sub-label {
  margin: var(--spacing-xs) 0 0;
  color: var(--pip-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-xs);
  letter-spacing: 0.1em;
  text-transform: uppercase;
}

.field-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: var(--spacing-sm);
}

.field {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.field-label {
  font-size: var(--font-size-xs);
  color: var(--pip-text-secondary);
  font-family: var(--font-heading);
  letter-spacing: 0.04em;
}

.special-grid {
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  gap: var(--spacing-xs);
}

.special {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}

.special-letter {
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  color: var(--pip-text-accent);
}

.input {
  width: 100%;
  text-align: center;
}

.demo-actions {
  display: flex;
  flex-wrap: wrap;
  gap: var(--spacing-sm);
  margin-top: var(--spacing-xs);
}
</style>
