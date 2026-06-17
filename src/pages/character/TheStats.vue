<template>
  <div class="d-flex flex-col flex-1 justify-between">
    <div class="list">
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
    </div>
    <div class="d-flex flex-col gap-md">
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
    </div>
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

<!-- All layout via utility classes; no scoped styles required. -->
