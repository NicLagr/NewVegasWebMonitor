import type { ComputedRef } from 'vue';

export interface CharacterStatsDisplay {
  displayLevel: ComputedRef<string>;
  displayExperience: ComputedRef<string>;
  displayCarryWeight: ComputedRef<string>;
  displayCaps: ComputedRef<string>;
  displayKarma: ComputedRef<string>;
  healthPercentage: ComputedRef<number>;
  apPercentage: ComputedRef<number>;
  radsPercentage: ComputedRef<number>;
}
