<template>
  <div class="modal-content settings-modal">
    <div class="modal-header">
      <p class="modal-title">{{ $t('settings.title') }}</p>
    </div>

    <div class="settings-section">
      <p class="settings-label">{{ $t('settings.theme') }}</p>
      <div class="theme-options">
        <button
          v-for="t in themes"
          :key="t"
          type="button"
          class="theme-option"
          :class="{ active: theme === t }"
          @click="settings.setTheme(t)"
        >
          <span
            class="theme-swatch"
            :data-theme="t"
          />
          <span class="theme-name">{{ $t(`settings.themes.${t}`) }}</span>
        </button>
      </div>
    </div>

    <div class="settings-section">
      <p class="settings-label">{{ $t('settings.status') }}</p>
      <button
        type="button"
        class="toggle-row"
        :class="{ on: showHiddenEffects }"
        :aria-pressed="showHiddenEffects"
        @click="settings.setShowHiddenEffects(!showHiddenEffects)"
      >
        <span class="toggle-text">{{ $t('settings.showHiddenEffects') }}</span>
        <span class="toggle-switch"><span class="toggle-knob" /></span>
      </button>
    </div>

    <div class="settings-section">
      <p class="settings-label">{{ $t('settings.credits') }}</p>
      <p class="credit-line">
        {{ $t('settings.iconsBy') }}
        <a
          href="https://www.nexusmods.com/profile/ItsMeJesusHChrist"
          target="_blank"
          rel="noopener"
        >Consistent Pip-Boy Icons</a>
        · ItsMeJesusHChrist
      </p>
      <p class="credit-line">
        {{ $t('settings.figureBy') }} fluffgar (DeviantArt)
      </p>
    </div>

    <div class="modal-actions">
      <button
        class="btn btn-lg btn-primary"
        @click="emit('close')"
      >
        {{ $t('settings.close') }}
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia';
import { useSettingsStore } from '@/stores/settings/useSettingsStore';

const settings = useSettingsStore();
const { theme, showHiddenEffects } = storeToRefs(settings);
const themes = settings.themes;

const emit = defineEmits<{ close: [] }>();
</script>

<style scoped lang="scss">
.settings-modal {
  min-width: 300px;
  max-width: 420px;
}

.settings-section {
  margin-bottom: var(--spacing-lg);
}

.settings-label {
  color: var(--pip-text-dim);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.12em;
  text-transform: uppercase;
  margin-bottom: var(--spacing-sm);
}

.theme-options {
  display: flex;
  gap: var(--spacing-sm);
}

.theme-option {
  flex: 1 1 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--spacing-xs);
  padding: var(--spacing-sm);
  background-color: var(--pip-bg-dark);
  border: var(--border-thin) solid var(--pip-border-dark);
  border-radius: var(--radius-sm);
  color: var(--pip-text-secondary);
  font-family: var(--font-heading);
  cursor: pointer;
  transition:
    border-color var(--transition-fast),
    color var(--transition-fast);

  &:hover {
    color: var(--pip-text-primary);
  }

  &.active {
    border-color: var(--pip-border-accent);
    color: var(--pip-text-accent);
  }
}

.theme-swatch {
  width: 1.75rem;
  height: 1.75rem;
  border-radius: 999px;
  /* Resolves to this swatch's own palette accent via its data-theme. */
  background-color: var(--pip-accent-gold);
  box-shadow: 0 0 8px rgb(var(--accent-rgb) / 50%);
  border: var(--border-thin) solid rgb(0 0 0 / 30%);
}

.theme-name {
  font-size: var(--font-size-sm);
}

.toggle-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  gap: var(--spacing-md);
  padding: var(--spacing-sm);
  background-color: var(--pip-bg-dark);
  border: var(--border-thin) solid var(--pip-border-dark);
  border-radius: var(--radius-sm);
  color: var(--pip-text-secondary);
  font-family: var(--font-heading);
  cursor: pointer;

  &.on {
    color: var(--pip-text-accent);
    border-color: var(--pip-border-accent);
  }
}

.toggle-text {
  font-size: var(--font-size-sm);
}

.toggle-switch {
  flex: 0 0 auto;
  position: relative;
  width: 2.25rem;
  height: 1.1rem;
  border: var(--border-thin) solid var(--pip-border-medium);
  border-radius: 999px;
  transition: border-color var(--transition-fast);

  .on & {
    border-color: var(--pip-border-accent);
  }
}

.toggle-knob {
  position: absolute;
  top: 1px;
  left: 1px;
  width: calc(1.1rem - 4px);
  height: calc(1.1rem - 4px);
  border-radius: 999px;
  background-color: var(--pip-text-dim);
  transition: transform var(--transition-fast), background-color var(--transition-fast);

  .on & {
    background-color: var(--pip-accent-gold);
    transform: translateX(1.15rem);
  }
}

.credit-line {
  color: var(--pip-text-secondary);
  font-size: var(--font-size-sm);
  line-height: 1.4;

  a {
    color: var(--pip-text-accent);
    text-decoration: underline;
  }
}
</style>
