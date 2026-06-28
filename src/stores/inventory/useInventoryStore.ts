import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { WeaponsState, ApparelState, FoodState, BookState, KeysState, PotionsState, MiscState, WeaponInventoryItem, MiscInventoryItem } from './types';

export const useInventoryStore = defineStore('inventory', () => {
  // State for inventory/weapons page
  const weapons = ref<WeaponsState>({
    items: undefined,
    ammo: undefined,
  });

  // State for inventory/apparel page
  const apparel = ref<ApparelState>({
    items: undefined,
  });

  // State for inventory/food page
  const food = ref<FoodState>({
    items: undefined,
  });

  // State for inventory/potions page
  const potions = ref<PotionsState>({
    items: undefined,
  });

  // State for inventory/books page
  const books = ref<BookState>({
    items: undefined,
  });

  // State for inventory/keys page
  const keys = ref<KeysState>({
    items: undefined,
  });

  // State for inventory/misc page
  const misc = ref<MiscState>({
    items: undefined,
    gems: undefined,
  });

  const weaponsList = computed(() => {
    const combined: WeaponInventoryItem[] = [];
    if (weapons.value.items) combined.push(...weapons.value.items);
    if (weapons.value.ammo) combined.push(...weapons.value.ammo);
    return combined.sort((a, b) => a.name.localeCompare(b.name));
  });

  const apparelList = computed(() => (apparel.value.items || []).sort((a, b) => a.name.localeCompare(b.name)));

  const foodList = computed(() => (food.value.items || []).sort((a, b) => a.name.localeCompare(b.name)));
  const potionsList = computed(() => (potions.value.items || []).sort((a, b) => a.name.localeCompare(b.name)));
  const booksList = computed(() => (books.value.items || []).sort((a, b) => a.name.localeCompare(b.name)));
  const keysList = computed(() => (keys.value.items || []).sort((a, b) => a.name.localeCompare(b.name)));
  const miscList = computed(() => {
    const combined: MiscInventoryItem[] = [];
    if (misc.value.items) combined.push(...misc.value.items);
    if (misc.value.gems) combined.push(...misc.value.gems);
    return combined.sort((a, b) => a.name.localeCompare(b.name));
  });

  const setWeapons = (newWeapons: WeaponsState) => {
    if ('items' in newWeapons) {
      weapons.value.items = newWeapons.items ?? [];
    }
    if ('ammo' in newWeapons) {
      weapons.value.ammo = newWeapons.ammo ?? [];
    }
  };

  const setApparel = (newApparel: ApparelState) => {
    apparel.value = newApparel;
  };

  const setFood = (newFood: FoodState) => {
    food.value = newFood;
  };

  const setPotions = (newPotions: PotionsState) => {
    potions.value = newPotions;
  };

  const setBooks = (newBooks: BookState) => {
    books.value = newBooks;
  };

  const setKeys = (newKeys: KeysState) => {
    keys.value = newKeys;
  };

  const setMisc = (newMisc: MiscState) => {
    if ('items' in newMisc) {
      misc.value.items = newMisc.items ?? [];
    }
    if ('gems' in newMisc) {
      misc.value.gems = newMisc.gems ?? [];
    }
  };

  return {
    weapons,
    apparel,
    misc,
    potions,
    food,
    keys,
    books,
    weaponsList,
    apparelList,
    miscList,
    potionsList,
    foodList,
    keysList,
    booksList,
    setWeapons,
    setApparel,
    setMisc,
    setPotions,
    setFood,
    setKeys,
    setBooks,
  };
});
