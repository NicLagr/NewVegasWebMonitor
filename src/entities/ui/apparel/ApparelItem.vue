<template>
  <inventory-item
    :name="name"
    :quantity="quantity"
    :is-favorite="isFavorite"
    :active="active"
    @click="$emit('click')"
  >
    <template #icon>
      <pipboy-icon
        :icon-path="iconPath"
        :size="28"
      >
        <apparel-icon
          :body-slots="bodySlots"
          :size="28"
        />
      </pipboy-icon>
    </template>
    <template #status>
      <equip-status :is-equipped="isEquipped" />
    </template>
  </inventory-item>
</template>

<script setup lang="ts">
import { InventoryItem, EquipStatus } from '@/shared/ui/items';
import { PipboyIcon } from '@/shared/ui';
import { ApparelIcon } from '@/entities/ui';
import type { ArmorType, BodySlot } from '@/stores/inventory/types';

defineProps<{
  name: string;
  armorType: ArmorType;
  bodySlots?: BodySlot[];
  iconPath?: string;
  quantity?: number;
  isFavorite?: boolean;
  isEquipped?: boolean;
  active?: boolean;
}>();

defineEmits<{
  click: [];
}>();
</script>
