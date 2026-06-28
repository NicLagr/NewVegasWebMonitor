<template>
  <inventory-item
    :name="name"
    :quantity="quantity"
    :is-favorite="isFavorite"
    :active="active"
  >
    <template #icon>
      <pipboy-icon
        :icon-path="iconPath"
        :size="28"
      >
        <weapon-icon
          :weapon-type="weaponType"
          :size="28"
        />
      </pipboy-icon>
    </template>
    <template #status>
      <equipped-hand-icon
        v-if="isEquipped"
        :equipped-hand="equippedHand"
      />
    </template>
  </inventory-item>
</template>

<script setup lang="ts">
import { InventoryItem } from '@/shared/ui/items';
import { PipboyIcon } from '@/shared/ui';
import { WeaponIcon } from '@/entities/ui';
import { EquippedHandIcon } from '@/features/ui';
import type { WeaponType } from '@/stores/inventory/types';
import type { EquippedHand } from '@/shared/lib/types/common';

defineProps<{
  name: string;
  weaponType: WeaponType;
  iconPath?: string;
  quantity?: number;
  isFavorite?: boolean;
  isEquipped?: boolean;
  equippedHand?: EquippedHand;
  active?: boolean;
}>();
</script>
