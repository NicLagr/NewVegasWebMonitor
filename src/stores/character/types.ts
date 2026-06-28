/** S.P.E.C.I.A.L. attributes (1–10). */
export interface SpecialStats {
  strength?: number;
  perception?: number;
  endurance?: number;
  charisma?: number;
  intelligence?: number;
  agility?: number;
  luck?: number;
}

/** Limb condition as a percentage (0–100) of each limb's max health. */
export interface LimbCondition {
  head?: number;
  torso?: number;
  leftArm?: number;
  rightArm?: number;
  leftLeg?: number;
  rightLeg?: number;
}

export interface CharacterStats {
  health?: number | null;
  healthBase?: number | null;
  ap?: number | null;
  apBase?: number | null;
  rads?: number | null;
  radsMax?: number | null;
  level?: number | null;
  xp?: number | null;
  xpNext?: number | null;
  inventoryWeight?: number | null;
  carryWeight?: number | null;
  caps?: number | null;
  karma?: number | null;
  special?: SpecialStats | null;
  limbs?: LimbCondition | null;
}
