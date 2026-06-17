import type { HotkeySlotEntry } from '@/stores/hotkeys/types';
import type { CategoryType } from '@/stores/inventory/types';
import { getWeaponIconPath } from '@/shared/lib/constants/weaponIcons';
import { getApparelIconPath } from '@/shared/lib/constants/apparelIcons';

/**
 * Icons shown in hotkey slot corner / picker button.
 * Mapped per category to stay DRY.
 * For Weapon / Apparel the concrete sub-type icon is reused from
 * the same mappings powering WeaponIcon / ApparelIcon.
 */
const CATEGORY_ICON_PATHS: Record<string, string> = {
  Weapon: 'lorc/crossed-pistols.svg',
  Ammo: 'lorc/bullets.svg',
  Apparel: 'lorc/leather-vest.svg',
  Book: 'delapouite/audio-cassette.svg',
  Potion: 'delapouite/medicine-pills.svg',
  Food: 'delapouite/canned-fish.svg',
  Ingredient: 'skoll/pestle-mortar.svg',
  Misc: 'lorc/swap-bag.svg',
  Key: 'lorc/key.svg',
  SoulGem: 'lorc/crystal-shine.svg',
  Scroll: 'lorc/book-cover.svg',
  Unknown: 'lorc/cog.svg',
};

const SPELL_TYPE_FALLBACK: Record<string, string> = {
  Shout: 'lorc/shouting.svg',
  VoicePower: 'lorc/shouting.svg',
  Power: 'lorc/star-swirl.svg',
  LesserPower: 'lorc/star-swirl.svg',
};

const FALLBACK_ICON = 'lorc/cog.svg';

export function getCategoryIconPath(categoryType: CategoryType | 'Unknown' | undefined | null): string {
  if (!categoryType) return FALLBACK_ICON;
  return CATEGORY_ICON_PATHS[categoryType] ?? FALLBACK_ICON;
}

export function getHotkeyIconPath(entry: HotkeySlotEntry | null | undefined): string | null {
  if (!entry || !entry.bound) return null;
  if (entry.kind === 'item') {
    // Prefer specific sub-type icons for weapons / apparel — same mapping as WeaponIcon / ApparelIcon.
    if (entry.categoryType === 'Weapon' && entry.weaponType) {
      return getWeaponIconPath(entry.weaponType);
    }
    if (entry.categoryType === 'Apparel' && entry.bodySlot) {
      return getApparelIconPath(entry.bodySlot);
    }
    return getCategoryIconPath(entry.categoryType);
  }
  // spell / ability
  return SPELL_TYPE_FALLBACK[entry.spellType] ?? FALLBACK_ICON;
}
