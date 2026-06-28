<template>
  <div class="stats-page">
    <!-- Iconic FNV STATS readout bar. -->
    <div class="stats-header">
      <span class="sh-title">{{ $t('pages.character.stats.tab') }}</span>
      <div class="sh-cells">
        <div class="sh-cell">
          <span class="sh-k">LVL</span><span class="sh-v">{{ displayLevel }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">HP</span><span class="sh-v">{{ displayHealth }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">AP</span><span class="sh-v">{{ displayAp }}</span>
        </div>
        <div class="sh-cell">
          <span class="sh-k">XP</span><span class="sh-v">{{ displayExperience }}</span>
        </div>
      </div>
    </div>

    <!-- CND / RAD column + Vault Boy with limb-condition bars. -->
    <div class="stats-main">
      <div class="stats-side">
        <div class="side-row">
          <span class="side-k">CND</span><span class="side-v">{{ cndPct }}%</span>
        </div>
        <div class="side-row">
          <span class="side-k">RAD</span><span class="side-v">{{ Math.round(radsPercentage) }}%</span>
        </div>
        <div class="side-row">
          <span class="side-k">EFF</span><span class="side-v">{{ visibleEffects.length }}</span>
        </div>
      </div>

      <div class="stats-figure">
        <div
          class="vaultboy"
          :style="{ '--vb-src': `url('${vaultboySrc}')` }"
        />
        <div
          v-for="l in limbBars"
          :key="l.key"
          class="limb"
          :class="{ 'limb--low': l.pct < 35 }"
          :style="l.pos"
        >
          <span
            class="limb-fill"
            :style="{ width: l.pct + '%' }"
          />
        </div>
      </div>
    </div>

    <div class="stats-level">{{ $t('pages.character.stats.level') }} {{ displayLevel }}</div>

    <!-- Active effects (EFF). -->
    <div
      v-if="visibleEffects.length"
      class="stats-effects"
    >
      <span
        v-for="e in visibleEffects"
        :key="e.name"
        class="eff-chip"
        :class="{ 'eff-chip--hidden': e.hidden }"
      >{{ e.name }}</span>
    </div>

    <!-- S.P.E.C.I.A.L. readout. -->
    <div class="stats-special">
      <div
        v-for="s in specialList"
        :key="s.k"
        class="spec"
      >
        <span class="spec-k">{{ s.k }}</span>
        <span class="spec-v">{{ s.v }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { storeToRefs } from 'pinia';
import { useCharacterStatsDisplay } from './composables/useCharacterStatsDisplay';
import { useSettingsStore } from '@/stores/settings/useSettingsStore';

const {
  displayLevel,
  displayExperience,
  displayHealth,
  displayAp,
  radsPercentage,
  special,
  limbs,
  effects,
} = useCharacterStatsDisplay();

const { showHiddenEffects } = storeToRefs(useSettingsStore());

// Match the in-game Pip-Boy EFF page by default (hides ability/passive effects);
// reveal everything when "Show hidden effects" is on.
const visibleEffects = computed(() =>
  effects.value.filter((e) => showHiddenEffects.value || !e.hidden)
);

const vaultboySrc = `${import.meta.env.BASE_URL}icons/pipboy/ui/vaultboy.svg`;

const clampPct = (v: number | undefined): number =>
  v == null ? 100 : Math.max(0, Math.min(100, v));

// Limb-condition bars positioned around the Vault Boy (vitruvian pose).
const limbBars = computed(() => {
  const L = limbs.value;
  return [
    { key: 'head', pct: clampPct(L.head), pos: 'top:4%;left:50%' },
    { key: 'leftArm', pct: clampPct(L.leftArm), pos: 'top:33%;left:18%' },
    { key: 'rightArm', pct: clampPct(L.rightArm), pos: 'top:33%;left:82%' },
    { key: 'torso', pct: clampPct(L.torso), pos: 'top:40%;left:50%' },
    { key: 'leftLeg', pct: clampPct(L.leftLeg), pos: 'top:74%;left:32%' },
    { key: 'rightLeg', pct: clampPct(L.rightLeg), pos: 'top:74%;left:68%' },
  ];
});

const cndPct = computed(() => {
  const vals = Object.values(limbs.value).filter(
    (v): v is number => typeof v === 'number'
  );
  if (!vals.length) return 100;
  return Math.round(vals.reduce((a, b) => a + b, 0) / vals.length);
});

// S.P.E.C.I.A.L. — one letter + value each.
const specialList = computed(() => {
  const s = special.value;
  return [
    { k: 'S', v: s.strength ?? '-' },
    { k: 'P', v: s.perception ?? '-' },
    { k: 'E', v: s.endurance ?? '-' },
    { k: 'C', v: s.charisma ?? '-' },
    { k: 'I', v: s.intelligence ?? '-' },
    { k: 'A', v: s.agility ?? '-' },
    { k: 'L', v: s.luck ?? '-' },
  ];
});
</script>

<style scoped lang="scss">
/* FNV STATS screen: segmented header, CND/RAD column, Vault Boy with live
   limb-condition bars, and a S.P.E.C.I.A.L. readout — theme-tinted and
   clamp()-scaled to fit small displays (e.g. AYN Thor 1080×1240). */
.stats-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  padding: clamp(var(--spacing-sm), 2vh, var(--spacing-lg));
  gap: clamp(var(--spacing-sm), 1.5vh, var(--spacing-md));
}

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

.stats-main {
  flex: 1 1 auto;
  min-height: 0;
  display: flex;
  gap: var(--spacing-sm);
}

.stats-side {
  flex: 0 0 auto;
  display: flex;
  flex-direction: column;
  gap: var(--spacing-sm);
  font-family: var(--font-heading);
}

.side-row {
  display: flex;
  flex-direction: column;
  border: var(--border-thin) solid var(--skyrim-border-dark);
  border-radius: var(--radius-sm);
  padding: 2px var(--spacing-sm);
}

.side-k {
  color: var(--skyrim-text-dim);
  font-size: var(--font-size-xs);
  letter-spacing: 0.08em;
}

.side-v {
  color: var(--skyrim-text-primary);
  font-size: var(--font-size-sm);
  font-variant-numeric: tabular-nums;
}

/* Vault Boy + limb bars. */
.stats-figure {
  position: relative;
  flex: 1 1 auto;
  min-height: 0;
}

.vaultboy {
  position: absolute;
  inset: 0;
  background-color: var(--skyrim-text-accent);
  opacity: 0.92;
  -webkit-mask: var(--vb-src) center / contain no-repeat;
  mask: var(--vb-src) center / contain no-repeat;
}

.limb {
  position: absolute;
  transform: translate(-50%, -50%);
  width: clamp(34px, 9vw, 58px);
  height: 8px;
  border: var(--border-thin) solid var(--skyrim-border-medium);
  background-color: var(--skyrim-bg-dark);
  border-radius: 1px;
  overflow: hidden;

  &--low {
    border-color: var(--color-danger);
  }
}

.limb-fill {
  display: block;
  height: 100%;
  background-color: var(--skyrim-accent-gold);
  transition: width var(--transition-normal);

  .limb--low & {
    background-color: var(--color-danger);
  }
}

.stats-level {
  flex: 0 0 auto;
  text-align: center;
  color: var(--skyrim-text-accent);
  font-family: var(--font-heading);
  font-size: clamp(var(--font-size-sm), 2vh, var(--font-size-lg));
  letter-spacing: 0.08em;
}

/* Active effects (EFF). */
.stats-effects {
  flex: 0 0 auto;
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  gap: var(--spacing-xs);
  max-height: 18%;
  overflow-y: auto;
}

.eff-chip {
  padding: 1px var(--spacing-sm);
  border: var(--border-thin) solid var(--skyrim-border-medium);
  border-radius: var(--radius-sm);
  background-color: rgb(var(--accent-rgb) / 8%);
  color: var(--skyrim-text-secondary);
  font-family: var(--font-heading);
  font-size: var(--font-size-xs);
  white-space: nowrap;
}

/* Normally-hidden ability/passive effects (shown only via the setting). */
.eff-chip--hidden {
  border-style: dashed;
  background-color: transparent;
  color: var(--skyrim-text-dim);
}

/* S.P.E.C.I.A.L. */
.stats-special {
  flex: 0 0 auto;
  display: flex;
  gap: var(--spacing-xs);
  border-top: var(--border-thin) solid var(--skyrim-border-dark);
  padding-top: var(--spacing-sm);
}

.spec {
  flex: 1 1 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  font-family: var(--font-heading);
}

.spec-k {
  color: var(--skyrim-text-secondary);
  font-size: var(--font-size-sm);
  letter-spacing: 0.05em;
}

.spec-v {
  color: var(--skyrim-text-primary);
  font-size: clamp(var(--font-size-base), 2.4vh, var(--font-size-xl));
  font-variant-numeric: tabular-nums;
}
</style>
