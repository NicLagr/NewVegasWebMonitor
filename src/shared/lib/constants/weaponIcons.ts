import { WEAPON_TYPES, type WeaponType } from '@/stores/inventory/types';

// FNV weapon icons. The backend reuses the engine WeaponType slots to carry
// Fallout weapon classes, so each slot is mapped to a thematically-fitting
// firearm / melee icon from the bundled GameIcons.net set.
export const WEAPON_ICON_PATHS: Record<Exclude<WeaponType, null>, string> = {
  [WEAPON_TYPES.ONE_HAND_SWORD]: 'skoll/revolver.svg',          // pistols
  [WEAPON_TYPES.ONE_HAND_DAGGER]: 'lorc/bowie-knife.svg',       // knives
  [WEAPON_TYPES.ONE_HAND_AXE]: 'lorc/machete.svg',              // bladed melee
  [WEAPON_TYPES.ONE_HAND_MACE]: 'delapouite/baseball-bat.svg',  // blunt melee
  [WEAPON_TYPES.TWO_HAND_SWORD]: 'skoll/winchester-rifle.svg',  // rifles
  [WEAPON_TYPES.TWO_HAND_AXE]: 'delapouite/sawed-off-shotgun.svg', // shotguns
  [WEAPON_TYPES.BOW]: 'skoll/machine-gun.svg',                  // automatics
  [WEAPON_TYPES.STAFF]: 'lorc/ray-gun.svg',                     // energy weapons
  [WEAPON_TYPES.CROSSBOW]: 'lorc/minigun.svg',                  // heavy weapons
  [WEAPON_TYPES.HAND_TO_HAND]: 'delapouite/brass-knuckles.svg', // unarmed
};

export function getWeaponIconPath(weaponType: WeaponType): string {
  if (!weaponType) {
    return WEAPON_ICON_PATHS[WEAPON_TYPES.ONE_HAND_SWORD];
  }
  return WEAPON_ICON_PATHS[weaponType];
}
