<template>
  <inventory-list
    v-model="activeItem"
    :items="weaponsList"
    :empty-message="$t('pages.inventory.weapons.waitingForData')"
    @favorite="toggleFavorite"
    @drop="startDrop"
    @item-double-click="equipItem"
  >
    <template #default="{ item, active, isFavorite, onSelect }">
      <weapon-item
        v-if="isWeaponItem(item)"
        :name="item.name || $t('pages.inventory.weapons.unknown')"
        :weapon-type="item.weaponType"
        :icon-path="item.iconPath"
        :is-equipped="item.isEquipped || false"
        :equipped-hand="item.equippedHand"
        :is-favorite="isFavorite"
        :active="active"
        :quantity="item.count"
        @click="onSelect"
      />
      <ammo-item
        v-else-if="isAmmoItem(item)"
        :name="item.name || $t('pages.inventory.weapons.unknown')"
        :icon-path="item.iconPath"
        :is-equipped="item.isEquipped || false"
        :is-favorite="isFavorite"
        :active="active"
        :quantity="item.count"
        template
        @click="onSelect"
      />
    </template>

    <template #preview>
      <weapon-preview
        v-if="isWeaponItem(activeItemData)"
        :data="activeItemData"
      />
      <ammo-preview
        v-else-if="isAmmoItem(activeItemData)"
        :data="activeItemData"
      />
    </template>
  </inventory-list>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia';
import {
  WeaponItem,
  AmmoItem,
  WeaponPreview,
  AmmoPreview,
} from '@/entities/ui';
import { InventoryList } from '@/features/ui';
import { useInventoryStore } from '@/stores/inventory/useInventoryStore';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';
import { isWeaponItem, isAmmoItem } from '@/stores/adapters/typeGuards';
import { useInventoryItemActions } from '@/pages/inventory/composables/useInventoryItemActions';

const inventoryStore = useInventoryStore();
const { weaponsList } = storeToRefs(inventoryStore);
const wsStore = useWebSocketStore();

const {
  activeItem,
  activeItemData,
  toggleFavorite,
  startDrop,
} = useInventoryItemActions(() => weaponsList.value);

// FNV has no dual-wielding: a tap simply toggles equip/unequip in the single
// weapon slot — no hand picker.
function equipItem(formId: string) {
  const item = weaponsList.value.find((w) => w.formId === formId);
  if (!item) return;
  wsStore.sendCommand({ command: item.isEquipped ? 'unequip' : 'equip', formId });
}
</script>
