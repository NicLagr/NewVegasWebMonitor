import type { ComputedRef } from 'vue';
import type { SpecialStats, LimbCondition } from '@/stores/character/types';

export interface CharacterStatsDisplay {
  displayLevel: ComputedRef<string>;
  displayExperience: ComputedRef<string>;
  displayCarryWeight: ComputedRef<string>;
  displayCaps: ComputedRef<string>;
  displayKarma: ComputedRef<string>;
  displayHealth: ComputedRef<string>;
  displayAp: ComputedRef<string>;
  healthPercentage: ComputedRef<number>;
  apPercentage: ComputedRef<number>;
  radsPercentage: ComputedRef<number>;
  special: ComputedRef<SpecialStats>;
  limbs: ComputedRef<LimbCondition>;
  effects: ComputedRef<string[]>;
}
