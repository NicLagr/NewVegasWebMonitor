import { computed } from 'vue';
import { storeToRefs } from 'pinia';
import { useCharacterStore } from '@/stores/character/useCharacterStore';
import type { CharacterStatsDisplay } from './types';

/**
 * Use character stats display
 *
 * Computes all display values for the character stats page
 * @returns Display values ready for rendering
 */
export function useCharacterStatsDisplay(): CharacterStatsDisplay {
  const characterStore = useCharacterStore();
  const { stats, statsPercentage } = storeToRefs(characterStore);

  /**
   * Format level for display
   */
  const displayLevel = computed(() => {
    const level = stats.value.level;
    return level ? String(level) : '-';
  });

  /**
   * Format experience for display (current / next)
   */
  const displayExperience = computed(() => {
    const xp = stats.value.xp;
    const xpNext = stats.value.xpNext;

    if (xp !== null && xp !== undefined && xpNext !== null && xpNext !== undefined) {
      return `${String(xp)} / ${String(xpNext)}`;
    }
    return '-';
  });

  /**
   * Format carry weight for display (current / max)
   */
  const displayCarryWeight = computed(() => {
    const inventoryWeight = stats.value.inventoryWeight;
    const carryWeight = stats.value.carryWeight;

    if (
      inventoryWeight !== null &&
      inventoryWeight !== undefined &&
      carryWeight !== null &&
      carryWeight !== undefined
    ) {
      return `${String(Math.round(inventoryWeight))} / ${String(Math.round(carryWeight))}`;
    }
    return '-';
  });

  /**
   * Format caps (currency) for display
   */
  const displayCaps = computed(() => {
    const caps = stats.value.caps;
    return caps ? String(caps) : '-';
  });

  /**
   * Format karma for display
   */
  const displayKarma = computed(() => {
    const karma = stats.value.karma;
    return karma !== null && karma !== undefined ? String(karma) : '-';
  });

  /**
   * Get health (HP) percentage for display
   */
  const healthPercentage = computed(() => statsPercentage.value.health);

  /**
   * Get action points (AP) percentage for display
   */
  const apPercentage = computed(() => statsPercentage.value.ap);

  /**
   * Get radiation (Rads) percentage for display
   */
  const radsPercentage = computed(() => statsPercentage.value.rads);

  /** HP current / max, e.g. "180 / 200" (for the STATS header readout). */
  const displayHealth = computed(() => {
    const h = stats.value.health;
    const m = stats.value.healthBase;
    return h != null && m != null ? `${Math.round(h)} / ${Math.round(m)}` : '-';
  });

  /** AP current / max, e.g. "65 / 77". */
  const displayAp = computed(() => {
    const a = stats.value.ap;
    const m = stats.value.apBase;
    return a != null && m != null ? `${Math.round(a)} / ${Math.round(m)}` : '-';
  });

  return {
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
  };
}
