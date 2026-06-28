<template>
  <div class="stats-page">
    <section class="stats-attrs">
      <attribute-row
        :label="$t('pages.character.stats.level')"
        :value="displayLevel"
      />
      <attribute-row
        :label="$t('pages.character.stats.experience')"
        :value="displayExperience"
      />
      <attribute-row
        :label="$t('pages.character.stats.carryWeight')"
        :value="displayCarryWeight"
      />
      <attribute-row
        :label="$t('pages.character.stats.caps')"
        :value="displayCaps"
      />
      <attribute-row
        :label="$t('pages.character.stats.karma')"
        :value="displayKarma"
      />
    </section>

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
  healthPercentage,
  apPercentage,
  radsPercentage,
} = useCharacterStatsDisplay();
</script>

<style scoped lang="scss">
/* Adaptive layout: fits the whole STATUS screen without cropping, scaling
   spacing/type to the viewport (e.g. the AYN Thor's 1080×1240 bottom screen).
   Attributes in a 2-column grid up top; vital bars anchored at the bottom. */
.stats-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  padding: clamp(var(--spacing-sm), 2.5vh, var(--spacing-lg));
  gap: clamp(var(--spacing-sm), 2.5vh, var(--spacing-lg));
}

.stats-attrs {
  display: grid;
  grid-template-columns: 1fr 1fr;
  column-gap: var(--spacing-lg);
  row-gap: clamp(0.1rem, 1vh, var(--spacing-sm));
  align-content: start;

  :deep(.attribute-row) {
    font-size: clamp(var(--font-size-sm), 2.2vh, var(--font-size-lg));
  }

  /* The fixed 120px label width overflows narrow grid columns. */
  :deep(.attr-label) {
    min-width: 0;
  }
}

.stats-bars {
  display: flex;
  flex-direction: column;
  gap: clamp(var(--spacing-xs), 2vh, var(--spacing-md));
  margin-top: auto;
}
</style>
