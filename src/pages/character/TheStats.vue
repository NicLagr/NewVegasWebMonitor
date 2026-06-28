<template>
  <div class="stats-page">
    <!-- Iconic FNV STATS readout bar. -->
    <div class="stats-header">
      <span class="sh-title">{{ $t('pages.character.stats.tab') }}</span>
      <div class="sh-cells">
        <div class="sh-cell">
          <span class="sh-k">LVL</span>
          <span class="sh-v">{{ displayLevel }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">HP</span>
          <span class="sh-v">{{ displayHealth }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">AP</span>
          <span class="sh-v">{{ displayAp }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">XP</span>
          <span class="sh-v">{{ displayExperience }}</span>
        </div>
      </div>
    </div>

    <!-- Attributes -->
    <section class="stats-attrs">
      <attribute-row
        :label="$t('pages.character.stats.caps')"
        :value="displayCaps"
      />
      <attribute-row
        :label="$t('pages.character.stats.karma')"
        :value="displayKarma"
      />
      <attribute-row
        :label="$t('pages.character.stats.carryWeight')"
        :value="displayCarryWeight"
      />
    </section>

    <!-- Vital bars, anchored at the bottom like the Pip-Boy. -->
    <section class="stats-bars">
      <stat-bar
        :label="$t('pages.character.stats.hp')"
        :value="healthPercentage"
        :max="100"
        color="hp"
      />
      <stat-bar
        :label="$t('pages.character.stats.ap')"
        :value="apPercentage"
        :max="100"
        color="ap"
      />
      <stat-bar
        :label="$t('pages.character.stats.rads')"
        :value="radsPercentage"
        :max="100"
        color="rads"
      />
    </section>
  </div>
</template>

<script setup lang="ts">
import { StatBar, AttributeRow } from '@/shared/ui';
import { useCharacterStatsDisplay } from './composables/useCharacterStatsDisplay';

const {
  displayLevel,
  displayExperience,
  displayCarryWeight,
  displayCaps,
  displayKarma,
  displayHealth,
  displayAp,
  healthPercentage,
  apPercentage,
  radsPercentage,
} = useCharacterStatsDisplay();
</script>

<style scoped lang="scss">
/* FNV-style STATUS screen: segmented header readout up top, attributes in the
   middle, vital bars anchored at the bottom. clamp()-scaled so the whole screen
   fits small displays (e.g. AYN Thor 1080×1240) without cropping. */
.stats-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  padding: clamp(var(--spacing-sm), 2.5vh, var(--spacing-lg));
  gap: clamp(var(--spacing-sm), 2.5vh, var(--spacing-lg));
}

/* Segmented "STATS | LVL | HP | AP | XP" bar. */
.stats-header {
  flex: 0 0 auto;
  display: flex;
  align-items: stretch;
  border: var(--border-thin) solid var(--skyrim-border-accent);
  border-radius: var(--radius-sm);
  background-color: rgb(var(--accent-rgb) / 6%);
  font-family: var(--font-heading);
  overflow: hidden;
}

.sh-title {
  display: flex;
  align-items: center;
  padding: 0 var(--spacing-sm);
  color: var(--skyrim-text-accent);
  font-size: clamp(var(--font-size-sm), 2vh, var(--font-size-lg));
  letter-spacing: 0.1em;
  background-color: rgb(var(--accent-rgb) / 10%);
  border-right: var(--border-thin) solid var(--skyrim-border-accent);
}

.sh-cells {
  display: flex;
  flex: 1 1 auto;
  min-width: 0;
}

.sh-cell {
  flex: 1 1 0;
  min-width: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1px;
  padding: clamp(2px, 0.8vh, var(--spacing-xs)) var(--spacing-xs);

  & + & {
    border-left: var(--border-thin) solid var(--skyrim-border-medium);
  }
}

.sh-k {
  color: var(--skyrim-text-dim);
  font-size: clamp(0.6rem, 1.4vh, var(--font-size-xs));
  letter-spacing: 0.08em;
}

.sh-v {
  color: var(--skyrim-text-primary);
  font-size: clamp(0.75rem, 1.9vh, var(--font-size-base));
  font-variant-numeric: tabular-nums;
  white-space: nowrap;
}

.stats-attrs {
  flex: 0 0 auto;
  display: grid;
  grid-template-columns: 1fr 1fr;
  column-gap: var(--spacing-lg);
  row-gap: clamp(0.1rem, 1vh, var(--spacing-sm));
  align-content: start;

  :deep(.attribute-row) {
    font-size: clamp(var(--font-size-sm), 2.2vh, var(--font-size-lg));
  }

  :deep(.attr-label) {
    min-width: 0;
  }
}

.stats-bars {
  flex: 0 0 auto;
  display: flex;
  flex-direction: column;
  gap: clamp(var(--spacing-xs), 2vh, var(--spacing-md));
  margin-top: auto;
}
</style>
